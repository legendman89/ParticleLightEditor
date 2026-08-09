#include "scanner.hpp"

#include "category.hpp"
#include "editor.hpp"
#include "reference.hpp"
#include "logger.hpp"
#include "settings.hpp"
#include "utility.hpp"
#include "vertices.hpp"

namespace ParticleLightEditor
{
    RE::BSEffectShaderMaterial* Editor::CreateEditableMaterial(RE::BSGeometry& a_geometry, RE::FormID a_ownerFormID)
    {
        auto* shader = Utility::GetEffectShader(a_geometry);
        auto* material = shader ? shader->GetMaterial() : nullptr;
        if (!shader || !material) {
            return nullptr;
        }

        auto* uniqueMaterial = material->Create();
        if (!uniqueMaterial) {
            logger::warn("Could not create an isolated material for particle light {:08X}", a_ownerFormID);
            return nullptr;
        }

        uniqueMaterial->CopyMembers(material);
        shader->DoClearRenderPasses();
        shader->SetMaterial(uniqueMaterial, true);
        a_geometry.SetMaterialNeedsUpdate(true);
        shader->SetupGeometry(&a_geometry);
        material = shader->GetMaterial();
        if (!material) {
            logger::warn("Could not assign an isolated material to particle light {:08X}", a_ownerFormID);
            return nullptr;
        }

        ReferenceManager::GetSingleton().RememberMaterial(&a_geometry, material);
        return material;
    }

    RE::BSEffectShaderMaterial* Editor::GetEditableMaterial(Entry& a_entry)
    {
        auto* geometry = a_entry.geometry.get();
        auto* shader = geometry ? Utility::GetEffectShader(*geometry) : nullptr;
        auto* material = shader ? shader->GetMaterial() : nullptr;
        if (!geometry || !shader || !material) {
            return nullptr;
        }

        if (!a_entry.editableMaterial) {
            a_entry.editableMaterial = ReferenceManager::GetSingleton().FindMaterial(geometry);
        }
        if (material == a_entry.editableMaterial) {
            return material;
        }

        if (a_entry.editableMaterial) {
            logger::debug("Particle-light material changed for reference {:08X}; rebinding the editable material", a_entry.ownerFormID);
        }

        // Effect materials are shared by equivalent instances. Each edited geometry
        // then needs its own material, and its render passes must be rebuilt immediately
        // so ENB observes the replacement without requiring a game restart.
        // This fixed a bug where exit to main menu then coc wouldn't apply saved edits.
        a_entry.editableMaterial = CreateEditableMaterial(*geometry, a_entry.ownerFormID);
        return a_entry.editableMaterial;
    }

    bool Editor::Apply(Entry& a_entry, Edit& a_edit)
    {
        auto* geometry = a_entry.geometry.get();
        if (!geometry) {
            return false;
        }

        bool materialChanged = false;
        if (a_edit.colorChanged || a_edit.intensityChanged) {
            auto* material = GetEditableMaterial(a_entry);
            if (!material) {
                return false;
            }

            auto materialColor = a_edit.color;
            if (a_edit.defaults.usesVertexColors) {
                materialColor = a_edit.defaults.materialColor;
                materialColor.alpha = a_edit.color.alpha;
                if (a_edit.colorChanged) {
                    auto* triShape = geometry->AsTriShape();
                    if (!triShape || !Vertices::Manager::GetSingleton().Apply(*triShape, a_edit.color, a_entry.ownerFormID)) {
                        return false;
                    }
                }
            }
            if (a_edit.colorChanged && !Utility::ColorsEqual(material->baseColor, materialColor)) {
                material->baseColor = materialColor;
                materialChanged = true;
            }
            if (a_edit.intensityChanged && material->baseColorScale != a_edit.intensity) {
                material->baseColorScale = a_edit.intensity;
                materialChanged = true;
            }
        }

        if (materialChanged) {
            geometry->SetMaterialNeedsUpdate(true);
        }
        if (materialChanged && a_edit.defaults.usesVertexColors) {
            if (auto* shader = Utility::GetEffectShader(*geometry)) {
                shader->DoClearRenderPasses();
                shader->SetupGeometry(geometry);
            }
        }
        if (a_edit.colorChanged) {
            a_entry.currentColor = a_edit.color;
        }

        bool transformChanged = false;
        if (a_edit.radiusChanged && a_edit.defaults.radius > 0.0F) {
            const auto scale = a_edit.defaults.local.scale * (a_edit.radius / a_edit.defaults.radius);
            if (std::isfinite(scale) && scale > 0.0001F && scale <= 10000.0F && geometry->local.scale != scale) {
                geometry->local.scale = scale;
                transformChanged = true;
            }
        }
        if (a_edit.positionChanged && !Utility::PointsEqual(geometry->local.translate, a_edit.localPosition)) {
            geometry->local.translate = a_edit.localPosition;
            transformChanged = true;
        }

        if (transformChanged) {
            UpdateTransform(*geometry);
        }
        return true;
    }

