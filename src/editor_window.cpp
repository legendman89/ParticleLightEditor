#include "color.hpp"
#include "editor_window.hpp"
#include "logger.hpp"
#include "settings.hpp"

namespace ParticleLightEditor::Menu
{
    void RenderSelection()
    {
        auto& scanner = Scanner::GetSingleton();
        auto& state = GetState();
        size_t resultCount = 0;
        for (size_t index = 0; index < scanner.GetLightCount(); ++index) {
            if (SelectionMatchesFilter(scanner, state, index)) {
                ++resultCount;
            }
        }

        GUI::SetNextItemWidth(420.0F);
        GUI::InputText("Filter##ParticleLightFilter", state.lightFilter.data(), state.lightFilter.size());
        Controls::Tooltip("Search the object text or Form ID shown in the list.");
        GUI::SameLine(0.0F, 12.0F);
        GUI::Checkbox("Edited only##ParticleLightFilter", &state.editedLightsOnly);
        GUI::SameLine(0.0F, 18.0F);
        GUI::Text("Found %llu of %llu", resultCount, scanner.GetLightCount());

        GUI::Spacing();

        const auto selectedIndex = scanner.GetSelectedLightIndex();
        const auto selectedLabel = scanner.GetLightLabel(selectedIndex);
        constexpr auto spacing = 8.0F;
        const auto editWidth = Controls::EditActionButtonWidth("Edit Selected Light");
        GUI::ImVec2 available{};
        GUI::GetContentRegionAvail(&available);
        GUI::SetNextItemWidth((std::max)(160.0F, available.x - editWidth - spacing));
        const auto* style = GUI::GetStyle();
        const auto comboPaddingX = style ? style->FramePadding.x : 4.0F;
        const auto comboPaddingY = (std::max)(0.0F, (Controls::kEditActionButtonHeight - GUI::GetFontSize()) * 0.5F);
        GUI::PushStyleVar(GUI::ImGuiStyleVar_FramePadding, GUI::ImVec2{ comboPaddingX, comboPaddingY });
        if (GUI::BeginCombo("##SelectedParticleLight", selectedLabel.c_str())) {
            for (size_t index = 0; index < scanner.GetLightCount(); ++index) {
                if (!SelectionMatchesFilter(scanner, state, index)) {
                    continue;
                }
                const auto label = scanner.GetLightLabel(index);
                const auto itemLabel = std::format("{}##ParticleLight{}", label, index);
                const auto selected = index == selectedIndex;
                if (GUI::Selectable(itemLabel.c_str(), selected)) {
                    scanner.SelectLight(index);
                    state.consoleStatus.clear();
                    ClearEditorStatus();
                }
                if (selected) {
                    GUI::SetItemDefaultFocus();
                }
            }
            GUI::EndCombo();
        }
        GUI::PopStyleVar();
        GUI::SameLine(0.0F, spacing);
        if (Controls::EditActionButton("Edit Selected Light", selectedIndex < scanner.GetLightCount())) {
            const auto editorIsOpen = SetEditorWindowOpen(true);
            logger::debug(
                "Editor opening requested from the selection panel; editorRegistered={}, editorIsOpen={}",
                state.editorWindow != nullptr,
                editorIsOpen);
        }
    }

