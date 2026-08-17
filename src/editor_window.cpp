#include "animation_ui.hpp"
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
        const auto filterLabel = std::format("{}##ParticleLightFilter", Trans::Tr("Scanner.Filter"));
        GUI::InputTextWithHint(filterLabel.c_str(), Trans::Tr("Scanner.Filter.Hint").c_str(), state.lightFilter.data(), state.lightFilter.size());
        Controls::Tooltip(Trans::Tr("Scanner.Filter.Tooltip").c_str());
        GUI::SameLine(0.0F, 18.0F);
        const auto editedOnlyLabel = std::format("{}##ParticleLightFilter", Trans::Tr("Scanner.Filter.EditedOnly"));
        GUI::Checkbox(editedOnlyLabel.c_str(), &state.editedLightsOnly);

        GUI::Spacing();

        GUI::TextUnformatted(Trans::Format("Scanner.Filter.Count", resultCount, scanner.GetLightCount()).c_str());

        GUI::Spacing();

        const auto selectedIndex = scanner.GetSelectedLightIndex();
        const auto selectedLabel = scanner.GetLightLabel(selectedIndex);
        constexpr auto spacing = 8.0F;
        const auto& editLabel = Trans::Tr("Scanner.EditSelected");
        const auto editWidth = Controls::EditActionButtonWidth(editLabel.c_str());
        const auto available = GUI::GetContentRegionAvail();
        GUI::SetNextItemWidth(std::max(160.0F, available.x - editWidth - spacing));
        const auto* style = GUI::GetStyle();
        const auto comboPaddingX = style ? style->FramePadding.x : 4.0F;
        const auto comboPaddingY = std::max(0.0F, (Controls::kEditActionButtonHeight - GUI::GetFontSize()) * 0.5F);
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
        if (Controls::EditActionButton(editLabel.c_str(), selectedIndex < scanner.GetLightCount())) {
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

        if (GUI::CollapsingHeader(Trans::Tr("Editor.Scope.Header").c_str(), GUI::ImGuiTreeNodeFlags_DefaultOpen)) {
            GUI::Spacing();
            const std::array scopes{
                Trans::Tr("Editor.Scope.ThisLight").c_str(),
                Trans::Tr("Editor.Scope.BaseCell").c_str(),
                Trans::Tr("Editor.Scope.BaseGlobal").c_str(),
                Trans::Tr("Editor.Scope.CategoryCell").c_str(),
                Trans::Tr("Editor.Scope.CategoryGlobal").c_str()
            };
            const auto categoryNames = Category::DisplayNames();
            auto scope = static_cast<int>(scanner.GetEditScope());
            auto category = static_cast<int>(scanner.GetTargetCategory());
            const auto categoryScope = IsCategoryScope(scanner.GetEditScope());
            constexpr auto tableFlags = GUI::ImGuiTableFlags_SizingStretchSame | GUI::ImGuiTableFlags_NoSavedSettings;
            if (GUI::BeginTable("EditScopeTable", categoryScope ? 2 : 1, tableFlags)) {
                GUI::TableSetupColumn("##ScopeColumn", GUI::ImGuiTableColumnFlags_WidthStretch);
                if (categoryScope) {
                    GUI::TableSetupColumn("##CategoryColumn", GUI::ImGuiTableColumnFlags_WidthStretch);
                }
                GUI::TableNextRow();
                GUI::TableNextColumn();
                GUI::TextUnformatted(Trans::Tr("Editor.Scope.ApplyTo").c_str());
                if (categoryScope) {
                    GUI::TableNextColumn();
                    GUI::TextUnformatted(Trans::Tr("Editor.Scope.TargetCategory").c_str());
                }
                GUI::TableNextRow();
                GUI::TableNextColumn();
                GUI::SetNextItemWidth(-1.0F);
                if (GUI::Combo("##EditScope", &scope, scopes.data(), static_cast<int>(scopes.size()))) {
                    scanner.SetEditScope(static_cast<EditScope>(scope));
                    ClearEditorStatus();
                }
                Controls::Tooltip(Trans::Tr("Editor.Scope.Tooltip").c_str());
                if (categoryScope) {
                    GUI::TableNextColumn();
                    GUI::SetNextItemWidth(-1.0F);
                    if (GUI::Combo("##TargetCategory", &category, categoryNames.data(), static_cast<int>(categoryNames.size()))) {
                        scanner.SetTargetCategory(static_cast<ParticleCategory>(category));
                        ClearEditorStatus();
                    }
                    Controls::Tooltip(Trans::Tr("Editor.Scope.Category.Tooltip").c_str());
                }
                GUI::EndTable();
            }

            GUI::Spacing();

            const auto targetCategory = scanner.GetTargetCategory();
            if (categoryScope && targetCategory != ParticleCategory::kUnclassified && targetCategory != a_editor.category) {
                if (Controls::CTAButton(Trans::Tr("Editor.Scope.Assign").c_str(), true)) {
                    scanner.SetSelectedCategory(targetCategory);
                    GetEditorWindowState().status = Trans::Tr("Editor.Scope.Assigned");
                }
                Controls::Tooltip(Trans::Tr("Editor.Scope.Assign.Tooltip").c_str());
            }

            if (categoryScope && targetCategory == ParticleCategory::kUnclassified) {
                GUI::TextWrapped("%s", Trans::Tr("Editor.Scope.ChooseCategory").c_str());
            }
            else {
                GUI::TextUnformatted(Trans::Format("Editor.Scope.Affected", scanner.GetAffectedLightCount()).c_str());
            }
        }

        GUI::Spacing();
        GUI::Spacing();

        auto enabled = a_editor.enabled;
        if (GUI::CollapsingHeader(Trans::Tr("Editor.Appearance.Header").c_str(), GUI::ImGuiTreeNodeFlags_DefaultOpen)) {
            GUI::Spacing();
            if (BeginPropertyTable("AppearanceProperties")) {
                BeginPropertyRow(Trans::Tr("Editor.Enabled.Label").c_str());
                const auto enabledLabel = std::format("{}##ParticleLightEnabled", Trans::Tr("Editor.Enabled.Checkbox"));
                if (GUI::Checkbox(enabledLabel.c_str(), &enabled)) {
                    scanner.SetSelectedEnabled(enabled);
                    GetEditorWindowState().status = Trans::Tr(enabled ? "Editor.Enabled.On" : "Editor.Enabled.Off");
                }
                Controls::Tooltip(Trans::Tr("Editor.Enabled.Tooltip").c_str());
                RenderPropertyReset(EditProperty::kEnabled, "ResetEnabled");

                if (!enabled) {
                    GUI::BeginDisabled();
                }

                BeginPropertyRow(Trans::Tr("Editor.Color").c_str());
                std::array color{ a_editor.color.red, a_editor.color.green, a_editor.color.blue, a_editor.color.alpha };
                auto opacity = a_editor.color.alpha;
                constexpr auto colorFlags = GUI::ImGuiColorEditFlags_DisplayRGB | GUI::ImGuiColorEditFlags_NoAlpha;
                GUI::SetNextItemWidth(330.0F);
                const auto colorChanged = GUI::ColorEdit4("##ParticleColor", color.data(), colorFlags);
                GUI::SameLine(0.0F, 14.0F);
                GUI::SetNextItemWidth(-1.0F);
                const auto opacityChanged = GUI::SliderFloat("##ParticleOpacity", &opacity, 0.0F, 1.0F, Trans::Tr("Editor.Opacity.Format").c_str());
                if (colorChanged || opacityChanged) {
                    scanner.SetSelectedColor({ color[0], color[1], color[2], opacity });
                    GetEditorWindowState().status = Trans::Tr("Editor.Color.Updated");
                }

                RenderPropertyReset(EditProperty::kColor, "ResetColor", 12.0f);

                BeginPropertyRow(Trans::Tr("Editor.Intensity").c_str());
                auto intensity = a_editor.intensity;
                GUI::SetNextItemWidth(-1.0F);
                if (GUI::SliderFloat("##ParticleIntensity", &intensity, 0.0F, 10.0F, "%.3f")) {
                    scanner.SetSelectedIntensity(intensity);
                    GetEditorWindowState().status = Trans::Tr("Editor.Intensity.Updated");
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

        AnimationUI::Render(a_editor);

        GUI::Spacing();
        GUI::Spacing();

        if (!enabled) {
            GUI::BeginDisabled();
        }
        if (GUI::CollapsingHeader(Trans::Tr("Editor.Geometry.Header").c_str(), GUI::ImGuiTreeNodeFlags_DefaultOpen)) {
            GUI::Spacing();
            if (BeginPropertyTable("GeometryProperties")) {
                BeginPropertyRow(Trans::Tr("Editor.Radius").c_str());
                auto radius = a_editor.radius;
                GUI::SetNextItemWidth(-1.0F);
                if (GUI::SliderFloat("##ParticleRadius", &radius, 1.0F, 4096.0F, "%.1f")) {
                    scanner.SetSelectedRadius(radius);
                    GetEditorWindowState().status = Trans::Tr("Editor.Radius.Updated");
                }
                Controls::Tooltip(Trans::Tr("Editor.Radius.Tooltip").c_str());
                RenderPropertyReset(EditProperty::kRadius, "ResetRadius", 12.0f);

                BeginPropertyRow(Trans::Tr("Editor.Position").c_str());
                std::array position{ a_editor.localPosition.x, a_editor.localPosition.y, a_editor.localPosition.z };
                GUI::SetNextItemWidth(-1.0F);
                if (GUI::DragFloat3("##ParticlePosition", position.data(), 1.0F, -4096.0F, 4096.0F, "%.2f")) {
                    scanner.SetSelectedLocalPosition({ position[0], position[1], position[2] });
                    GetEditorWindowState().status = Trans::Tr("Editor.Position.Updated");
                }
                Controls::Tooltip(Trans::Tr("Editor.Position.Tooltip").c_str());
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
        const auto windowTitle = std::format("{}###ParticleLightEditorWindow", Trans::Tr("Editor.Title"));
        const auto opened = GUI::Begin(windowTitle.c_str(), &editorOpen, 0);
        GUI::PopStyleColor();
        Controls::WindowTitleIcon(Trans::Tr("Editor.Title").c_str(), Icons::kLightbulb);
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
            GUI::TextUnformatted(Trans::Tr("Editor.NoLights").c_str());
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
            GUI::TextUnformatted(Trans::Tr("Editor.Unavailable").c_str());
        }

        Controls::SpacedSeparator();

        constexpr auto spacing = 8.0F;
        const auto selectedScope = scanner.GetEditScope() == EditScope::kSelectedLight;
        const auto& resetLabel = Trans::Tr(selectedScope ? "Editor.Reset.Light" : "Editor.Reset.Scope");
        const auto resetWidth = Controls::IconCTAButtonWidth(resetLabel.c_str(), Icons::kReset);
        const auto& saveLabel = Trans::Tr("Editor.Save");
        const auto saveWidth = Controls::IconCTAButtonWidth(saveLabel.c_str(), Icons::kSave);
        auto& state = GetEditorWindowState();
        const auto& defaultStatus = Trans::Tr(editorAvailable && !editor.enabled ? "Editor.Status.Disabled" : "Editor.Status.ResetHint");
        Controls::AlignTextToCTAButton();
        GUI::TextColored(Color::kEditActionText, "%s", state.status.empty() ? defaultStatus.c_str() : state.status.c_str());
        GUI::SameLine(0.0F, spacing);
        Controls::AlignActions(resetWidth + saveWidth + spacing);
        if (Controls::IconCTAButton(resetLabel.c_str(), scanner.IsSelectedScopeEdited(), Icons::kReset)) {
            state.status = Trans::Tr(scanner.ResetSelectedScope() ? (selectedScope ? "Editor.Reset.Light.Success" : "Editor.Reset.Scope.Success") : (selectedScope ? "Editor.Reset.Light.Failure" : "Editor.Reset.Scope.Failure"));
        }
        Controls::Tooltip(Trans::Tr(selectedScope ? "Editor.Reset.Light.Tooltip" : "Editor.Reset.Scope.Tooltip").c_str());
        GUI::SameLine(0.0F, spacing);
        if (Controls::IconCTAButton(saveLabel.c_str(), Settings::AreEditsDirty(), Icons::kSave)) {
            state.status = Trans::Tr(Settings::Save() ? "Editor.Save.Success" : "Editor.Save.Failure");
        }
        Controls::Tooltip(Trans::Tr("Editor.Save.Tooltip").c_str());
        GUI::End();
    }
}