    bool Editor::Restore(Entry& a_entry, const Edit& a_edit)
    {
        auto* geometry = a_entry.geometry.get();
        if (!geometry) {
            return false;
        }

        if (a_edit.defaults.hasMaterial) {
            auto* material = GetEditableMaterial(a_entry);
            if (!material) {
                return false;
            }
            material->baseColor = a_edit.defaults.materialColor;
            material->baseColorScale = a_edit.defaults.intensity;
            geometry->SetMaterialNeedsUpdate(true);
        }

        if (a_edit.defaults.usesVertexColors) {
            if (auto* triShape = geometry->AsTriShape()) {
                Vertices::Manager::GetSingleton().Restore(*triShape, a_entry.ownerFormID);
            }
        }

        a_entry.currentColor = a_edit.defaults.color;
        geometry->local = a_edit.defaults.local;
        UpdateTransform(*geometry);
        return true;
    }

    size_t Scanner::GetSelectedLightIndex() const
    {
        const auto selected = std::ranges::find(editorIndices, selectedIndex);
        return selected != editorIndices.end() ? static_cast<size_t>(std::distance(editorIndices.begin(), selected)) : (std::numeric_limits<size_t>::max)();
    }

    std::string Scanner::GetLightLabel(size_t a_index) const
    {
        if (a_index >= editorIndices.size()) {
            return "<invalid particle light>";
        }

        const auto& entry = entries[editorIndices[a_index]];
        const auto& objectEditorID = entry.baseEditorID.empty() || entry.baseEditorID == "<unavailable>" ? entry.baseName : entry.baseEditorID;
        return std::format("{} - Particle Light {} [{:08X}]", objectEditorID.empty() || objectEditorID == "<unnamed>" ? "Unnamed Light Fixture" : objectEditorID,
            entry.particleOrdinal, entry.ownerFormID);
    }

    bool Scanner::SelectLight(size_t a_index)
    {
        if (a_index >= editorIndices.size()) {
            return false;
        }

        selectedIndex = editorIndices[a_index];
        targetCategory = entries[selectedIndex].category;
        return true;
    }

    size_t Scanner::SelectReference(RE::TESObjectREFR* a_reference)
    {
        const auto formID = a_reference ? a_reference->GetFormID() : 0;
        if (formID == 0) {
            return 0;
        }

        size_t firstMatch = (std::numeric_limits<size_t>::max)();
        size_t matchCount = 0;
        for (const auto index : editorIndices) {
            if (entries[index].ownerFormID == formID) {
                if (firstMatch == (std::numeric_limits<size_t>::max)()) {
                    firstMatch = index;
                }
                ++matchCount;
            }
        }

        if (matchCount == 0) {
            for (const auto index : editorIndices) {
                if (entries[index].associatedLightRefID == formID) {
                    if (firstMatch == (std::numeric_limits<size_t>::max)()) {
                        firstMatch = index;
                    }
                    ++matchCount;
                }
            }
        }

        if (matchCount == 0) {
            selectedIndex = (std::numeric_limits<size_t>::max)();
            logger::info("Console-selected reference {:08X} has no particle lights inside the current detection range", formID);
            return 0;
        }

        selectedIndex = firstMatch;
        targetCategory = entries[selectedIndex].category;
        logger::info("Selected particle light 1 of {} for console reference {:08X}", matchCount, formID);
        return matchCount;
    }