    void RenderEditorControls(const EditorState& a_editor)
    {
        auto& scanner = Scanner::GetSingleton();
        RenderClipboardActions(a_editor);

        GUI::Spacing();

        if (GUI::CollapsingHeader("Edit Scope", GUI::ImGuiTreeNodeFlags_DefaultOpen)) {
            GUI::Spacing();
            constexpr const char* scopes[]{ "This light", "Same category in current cell", "Same category everywhere" };
            auto scope = static_cast<int>(scanner.GetEditScope());
            auto category = static_cast<int>(scanner.GetTargetCategory());
            constexpr auto tableFlags = GUI::ImGuiTableFlags_SizingStretchSame | GUI::ImGuiTableFlags_NoSavedSettings;
            if (GUI::BeginTable("EditScopeTable", 2, tableFlags)) {
                GUI::TableSetupColumn("Scope", GUI::ImGuiTableColumnFlags_WidthStretch);
                GUI::TableSetupColumn("Category", GUI::ImGuiTableColumnFlags_WidthStretch);
                GUI::TableNextRow();
                GUI::TableNextColumn();
                GUI::TextUnformatted("Apply changes to");
                GUI::TableNextColumn();
                GUI::TextUnformatted("Target category");
                GUI::TableNextRow();
                GUI::TableNextColumn();
                GUI::SetNextItemWidth(-1.0F);
                if (GUI::Combo("##EditScope", &scope, scopes, static_cast<int>(std::size(scopes)))) {
                    scanner.SetEditScope(static_cast<EditScope>(scope));
                    ClearEditorStatus();
                }
                Controls::Tooltip("Category scopes affect color, opacity, intensity, and proportional radius. Local position and enabled state always affect only this light.");
                GUI::TableNextColumn();
                GUI::SetNextItemWidth(-1.0F);
                if (GUI::Combo("##TargetCategory", &category, Category::kNames, static_cast<int>(std::size(Category::kNames)))) {
                    scanner.SetTargetCategory(static_cast<ParticleCategory>(category));
                    ClearEditorStatus();
                }
                Controls::Tooltip("Choose which category the current scope edits. This does not change the selected mesh's category.");
                GUI::EndTable();
            }

            GUI::Spacing();

            const auto targetCategory = scanner.GetTargetCategory();
            if (targetCategory != ParticleCategory::kUnclassified && targetCategory != a_editor.category) {
                if (Controls::CTAButton("Assign Selected Mesh to Target", true)) {
                    scanner.SetSelectedCategory(targetCategory);
                    GetEditorWindowState().status = "Assigned the selected mesh to the target category.";
                }
                Controls::Tooltip("Explicitly reclassify every instance using the selected mesh. Use this only when automatic classification is wrong.");
            }

            if (scanner.GetEditScope() != EditScope::kSelectedLight && targetCategory == ParticleCategory::kUnclassified) {
                GUI::TextWrapped("Choose a target category before using a category scope.");
            }
            else {
                GUI::Text("Affected particle lights in the current cache: %llu", scanner.GetAffectedLightCount());
            }
        }

        GUI::Spacing();
        GUI::Spacing();

        auto enabled = a_editor.enabled;
        if (GUI::CollapsingHeader("Appearance", GUI::ImGuiTreeNodeFlags_DefaultOpen)) {
            GUI::Spacing();
            if (BeginPropertyTable("AppearanceProperties")) {
                BeginPropertyRow("Particle light enabled");
                if (GUI::Checkbox("Enabled##ParticleLightEnabled", &enabled)) {
                    scanner.SetSelectedEnabled(enabled);
                    GetEditorWindowState().status = enabled ? "Particle light enabled." : "Particle light disabled; appearance and geometry controls are locked.";
                }
                Controls::Tooltip("Show or hide only the selected particle-light geometry. It remains available in the editor while disabled.");
                RenderPropertyReset(EditProperty::kEnabled, "ResetEnabled");

                if (!enabled) {
                    GUI::BeginDisabled();
                }

                BeginPropertyRow("Color and opacity");
                std::array color{ a_editor.color.red, a_editor.color.green, a_editor.color.blue, a_editor.color.alpha };
                auto opacity = a_editor.color.alpha;
                constexpr auto colorFlags = GUI::ImGuiColorEditFlags_DisplayRGB | GUI::ImGuiColorEditFlags_NoAlpha;
                GUI::SetNextItemWidth(330.0F);
                const auto colorChanged = GUI::ColorEdit4("##ParticleColor", color.data(), colorFlags);
                GUI::SameLine(0.0F, 14.0F);
                GUI::SetNextItemWidth(-1.0F);
                const auto opacityChanged = GUI::SliderFloat("##ParticleOpacity", &opacity, 0.0F, 1.0F, "Opacity %.2f");
                if (colorChanged || opacityChanged) {
                    scanner.SetSelectedColor({ color[0], color[1], color[2], opacity });
                    GetEditorWindowState().status = "Color and opacity updated.";
                }

                RenderPropertyReset(EditProperty::kColor, "ResetColor", 12.0f);

                BeginPropertyRow("Intensity");
                auto intensity = a_editor.intensity;
                GUI::SetNextItemWidth(-1.0F);
                if (GUI::SliderFloat("##ParticleIntensity", &intensity, 0.0F, 10.0F, "%.3f")) {
                    scanner.SetSelectedIntensity(intensity);
                    GetEditorWindowState().status = "Intensity updated.";
                }
                RenderPropertyReset(EditProperty::kIntensity, "ResetIntensity", 12.0f);

                if (!enabled) {
                    GUI::EndDisabled();
                }
                GUI::EndTable();
            }
        }

        GUI::Spacing();
        GUI::Spacing();

        if (!enabled) {
            GUI::BeginDisabled();
        }
        if (GUI::CollapsingHeader("Geometry", GUI::ImGuiTreeNodeFlags_DefaultOpen)) {
            GUI::Spacing();
            if (BeginPropertyTable("GeometryProperties")) {
                BeginPropertyRow("Particle radius");
                auto radius = a_editor.radius;
                GUI::SetNextItemWidth(-1.0F);
                if (GUI::SliderFloat("##ParticleRadius", &radius, 1.0F, 4096.0F, "%.1f")) {
                    scanner.SetSelectedRadius(radius);
                    GetEditorWindowState().status = "Particle radius updated.";
                }
                Controls::Tooltip("Category scopes preserve each light's relative size by applying the selected light's radius ratio.");
                RenderPropertyReset(EditProperty::kRadius, "ResetRadius", 12.0f);

                BeginPropertyRow("Local position");
                std::array position{ a_editor.localPosition.x, a_editor.localPosition.y, a_editor.localPosition.z };
                GUI::SetNextItemWidth(-1.0F);
                if (GUI::DragFloat3("##ParticlePosition", position.data(), 1.0F, -4096.0F, 4096.0F, "%.2f")) {
                    scanner.SetSelectedLocalPosition({ position[0], position[1], position[2] });
                    GetEditorWindowState().status = "Local position updated for this light.";
                }
                Controls::Tooltip("Local position always changes only the selected light because different meshes use different pivots.");
                RenderPropertyReset(EditProperty::kPosition, "ResetPosition", 12.0f);
                GUI::EndTable();
            }
        }
        if (!enabled) {
            GUI::EndDisabled();
        }
    }

