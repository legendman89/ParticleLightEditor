#include "scanner.hpp"

#include "animation.hpp"
#include "category.hpp"
#include "editor.hpp"
#include "reference.hpp"
#include "logger.hpp"
#include "settings.hpp"
#include "translate.hpp"
#include "utility.hpp"
#include "vertices.hpp"

#define ANIMATION_FLOAT_CLAMP(NAME, DEFAULT_VALUE, MINIMUM, MAXIMUM) animation.NAME = std::clamp(animation.NAME, MINIMUM, MAXIMUM);
#define ANIMATION_COLOR_CLAMP(NAME, RED, GREEN, BLUE, ALPHA) animation.NAME = { std::clamp(animation.NAME.red, 0.0F, 10.0F), std::clamp(animation.NAME.green, 0.0F, 10.0F), std::clamp(animation.NAME.blue, 0.0F, 10.0F), std::clamp(animation.NAME.alpha, 0.0F, 1.0F) };
#define APPLY_SCOPE_PROPERTY(PROPERTY, NAME, CHANGED, COMPARISON, LABEL) APPLY_SCOPE_PROPERTY_##PROPERTY(NAME, CHANGED)
#define APPLY_SCOPE_PROPERTY_kColor(NAME, CHANGED) if (rule.CHANGED && rule.NAME##Revision >= a_edit.colorRevision) { a_edit.color = rule.NAME; a_edit.colorChanged = true; a_edit.colorRevision = rule.NAME##Revision; }
#define APPLY_SCOPE_PROPERTY_kIntensity(NAME, CHANGED) if (rule.CHANGED && rule.NAME##Revision >= a_edit.intensityRevision) { a_edit.intensity = rule.NAME; a_edit.intensityChanged = true; a_edit.intensityRevision = rule.NAME##Revision; }
#define APPLY_SCOPE_PROPERTY_kRadius(NAME, CHANGED) if (rule.CHANGED && rule.NAME##Revision >= a_edit.radiusRevision && std::isfinite(rule.NAME) && rule.NAME > 0.0F) { a_edit.radius = a_edit.defaults.radius * rule.NAME; a_edit.radiusChanged = true; a_edit.radiusRevision = rule.NAME##Revision; }
#define APPLY_SCOPE_PROPERTY_kPosition(NAME, CHANGED) if (rule.CHANGED && rule.NAME##Revision >= a_edit.localPositionRevision) { a_edit.localPosition = rule.NAME; a_edit.positionChanged = true; a_edit.localPositionRevision = rule.NAME##Revision; }
#define APPLY_SCOPE_PROPERTY_kEnabled(NAME, CHANGED) if (rule.CHANGED && rule.NAME##Revision >= a_edit.enabledRevision) { a_edit.enabled = rule.NAME; a_edit.enabledChanged = true; a_edit.enabledRevision = rule.NAME##Revision; }

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

        if (a_edit.enabledChanged) {
            const auto shouldCull = !a_edit.enabled;
            if (geometry->GetAppCulled() != shouldCull) {
                geometry->SetAppCulled(shouldCull);
            }
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
        const auto shouldCull = !a_edit.defaults.enabled;
        if (geometry->GetAppCulled() != shouldCull) {
            geometry->SetAppCulled(shouldCull);
        }
        geometry->local = a_edit.defaults.local;
        UpdateTransform(*geometry);
        return true;
    }

    size_t Scanner::GetSelectedLightIndex() const
    {
        const auto selected = std::ranges::find(editorIndices, selectedIndex);
        return selected != editorIndices.end() ? static_cast<size_t>(std::distance(editorIndices.begin(), selected)) : std::numeric_limits<size_t>::max();
    }

    std::string Scanner::GetLightLabel(size_t a_index) const
    {
        if (a_index >= editorIndices.size()) {
            return Trans::Tr("Selection.InvalidLight");
        }

        const auto& entry = entries[editorIndices[a_index]];
        const auto& objectEditorID = entry.baseEditorID.empty() || entry.baseEditorID == "Unavailable" ? entry.baseName : entry.baseEditorID;
        const auto label = Utility::BeautifyLabel(objectEditorID.empty() || objectEditorID == "Unnamed" ? Trans::Tr("Selection.UnnamedFixture") : objectEditorID);
        return std::format("{} [{:08X}]", label, entry.ownerFormID);
    }

    bool Scanner::LightMatchesFilter(size_t a_index, std::string_view a_filter) const
    {
        if (a_index >= editorIndices.size()) {
            return false;
        }

        const auto filter = Category::Lowercase(a_filter);
        if (filter.empty()) {
            return true;
        }

        return Category::Lowercase(GetLightLabel(a_index)).contains(filter);
    }

    bool Scanner::IsLightEdited(size_t a_index) const
    {
        if (a_index >= editorIndices.size()) {
            return false;
        }
        const auto& entry = entries[editorIndices[a_index]];
        return FindEdit(entry) && HasChanges(GetEffectiveEdit(entry));
    }

    bool Scanner::SelectLight(size_t a_index)
    {
        if (a_index >= editorIndices.size()) {
            return false;
        }

        selectedIndex = editorIndices[a_index];
        targetCategory = entries[selectedIndex].category;
        selectionCleared = false;
        return true;
    }

    size_t Scanner::SelectReference(RE::TESObjectREFR* a_reference)
    {
        const auto formID = a_reference ? a_reference->GetFormID() : 0;
        if (formID == 0) {
            return 0;
        }

        size_t firstMatch = std::numeric_limits<size_t>::max();
        size_t matchCount = 0;
        for (const auto index : editorIndices) {
            if (entries[index].ownerFormID == formID) {
                if (firstMatch == std::numeric_limits<size_t>::max()) {
                    firstMatch = index;
                }
                ++matchCount;
            }
        }

        if (matchCount == 0) {
            for (const auto index : editorIndices) {
                if (entries[index].associatedLightRefID == formID) {
                    if (firstMatch == std::numeric_limits<size_t>::max()) {
                        firstMatch = index;
                    }
                    ++matchCount;
                }
            }
        }

        if (matchCount == 0) {
            selectedIndex = std::numeric_limits<size_t>::max();
            targetCategory = ParticleCategory::kUnclassified;
            selectionCleared = true;
            logger::info("Console-selected reference {:08X} has no particle lights inside the current detection range", formID);
            return 0;
        }

        selectedIndex = firstMatch;
        targetCategory = entries[selectedIndex].category;
        selectionCleared = false;
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
            if (!edit.enabledChanged) {
                edit.enabled = a_entry.defaults.enabled;
            }
            if (!edit.animationChanged) {
                edit.animation = Animation::MakeDefault(a_entry.defaults, a_entry.category);
            }
            edit.initialized = true;
        }
        else {
            a_entry.defaults = edit.defaults;
        }

        auto effectiveEdit = GetEffectiveEdit(a_entry);
        Editor::Apply(a_entry, effectiveEdit);
        Animation::Apply(a_entry, effectiveEdit);
        ReferenceManager::GetSingleton().Capture(a_entry, edit);
    }

    void Scanner::ApplyParticleLightEdits()
    {
        for (auto& entry : entries) {
            if (FindEdit(entry)) {
                auto effectiveEdit = GetEffectiveEdit(entry);
                Editor::Apply(entry, effectiveEdit);
                Animation::Apply(entry, effectiveEdit);
            }
        }
    }

    void Scanner::ApplyCategoryRule(Edit& a_edit, const CategoryRuleKey& a_key) const
    {
        const auto found = categoryRules.find(a_key);
        if (found == categoryRules.end()) {
            return;
        }

        const auto& rule = found->second;
        FOREACH_CATEGORY_RULE_PROPERTY(APPLY_SCOPE_PROPERTY)
        if (rule.animationChanged && rule.animationRevision >= a_edit.animationRevision) {
            a_edit.animation = rule.animation;
            a_edit.animationChanged = true;
            a_edit.animationRevision = rule.animationRevision;
        }
    }

    void Scanner::ApplyBaseRule(Edit& a_edit, const BaseRuleKey& a_key) const
    {
        const auto found = baseRules.find(a_key);
        if (found == baseRules.end()) {
            return;
        }

        const auto& rule = found->second;
        FOREACH_SCOPE_EDIT_PROPERTY(APPLY_SCOPE_PROPERTY)
        if (rule.animationChanged && rule.animationRevision >= a_edit.animationRevision) {
            a_edit.animation = rule.animation;
            a_edit.animationChanged = true;
            a_edit.animationRevision = rule.animationRevision;
        }
    }

    Edit Scanner::GetEffectiveEdit(const Entry& a_entry) const
    {
        const auto* baseEdit = FindEdit(a_entry);
        if (!baseEdit) {
            return {};
        }

        auto effective = *baseEdit;

        if (a_entry.category != ParticleCategory::kUnclassified) {
            ApplyCategoryRule(effective, { a_entry.category, 0 });
            if (a_entry.cellFormID != 0) {
                ApplyCategoryRule(effective, { a_entry.category, a_entry.cellFormID });
            }
        }
        if (a_entry.baseFormID != 0) {
            ApplyBaseRule(effective, { a_entry.baseFormID, a_entry.particleOrdinal, 0 });
            if (a_entry.cellFormID != 0) {
                ApplyBaseRule(effective, { a_entry.baseFormID, a_entry.particleOrdinal, a_entry.cellFormID });
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
        const auto* appearanceBaseEdit = appearanceEntry ? FindEdit(*appearanceEntry) : nullptr;
        if (!entry || !geometry || !baseEdit || !appearanceEntry || !appearanceBaseEdit) {
            return false;
        }
        const auto edit = GetEffectiveEdit(*appearanceEntry);
        const auto selectedEdit = GetEffectiveEdit(*entry);

        a_state.nodeName = entry->nodeName;
        a_state.objectName = entry->baseName;
        a_state.baseEditorID = entry->baseEditorID;
        a_state.associatedLightEditorID = entry->associatedLightEditorID;
        a_state.associatedLightName = entry->associatedLightName;
        a_state.runtimeLightNodeName = entry->runtimeLightNodeName;
        a_state.ownerFormID = entry->ownerFormID;
        a_state.associatedLightRefID = entry->associatedLightRefID;
        a_state.color = edit.color;
        a_state.localPosition = selectedEdit.positionChanged ? selectedEdit.localPosition : geometry->local.translate;
        a_state.intensity = edit.intensity;
        a_state.radius = edit.radiusChanged ? edit.radius : edit.defaults.radius;
        a_state.defaultRadius = edit.defaults.radius;
        a_state.enabled = selectedEdit.enabledChanged ? selectedEdit.enabled : selectedEdit.defaults.enabled;
        a_state.category = entry->category;
        a_state.animation = edit.animation;
        a_state.animationEdited = edit.animationChanged;
        a_state.nativeAnimated = edit.defaults.animation.available;
        a_state.usesVertexColors = edit.defaults.usesVertexColors;
        return std::isfinite(a_state.radius) && a_state.radius > 0.0F;
    }

    bool Scanner::SetSelectedColor(const RE::NiColorA& a_color)
    {
        auto* entry = GetSelectedEntry();
        auto* edit = entry ? FindEdit(*entry) : nullptr;
        if (!entry || !edit || !Utility::IsFiniteColor(a_color)) {
            return false;
        }

        const RE::NiColorA color{ std::clamp(a_color.red, 0.0F, 10.0F), std::clamp(a_color.green, 0.0F, 10.0F), std::clamp(a_color.blue, 0.0F, 10.0F), std::clamp(a_color.alpha, 0.0F, 1.0F) };
        if (editScope != EditScope::kSelectedLight) {
            auto* rule = GetOrCreateSelectedScopeEdit();
            if (!rule) {
                return false;
            }
            rule->color = color;
            rule->colorChanged = true;
            rule->colorRevision = NextEditRevision();
            ApplyParticleLightEdits();
            return true;
        }

        edit->color = color;
        edit->colorChanged = true;
        edit->colorRevision = NextEditRevision();
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
            auto* rule = GetOrCreateSelectedScopeEdit();
            if (!rule) {
                return false;
            }
            rule->intensity = intensity;
            rule->intensityChanged = true;
            rule->intensityRevision = NextEditRevision();
            ApplyParticleLightEdits();
            return true;
        }

        edit->intensity = intensity;
        edit->intensityChanged = true;
        edit->intensityRevision = NextEditRevision();
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
            auto* rule = GetOrCreateSelectedScopeEdit();
            if (!rule) {
                return false;
            }
            rule->radiusScale = a_radius / radiusEdit->defaults.radius;
            rule->radiusChanged = true;
            rule->radiusScaleRevision = NextEditRevision();
            ApplyParticleLightEdits();
            return true;
        }

        edit->radius = a_radius;
        edit->radiusChanged = true;
        edit->radiusRevision = NextEditRevision();
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

        if (IsBaseScope(editScope)) {
            auto* rule = GetOrCreateSelectedScopeEdit();
            if (!rule) {
                return false;
            }
            rule->localPosition = a_position;
            rule->positionChanged = true;
            rule->localPositionRevision = NextEditRevision();
            ApplyParticleLightEdits();
            return true;
        }

        edit->localPosition = a_position;
        edit->positionChanged = true;
        edit->localPositionRevision = NextEditRevision();
        auto effectiveEdit = GetEffectiveEdit(*entry);
        const auto applied = Editor::Apply(*entry, effectiveEdit);
        ReferenceManager::GetSingleton().Capture(*entry, *edit);
        return applied;
    }

    bool Scanner::SetSelectedEnabled(bool a_enabled)
    {
        auto* entry = GetSelectedEntry();
        auto* edit = entry ? FindEdit(*entry) : nullptr;
        if (!entry || !edit) {
            return false;
        }

        if (IsBaseScope(editScope)) {
            auto* rule = GetOrCreateSelectedScopeEdit();
            if (!rule) {
                return false;
            }
            rule->enabled = a_enabled;
            rule->enabledChanged = true;
            rule->enabledRevision = NextEditRevision();
            ApplyParticleLightEdits();
            return true;
        }

        edit->enabled = a_enabled;
        edit->enabledChanged = true;
        edit->enabledRevision = NextEditRevision();
        auto effectiveEdit = GetEffectiveEdit(*entry);
        const auto applied = Editor::Apply(*entry, effectiveEdit);
        ReferenceManager::GetSingleton().Capture(*entry, *edit);
        return applied;
    }

    bool Scanner::SetSelectedAnimation(const AnimationEdit& a_animation)
    {
        auto* entry = GetSelectedEntry();
        auto* edit = entry ? FindEdit(*entry) : nullptr;
        if (!entry || !edit || !Animation::IsFinite(a_animation)) {
            return false;
        }

        auto animation = a_animation;
        FOREACH_ANIMATION_FLOAT_PROPERTY(ANIMATION_FLOAT_CLAMP)
        animation.maximumBrightness = std::max(animation.minimumBrightness, animation.maximumBrightness);
        FOREACH_ANIMATION_COLOR_PROPERTY(ANIMATION_COLOR_CLAMP)
        if (animation.profile == AnimationProfile::kOriginal && !edit->defaults.animation.available) {
            animation.profile = Animation::SuggestedProfile(entry->category);
        }

        if (editScope != EditScope::kSelectedLight) {
            auto* rule = GetOrCreateSelectedScopeEdit();
            if (!rule) {
                return false;
            }
            rule->animation = animation;
            rule->animationChanged = true;
            rule->animationRevision = NextEditRevision();
            ApplyParticleLightEdits();
            return true;
        }

        edit->animation = animation;
        edit->animationChanged = true;
        edit->animationRevision = NextEditRevision();
        auto effectiveEdit = GetEffectiveEdit(*entry);
        const auto applied = Editor::Apply(*entry, effectiveEdit) && Animation::Apply(*entry, effectiveEdit);
        ReferenceManager::GetSingleton().Capture(*entry, *edit);
        return applied;
    }

    bool Scanner::ResetSelectedAnimation()
    {
        auto* entry = GetSelectedEntry();
        auto* edit = entry ? FindEdit(*entry) : nullptr;
        if (!entry || !edit) {
            return false;
        }

        if (editScope == EditScope::kSelectedLight) {
            if (!edit->animationChanged) {
                return false;
            }
            Animation::Restore(*entry, *edit);
            edit->animationChanged = false;
            edit->animationRevision = 0;
            edit->animation = Animation::MakeDefault(edit->defaults, entry->category);
            auto effectiveEdit = GetEffectiveEdit(*entry);
            const auto applied = Editor::Apply(*entry, effectiveEdit) && Animation::Apply(*entry, effectiveEdit);
            ReferenceManager::GetSingleton().Capture(*entry, *edit);
            return applied;
        }

        auto* rule = FindSelectedScopeEdit();
        if (!rule || !rule->animationChanged) {
            return false;
        }
        for (auto& current : entries) {
            if (MatchesSelectedScope(current)) {
                const auto* currentEdit = FindEdit(current);
                if (currentEdit) {
                    Animation::Restore(current, *currentEdit);
                }
            }
        }
        rule->animationChanged = false;
        rule->animationRevision = 0;
        if (!HasChanges(*rule)) {
            if (IsBaseScope(editScope)) {
                baseRules.erase(GetSelectedBaseRuleKey());
            }
            else {
                categoryRules.erase(GetSelectedCategoryRuleKey());
            }
        }
        ApplyParticleLightEdits();
        return true;
    }

    bool Scanner::IsSelectedAnimationEdited() const
    {
        const auto* entry = GetSelectedEntry();
        const auto* edit = entry ? FindEdit(*entry) : nullptr;
        if (!edit) {
            return false;
        }
        if (editScope == EditScope::kSelectedLight) {
            return edit->animationChanged;
        }
        auto* rule = FindSelectedScopeEdit();
        return rule && rule->animationChanged;
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
        Animation::RestoreDefault(*entry, *edit);
        ReferenceManager::GetSingleton().Remove(*entry);
        edits.erase(entry->editKey);
        SetParticleLightEdit(*entry);
        logger::info("Reset particle light '{}' on reference {:08X}", entry->nodeName, entry->ownerFormID);
        return true;
    }

    bool Scanner::ResetSelectedProperty(EditProperty a_property)
    {
        auto* entry = GetSelectedEntry();
        auto* edit = entry ? FindEdit(*entry) : nullptr;
        if (!entry || !edit) {
            return false;
        }

        const auto selectedOnly = !ScopeSupportsProperty(editScope, a_property);
        if (selectedOnly) {
            if (!IsPropertyChanged(*edit, a_property) || !Editor::Restore(*entry, *edit)) {
                return false;
            }
            ClearProperty(*edit, a_property);
            auto effectiveEdit = GetEffectiveEdit(*entry);
            const auto applied = Editor::Apply(*entry, effectiveEdit);
            ReferenceManager::GetSingleton().Capture(*entry, *edit);
            return applied;
        }

        auto* rule = FindSelectedScopeEdit();
        if (!rule || !IsPropertyChanged(*rule, a_property)) {
            return false;
        }
        for (auto& current : entries) {
            if (MatchesSelectedScope(current)) {
                RestoreEntryRuntime(current);
            }
        }
        ClearProperty(*rule, a_property);
        if (!HasChanges(*rule)) {
            if (IsBaseScope(editScope)) {
                baseRules.erase(GetSelectedBaseRuleKey());
            }
            else {
                categoryRules.erase(GetSelectedCategoryRuleKey());
            }
        }
        ApplyParticleLightEdits();
        return true;
    }

    bool Scanner::IsSelectedLightEdited() const
    {
        const auto* entry = GetSelectedEntry();
        const auto* edit = entry ? FindEdit(*entry) : nullptr;
        return edit && HasChanges(*edit);
    }

    bool Scanner::IsSelectedPropertyEdited(EditProperty a_property) const
    {
        const auto* entry = GetSelectedEntry();
        const auto* edit = entry ? FindEdit(*entry) : nullptr;
        if (!edit) {
            return false;
        }
        if (!ScopeSupportsProperty(editScope, a_property)) {
            return IsPropertyChanged(*edit, a_property);
        }
        const auto* rule = FindSelectedScopeEdit();
        return rule && IsPropertyChanged(*rule, a_property);
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

    BaseRuleKey Scanner::GetSelectedBaseRuleKey() const
    {
        const auto* entry = GetSelectedEntry();
        if (!entry) {
            return {};
        }
        return { entry->baseFormID, entry->particleOrdinal, editScope == EditScope::kBaseCell ? entry->cellFormID : 0 };
    }

    ScopeEdit* Scanner::GetOrCreateSelectedScopeEdit()
    {
        if (IsBaseScope(editScope)) {
            const auto key = GetSelectedBaseRuleKey();
            return key.baseFormID != 0 ? &baseRules[key] : nullptr;
        }
        if (IsCategoryScope(editScope)) {
            const auto key = GetSelectedCategoryRuleKey();
            return key.category != ParticleCategory::kUnclassified ? &categoryRules[key] : nullptr;
        }
        return nullptr;
    }

    ScopeEdit* Scanner::FindSelectedScopeEdit()
    {
        if (IsBaseScope(editScope)) {
            const auto found = baseRules.find(GetSelectedBaseRuleKey());
            return found != baseRules.end() ? &found->second : nullptr;
        }
        if (IsCategoryScope(editScope)) {
            const auto found = categoryRules.find(GetSelectedCategoryRuleKey());
            return found != categoryRules.end() ? &found->second : nullptr;
        }
        return nullptr;
    }

    const ScopeEdit* Scanner::FindSelectedScopeEdit() const
    {
        if (IsBaseScope(editScope)) {
            const auto found = baseRules.find(GetSelectedBaseRuleKey());
            return found != baseRules.end() ? &found->second : nullptr;
        }
        if (IsCategoryScope(editScope)) {
            const auto found = categoryRules.find(GetSelectedCategoryRuleKey());
            return found != categoryRules.end() ? &found->second : nullptr;
        }
        return nullptr;
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
        if (IsBaseScope(editScope)) {
            return selected->baseFormID != 0 && a_entry.baseFormID == selected->baseFormID && a_entry.particleOrdinal == selected->particleOrdinal &&
                (editScope != EditScope::kBaseCell || a_entry.cellFormID == selected->cellFormID);
        }
        return a_entry.category == targetCategory && (editScope != EditScope::kCategoryCell || a_entry.cellFormID == selected->cellFormID);
    }

    const Entry* Scanner::GetScopeRepresentative() const
    {
        const auto* selected = GetSelectedEntry();
        if (!selected || editScope == EditScope::kSelectedLight || (IsCategoryScope(editScope) && targetCategory == ParticleCategory::kUnclassified)) {
            return selected;
        }

        for (const auto& entry : entries) {
            if (MatchesSelectedScope(entry)) {
                return &entry;
            }
        }
        return selected;
    }

    size_t Scanner::GetAffectedLightCount() const
    {
        size_t count = 0;
        for (const auto& entry : entries) {
            if (MatchesSelectedScope(entry)) {
                ++count;
            }
        }
        return count;
    }

    bool Scanner::RestoreEntryRuntime(Entry& a_entry)
    {
        const auto* edit = FindEdit(a_entry);
        if (!edit || !Editor::Restore(a_entry, *edit)) {
            return false;
        }
        Animation::RestoreDefault(a_entry, *edit);
        return true;
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

        const auto* rule = FindSelectedScopeEdit();
        if (!rule) {
            return false;
        }

        for (auto& entry : entries) {
            if (MatchesSelectedScope(entry)) {
                RestoreEntryRuntime(entry);
            }
        }
        if (IsBaseScope(editScope)) {
            baseRules.erase(GetSelectedBaseRuleKey());
        }
        else {
            categoryRules.erase(GetSelectedCategoryRuleKey());
        }
        ApplyParticleLightEdits();
        return true;
    }

    bool Scanner::IsSelectedScopeEdited() const
    {
        if (editScope == EditScope::kSelectedLight) {
            return IsSelectedLightEdited();
        }
        const auto* rule = FindSelectedScopeEdit();
        return rule && HasChanges(*rule);
    }

    void Scanner::UpdateEditorList(RE::PlayerCharacter* a_player)
    {
        editorIndices.clear();
        if (!a_player) {
            selectedIndex = std::numeric_limits<size_t>::max();
            return;
        }

        const auto& settings = Settings::GetSettings();
        const auto range = std::max(0.0F, settings.drawRange);
        const auto rangeSquared = range * range;
        const auto playerPosition = a_player->GetPosition();
        editorIndices.reserve(entries.size());
        std::vector<std::pair<float, size_t>> lightsByDistance;
        lightsByDistance.reserve(entries.size());

        for (size_t index = 0; index < entries.size(); ++index) {
            // An equipped light is in range of the player.
            if (entries[index].runtimeAttachment) {
                lightsByDistance.emplace_back(0.0F, index);
                continue;
            }

            auto* geometry = entries[index].geometry.get();
            if (!geometry) {
                continue;
            }

            const auto distanceSquared = Utility::DistanceSquared(geometry->worldBound.center, playerPosition);
            if (std::isfinite(distanceSquared) && distanceSquared <= rangeSquared) {
                lightsByDistance.emplace_back(distanceSquared, index);
            }
        }

        std::ranges::sort(lightsByDistance);
        for (const auto& light : lightsByDistance) {
            editorIndices.push_back(light.second);
        }

        if (std::ranges::find(editorIndices, selectedIndex) == editorIndices.end()) {
            selectedIndex = selectionCleared || editorIndices.empty() ? std::numeric_limits<size_t>::max() : editorIndices.front();
            targetCategory = selectedIndex < entries.size() ? entries[selectedIndex].category : ParticleCategory::kUnclassified;
        }
    }
}

#undef ANIMATION_FLOAT_CLAMP
#undef ANIMATION_COLOR_CLAMP
#undef APPLY_SCOPE_PROPERTY
#undef APPLY_SCOPE_PROPERTY_kColor
#undef APPLY_SCOPE_PROPERTY_kIntensity
#undef APPLY_SCOPE_PROPERTY_kRadius
#undef APPLY_SCOPE_PROPERTY_kPosition
#undef APPLY_SCOPE_PROPERTY_kEnabled