    void Scanner::SetParticleLightEdit(Entry& a_entry)
    {
        a_entry.editKey = Editor::GetKey(a_entry);
        const auto [iterator, inserted] = edits.try_emplace(a_entry.editKey);
        auto& edit = iterator->second;
        if (inserted || !edit.initialized) {
            edit.defaults = a_entry.defaults;
            if (!edit.colorChanged) {
                edit.color = a_entry.defaults.color;
            }
            if (!edit.intensityChanged) {
                edit.intensity = a_entry.defaults.intensity;
            }
            if (!edit.radiusChanged) {
                edit.radius = a_entry.defaults.radius;
            }
            if (!edit.positionChanged) {
                edit.localPosition = a_entry.defaults.local.translate;
            }
            edit.initialized = true;
        }
        else {
            a_entry.defaults = edit.defaults;
        }

        auto effectiveEdit = GetEffectiveEdit(a_entry);
        Editor::Apply(a_entry, effectiveEdit);
        ReferenceManager::GetSingleton().Capture(a_entry, edit);
    }

    void Scanner::ApplyParticleLightEdits()
    {
        for (auto& entry : entries) {
            if (FindEdit(entry)) {
                auto effectiveEdit = GetEffectiveEdit(entry);
                Editor::Apply(entry, effectiveEdit);
            }
        }
    }

    Edit Scanner::GetEffectiveEdit(const Entry& a_entry) const
    {
        const auto* baseEdit = FindEdit(a_entry);
        if (!baseEdit) {
            return {};
        }

        auto effective = *baseEdit;
        const auto applyRule = [&](const CategoryRuleKey& a_key) {
            const auto found = categoryRules.find(a_key);
            if (found == categoryRules.end()) {
                return;
            }

            const auto& rule = found->second;
            if (rule.colorChanged) {
                effective.color = rule.color;
                effective.colorChanged = true;
            }
            if (rule.intensityChanged) {
                effective.intensity = rule.intensity;
                effective.intensityChanged = true;
            }
            if (rule.radiusChanged && std::isfinite(rule.radiusScale) && rule.radiusScale > 0.0F) {
                effective.radius = effective.defaults.radius * rule.radiusScale;
                effective.radiusChanged = true;
            }
        };

        if (a_entry.category != ParticleCategory::kUnclassified) {
            applyRule({ a_entry.category, 0 });
            if (a_entry.cellFormID != 0) {
                applyRule({ a_entry.category, a_entry.cellFormID });
            }
        }
        return effective;
    }

    bool Scanner::GetSelectedEditorState(EditorState& a_state) const
    {
        const auto* entry = GetSelectedEntry();
        auto* geometry = entry ? entry->geometry.get() : nullptr;
        const auto* baseEdit = entry ? FindEdit(*entry) : nullptr;
        const auto* appearanceEntry = GetScopeRepresentative();
        auto* appearanceGeometry = appearanceEntry ? appearanceEntry->geometry.get() : nullptr;
        auto* appearanceShader = appearanceGeometry ? Utility::GetEffectShader(*appearanceGeometry) : nullptr;
        auto* appearanceMaterial = appearanceShader ? appearanceShader->GetMaterial() : nullptr;
        const auto* appearanceBaseEdit = appearanceEntry ? FindEdit(*appearanceEntry) : nullptr;
        if (!entry || !geometry || !baseEdit || !appearanceEntry || !appearanceGeometry || !appearanceMaterial || !appearanceBaseEdit) {
            return false;
        }
        const auto edit = GetEffectiveEdit(*appearanceEntry);

        a_state.nodeName = entry->nodeName;
        a_state.objectName = entry->baseName;
        a_state.baseEditorID = entry->baseEditorID;
        a_state.associatedLightEditorID = entry->associatedLightEditorID;
        a_state.associatedLightName = entry->associatedLightName;
        a_state.runtimeLightNodeName = entry->runtimeLightNodeName;
        a_state.ownerFormID = entry->ownerFormID;
        a_state.associatedLightRefID = entry->associatedLightRefID;
        a_state.color = edit.color;
        a_state.localPosition = baseEdit->positionChanged ? baseEdit->localPosition : geometry->local.translate;
        a_state.intensity = edit.intensityChanged ? edit.intensity : appearanceMaterial->baseColorScale;
        a_state.radius = edit.radiusChanged ? edit.radius : edit.defaults.radius;
        a_state.category = entry->category;
        return std::isfinite(a_state.radius) && a_state.radius > 0.0F;
    }