    void __stdcall RenderEditorWindow()
    {
        if (!IsEditorWindowOpen()) {
            return;
        }

        Controls::CenterNextWindow();
        GUI::SetNextWindowSize(GUI::ImVec2{ 920.0F, 840.0F }, GUI::ImGuiCond_Appearing);
        GUI::PushStyleColor(GUI::ImGuiCol_WindowBg, Color::kEditorBackground);
        auto editorOpen = true;
        const auto opened = GUI::Begin("Particle Light Editor###ParticleLightEditorWindow", &editorOpen, 0);
        GUI::PopStyleColor();
        Controls::WindowTitleIcon("Particle Light Editor", Icons::kLightbulb);
        if (!editorOpen) {
            SetEditorWindowOpen(false);
            logger::debug("Particle Light Editor window closed");
        }
        if (!opened) {
            GUI::End();
            return;
        }

        auto& scanner = Scanner::GetSingleton();
        if (scanner.GetLightCount() == 0) {
            GUI::Text("No editable particle lights are inside the current detection range.");
            GUI::End();
            return;
        }

        EditorState editor;
        const auto editorAvailable = scanner.GetSelectedEditorState(editor);
        if (editorAvailable) {
            RenderIdentity(editor);
            GUI::Spacing();
            RenderEditorControls(editor);
            HandleEditorShortcuts(editor);
        }
        else {
            GUI::Text("The selected particle geometry is no longer editable.");
        }

        Controls::SpacedSeparator();

        constexpr auto spacing = 8.0F;
        const auto resetLabel = scanner.GetEditScope() == EditScope::kSelectedLight ? "Reset Light" : "Reset Scope";
        const auto resetWidth = Controls::IconCTAButtonWidth(resetLabel, Icons::kReset);
        const auto saveWidth = Controls::IconCTAButtonWidth("Save Changes", Icons::kSave);
        auto& state = GetEditorWindowState();
        const auto defaultStatus = editorAvailable && !editor.enabled ? "Appearance and geometry edits are unavailable while the light is disabled." : "Per-property reset uses the active edit scope.";
        GUI::TextColored(Color::kEditActionText, "%s", state.status.empty() ? defaultStatus : state.status.c_str());
        GUI::SameLine(0.0F, spacing);
        Controls::AlignActions(resetWidth + saveWidth + spacing);
        if (Controls::IconCTAButton(resetLabel, scanner.IsSelectedScopeEdited(), Icons::kReset)) {
            const auto target = scanner.GetEditScope() == EditScope::kSelectedLight ? "Light" : "Scope";
            state.status = scanner.ResetSelectedScope() ? std::format("{} restored.", target) : std::format("Could not restore {}.", target);
        }
        const auto resetTooltip = scanner.GetEditScope() == EditScope::kSelectedLight ? "Restore the selected particle light to its original runtime values." : "Remove the active category rule and reveal any more-specific saved edits.";
        Controls::Tooltip(resetTooltip);
        GUI::SameLine(0.0F, spacing);
        if (Controls::IconCTAButton("Save Changes", Settings::AreEditsDirty(), Icons::kSave)) {
            state.status = Settings::Save() ? "Changes saved to disk." : "Could not save changes to disk.";
        }
        Controls::Tooltip("Save every edited particle light and the current menu options.");
        GUI::End();
    }
}
