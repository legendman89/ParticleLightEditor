#include "menu.hpp"

#include "category.hpp"
#include "color.hpp"
#include "scanner.hpp"
#include "controls.hpp"
#include "logger.hpp"
#include "settings.hpp"

namespace ParticleLightEditor::Menu
{
    bool SetEditorWindowOpen(bool a_open)
    {
        auto* window = GetState().editorWindow;
        if (!window) {
            return false;
        }
        window->IsOpen.store(a_open);
        return window->IsOpen.load();
    }

    bool IsEditorWindowOpen()
    {
        auto* window = GetState().editorWindow;
        return window && window->IsOpen.load();
    }

    void RenderSelection()
    {
        auto& scanner = Scanner::GetSingleton();
        auto& state = GetState();
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
                const auto label = scanner.GetLightLabel(index);
                const auto selected = index == selectedIndex;
                if (GUI::Selectable(label.c_str(), selected)) {
                    scanner.SelectLight(index);
                    state.consoleStatus.clear();
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

    void RenderIdentity(const EditorState& a_editor)
    {
        const auto hasSource = a_editor.associatedLightRefID != 0;
        const auto& sourceName = hasSource ? a_editor.associatedLightName : a_editor.baseEditorID;
        GUI::Text("Source light: %s [%08X]", sourceName.empty() ? "<unnamed>" : sourceName.c_str(), hasSource ? a_editor.associatedLightRefID : a_editor.ownerFormID);
        GUI::Text("Category: %s", Category::Name(a_editor.category));
    }

    void RenderEditorControls(const EditorState& a_editor)
    {
        auto& scanner = Scanner::GetSingleton();
        if (GUI::CollapsingHeader("Edit Scope", GUI::ImGuiTreeNodeFlags_DefaultOpen)) {

            GUI::Spacing();

            constexpr const char* scopes[]{ "This light", "Same category in current cell", "Same category everywhere" };
            
            auto scope = static_cast<int>(scanner.GetEditScope());
            GUI::SetNextItemWidth(350.0F);
            if (GUI::Combo("Apply changes to", &scope, scopes, static_cast<int>(std::size(scopes)))) {
                scanner.SetEditScope(static_cast<EditScope>(scope));
            }
            Controls::Tooltip("Category scopes affect color, opacity, intensity, and proportional radius. Local position always affects only this light.");

            GUI::SameLine(0.0f, 12.f);

            auto category = static_cast<int>(scanner.GetTargetCategory());
            GUI::SetNextItemWidth(220.0F);
            if (GUI::Combo("Target category", &category, Category::kNames, static_cast<int>(std::size(Category::kNames)))) {
                scanner.SetTargetCategory(static_cast<ParticleCategory>(category));
            }
            Controls::Tooltip("Choose which category the current scope edits. This does not change the selected mesh's category.");

            GUI::Spacing();
            GUI::Spacing();

            const auto targetCategory = scanner.GetTargetCategory();
            if (targetCategory != ParticleCategory::kUnclassified && targetCategory != a_editor.category) {
                if (Controls::CTAButton("Assign Selected Mesh to Target", true)) {
                    scanner.SetSelectedCategory(targetCategory);
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

        if (GUI::CollapsingHeader("Appearance", GUI::ImGuiTreeNodeFlags_DefaultOpen)) {

            GUI::Spacing();

            std::array color{ a_editor.color.red, a_editor.color.green, a_editor.color.blue, a_editor.color.alpha };
            constexpr auto colorFlags = GUI::ImGuiColorEditFlags_DisplayRGB | GUI::ImGuiColorEditFlags_AlphaBar;
            if (GUI::ColorEdit4("Color and opacity", color.data(), colorFlags)) {
                scanner.SetSelectedColor({ color[0], color[1], color[2], color[3] });
            }
            
            GUI::Spacing();

            auto intensity = a_editor.intensity;
            if (GUI::SliderFloat("Intensity", &intensity, 0.0F, 10.0F, "%.3f")) {
                scanner.SetSelectedIntensity(intensity);
            }
        }

        GUI::Spacing();
        GUI::Spacing();

        if (GUI::CollapsingHeader("Geometry", GUI::ImGuiTreeNodeFlags_DefaultOpen)) {
            
            GUI::Spacing();

            auto radius = a_editor.radius;
            if (GUI::SliderFloat("Particle radius", &radius, 1.0F, 4096.0F, "%.1f")) {
                scanner.SetSelectedRadius(radius);
            }
            Controls::Tooltip("Category scopes preserve each light's relative size by applying the selected light's radius ratio.");
            
            GUI::Spacing();

            std::array position{ a_editor.localPosition.x, a_editor.localPosition.y, a_editor.localPosition.z };
            if (GUI::DragFloat3("Local position", position.data(), 1.0F, -4096.0F, 4096.0F, "%.2f")) {
                scanner.SetSelectedLocalPosition({ position[0], position[1], position[2] });
            }
            Controls::Tooltip("Local position always changes only the selected light because different meshes use different pivots.");
        }
    }

    void __stdcall RenderEditorWindow()
    {
        if (!IsEditorWindowOpen()) {
            return;
        }

        Controls::CenterNextWindow();
        GUI::SetNextWindowSize(GUI::ImVec2{ 900.0F, 640.0F }, GUI::ImGuiCond_Appearing);
        GUI::PushStyleColor(GUI::ImGuiCol_WindowBg, Color::kEditorBackground);
        auto editorOpen = true;
        const auto opened = GUI::Begin("Particle Light Editor###ParticleLightEditorWindow", &editorOpen, 0);
        GUI::PopStyleColor();
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
        if (scanner.GetSelectedEditorState(editor)) {
            RenderIdentity(editor);
            GUI::Spacing();
            RenderEditorControls(editor);
        }
        else {
            GUI::Text("The selected particle geometry is no longer editable.");
        }

        GUI::Spacing();
        GUI::Separator();
        GUI::Spacing();

        constexpr auto spacing = 8.0F;
        const auto saveWidth = Controls::IconCTAButtonWidth("Save Changes");
        const auto resetLabel = scanner.GetEditScope() == EditScope::kSelectedLight ? "Reset Light" : "Reset Scope";
        const auto resetWidth = Controls::IconCTAButtonWidth(resetLabel);
        Controls::AlignActions(saveWidth + resetWidth + spacing);
        if (Controls::IconCTAButton("Save Changes", Settings::AreEditsDirty(), Icons::kSave)) {
            Settings::Save();
        }
        Controls::Tooltip("Save every edited particle light and the current menu options.");
        GUI::SameLine(0.0F, spacing);
        if (Controls::IconCTAButton(resetLabel, scanner.IsSelectedScopeEdited(), Icons::kReset)) {
            scanner.ResetSelectedScope();
        }
        Controls::Tooltip(scanner.GetEditScope() == EditScope::kSelectedLight ?
            "Restore the selected particle light to its original runtime values." :
            "Remove the active category rule and reveal any more-specific saved edits.");
        GUI::End();
    }
}