    bool Scanner::SetSelectedColor(const RE::NiColorA& a_color)
    {
        auto* entry = GetSelectedEntry();
        auto* edit = entry ? FindEdit(*entry) : nullptr;
        if (!entry || !edit || !std::isfinite(a_color.red) || !std::isfinite(a_color.green) || !std::isfinite(a_color.blue) || !std::isfinite(a_color.alpha)) {
            return false;
        }

        const RE::NiColorA color{ std::clamp(a_color.red, 0.0F, 10.0F), std::clamp(a_color.green, 0.0F, 10.0F), std::clamp(a_color.blue, 0.0F, 10.0F), std::clamp(a_color.alpha, 0.0F, 1.0F) };
        if (editScope != EditScope::kSelectedLight) {
            if (targetCategory == ParticleCategory::kUnclassified) {
                return false;
            }
            auto& rule = categoryRules[GetSelectedCategoryRuleKey()];
            rule.color = color;
            rule.colorChanged = true;
            ApplyParticleLightEdits();
            return true;
        }

        edit->color = color;
        edit->colorChanged = true;
        auto effectiveEdit = GetEffectiveEdit(*entry);
        const auto applied = Editor::Apply(*entry, effectiveEdit);
        ReferenceManager::GetSingleton().Capture(*entry, *edit);
        return applied;
    }

    bool Scanner::SetSelectedIntensity(float a_intensity)
    {
        auto* entry = GetSelectedEntry();
        auto* edit = entry ? FindEdit(*entry) : nullptr;
        if (!entry || !edit || !std::isfinite(a_intensity)) {
            return false;
        }

        const auto intensity = std::clamp(a_intensity, 0.0F, 100.0F);
        if (editScope != EditScope::kSelectedLight) {
            if (targetCategory == ParticleCategory::kUnclassified) {
                return false;
            }
            auto& rule = categoryRules[GetSelectedCategoryRuleKey()];
            rule.intensity = intensity;
            rule.intensityChanged = true;
            ApplyParticleLightEdits();
            return true;
        }

        edit->intensity = intensity;
        edit->intensityChanged = true;
        auto effectiveEdit = GetEffectiveEdit(*entry);
        const auto applied = Editor::Apply(*entry, effectiveEdit);
        ReferenceManager::GetSingleton().Capture(*entry, *edit);
        return applied;
    }

    bool Scanner::SetSelectedRadius(float a_radius)
    {
        auto* entry = GetSelectedEntry();
        auto* edit = entry ? FindEdit(*entry) : nullptr;
        if (!entry || !edit || !std::isfinite(a_radius) || a_radius <= 0.0F) {
            return false;
        }

        const auto* radiusEntry = editScope == EditScope::kSelectedLight ? entry : GetScopeRepresentative();
        const auto* radiusEdit = radiusEntry ? FindEdit(*radiusEntry) : nullptr;
        if (!radiusEdit || !std::isfinite(radiusEdit->defaults.radius) || radiusEdit->defaults.radius <= 0.0F) {
            return false;
        }

        const auto scale = radiusEdit->defaults.local.scale * (a_radius / radiusEdit->defaults.radius);
        if (!std::isfinite(scale) || scale <= 0.0001F || scale > 10000.0F) {
            return false;
        }

        if (editScope != EditScope::kSelectedLight) {
            if (targetCategory == ParticleCategory::kUnclassified) {
                return false;
            }
            auto& rule = categoryRules[GetSelectedCategoryRuleKey()];
            rule.radiusScale = a_radius / radiusEdit->defaults.radius;
            rule.radiusChanged = true;
            ApplyParticleLightEdits();
            return true;
        }

        edit->radius = a_radius;
        edit->radiusChanged = true;
        auto effectiveEdit = GetEffectiveEdit(*entry);
        const auto applied = Editor::Apply(*entry, effectiveEdit);
        ReferenceManager::GetSingleton().Capture(*entry, *edit);
        return applied;
    }

    bool Scanner::SetSelectedLocalPosition(const RE::NiPoint3& a_position)
    {
        auto* entry = GetSelectedEntry();
        auto* edit = entry ? FindEdit(*entry) : nullptr;
        if (!entry || !edit || !std::isfinite(a_position.x) || !std::isfinite(a_position.y) || !std::isfinite(a_position.z)) {
            return false;
        }

        edit->localPosition = a_position;
        edit->positionChanged = true;
        auto effectiveEdit = GetEffectiveEdit(*entry);
        const auto applied = Editor::Apply(*entry, effectiveEdit);
        ReferenceManager::GetSingleton().Capture(*entry, *edit);
        return applied;
    }

    bool Scanner::ResetSelectedLight()
    {
        auto* entry = GetSelectedEntry();
        auto* geometry = entry ? entry->geometry.get() : nullptr;
        auto* edit = entry ? FindEdit(*entry) : nullptr;
        if (!entry || !geometry || !edit) {
            return false;
        }

        if (!Editor::Restore(*entry, *edit)) {
            return false;
        }
        ReferenceManager::GetSingleton().Remove(*entry);
        edits.erase(entry->editKey);
        SetParticleLightEdit(*entry);
        logger::info("Reset particle light '{}' on reference {:08X}", entry->nodeName, entry->ownerFormID);
        return true;
    }

    bool Scanner::IsSelectedLightEdited() const
    {
        const auto* entry = GetSelectedEntry();
        const auto* edit = entry ? FindEdit(*entry) : nullptr;
        return edit && (edit->colorChanged || edit->intensityChanged || edit->radiusChanged || edit->positionChanged);
    }

    bool Scanner::SetEditScope(EditScope a_scope)
    {
        if (a_scope >= EditScope::kTotal) {
            return false;
        }
        editScope = a_scope;
        return true;
    }

    ParticleCategory Scanner::GetSelectedCategory() const
    {
        const auto* entry = GetSelectedEntry();
        return entry ? entry->category : ParticleCategory::kUnclassified;
    }

    bool Scanner::SetTargetCategory(ParticleCategory a_category)
    {
        if (!Category::IsValid(a_category)) {
            return false;
        }
        targetCategory = a_category;
        return true;
    }

    CategoryRuleKey Scanner::GetSelectedCategoryRuleKey() const
    {
        const auto* entry = GetSelectedEntry();
        if (!entry) {
            return {};
        }
        return { targetCategory, editScope == EditScope::kCategoryCell ? entry->cellFormID : 0 };
    }

    bool Scanner::MatchesSelectedScope(const Entry& a_entry) const
    {
        const auto* selected = GetSelectedEntry();
        if (!selected) {
            return false;
        }
        if (editScope == EditScope::kSelectedLight) {
            return &a_entry == selected;
        }
        return a_entry.category == targetCategory &&
            (editScope != EditScope::kCategoryCell || a_entry.cellFormID == selected->cellFormID);
    }

    const Entry* Scanner::GetScopeRepresentative() const
    {
        const auto* selected = GetSelectedEntry();
        if (!selected || editScope == EditScope::kSelectedLight || targetCategory == ParticleCategory::kUnclassified) {
            return selected;
        }

        const auto found = std::ranges::find_if(entries, [this](const Entry& a_entry) { return MatchesSelectedScope(a_entry); });
        return found != entries.end() ? &*found : selected;
    }

    size_t Scanner::GetAffectedLightCount() const
    {
        return static_cast<size_t>(std::ranges::count_if(entries, [this](const Entry& a_entry) { return MatchesSelectedScope(a_entry); }));
    }

    bool Scanner::RestoreEntryRuntime(Entry& a_entry)
    {
        const auto* edit = FindEdit(a_entry);
        return edit && Editor::Restore(a_entry, *edit);
    }

    bool Scanner::SetSelectedCategory(ParticleCategory a_category)
    {
        auto* selected = GetSelectedEntry();
        if (!selected || !Category::IsValid(a_category) || selected->categoryKey.empty()) {
            return false;
        }

        const auto key = selected->categoryKey;
        for (auto& entry : entries) {
            if (entry.categoryKey == key) {
                RestoreEntryRuntime(entry);
            }
        }

        if (a_category == Category::Classify(*selected)) {
            categoryOverrides.erase(key);
        }
        else {
            categoryOverrides[key] = a_category;
        }
        for (auto& entry : entries) {
            if (entry.categoryKey == key) {
                entry.category = Category::Resolve(entry, categoryOverrides);
            }
        }
        ApplyParticleLightEdits();
        return true;
    }

    bool Scanner::ResetSelectedScope()
    {
        if (editScope == EditScope::kSelectedLight) {
            return ResetSelectedLight();
        }

        const auto key = GetSelectedCategoryRuleKey();
        const auto found = categoryRules.find(key);
        if (found == categoryRules.end()) {
            return false;
        }

        for (auto& entry : entries) {
            if (MatchesSelectedScope(entry)) {
                RestoreEntryRuntime(entry);
            }
        }
        categoryRules.erase(found);
        ApplyParticleLightEdits();
        return true;
    }

    bool Scanner::IsSelectedScopeEdited() const
    {
        if (editScope == EditScope::kSelectedLight) {
            return IsSelectedLightEdited();
        }
        const auto found = categoryRules.find(GetSelectedCategoryRuleKey());
        return found != categoryRules.end() && (found->second.colorChanged || found->second.intensityChanged || found->second.radiusChanged);
    }

    void Scanner::UpdateEditorList(RE::PlayerCharacter* a_player)
    {
        editorIndices.clear();
        if (!a_player) {
            selectedIndex = (std::numeric_limits<size_t>::max)();
            return;
        }

        const auto& settings = Settings::GetSettings();
        const auto range = (std::max)(0.0F, settings.drawRange);
        const auto rangeSquared = range * range;
        const auto playerPosition = a_player->GetPosition();
        editorIndices.reserve(entries.size());

        for (size_t index = 0; index < entries.size(); ++index) {
            // Player attachment bounds are maintained in actor/skeleton space and
            // are not guaranteed to be comparable to a placed object's world
            // position. An equipped light is inherently in range of the player.
            if (entries[index].runtimeAttachment) {
                editorIndices.push_back(index);
                continue;
            }

            auto* geometry = entries[index].geometry.get();
            if (!geometry) {
                continue;
            }

            const auto distanceSquared = Utility::DistanceSquared(geometry->worldBound.center, playerPosition);
            if (std::isfinite(distanceSquared) && distanceSquared <= rangeSquared) {
                editorIndices.push_back(index);
            }
        }

        if (std::ranges::find(editorIndices, selectedIndex) == editorIndices.end()) {
            selectedIndex = editorIndices.empty() ? (std::numeric_limits<size_t>::max)() : editorIndices.front();
            targetCategory = selectedIndex < entries.size() ? entries[selectedIndex].category : ParticleCategory::kUnclassified;
        }
    }
}
