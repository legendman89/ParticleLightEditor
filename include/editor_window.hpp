#pragma once

#include "menu.hpp"

#include "category.hpp"
#include "controls.hpp"
#include "scanner.hpp"
#include "utility.hpp"

#define EDIT_PROPERTY_NAME_CASE(PROPERTY, NAME, CHANGED, COMPARISON, LABEL) case EditProperty::PROPERTY: return LABEL;

namespace ParticleLightEditor::Menu
{
    struct CopiedEdits
    {
        RE::NiColorA color{ 1.0F, 1.0F, 1.0F, 1.0F };
        float intensity{ 1.0F };
        float radiusScale{ 1.0F };
        bool available{ false };
    };

    struct EditorWindowState
    {
        CopiedEdits copiedEdits;
        std::string status;
    };

    inline EditorWindowState& GetEditorWindowState()
    {
        static EditorWindowState state;
        return state;
    }

    inline void ClearEditorStatus() { GetEditorWindowState().status.clear(); }

    inline bool SelectionMatchesFilter(const Scanner& a_scanner, const State& a_state, size_t a_index)
    {
        return (!a_state.editedLightsOnly || a_scanner.IsLightEdited(a_index)) && a_scanner.LightMatchesFilter(a_index, a_state.lightFilter.data());
    }

    inline bool SetEditorWindowOpen(bool a_open)
    {
        auto* window = GetState().editorWindow;
        if (!window) {
            return false;
        }
        if (a_open && !window->IsOpen.load()) {
            ClearEditorStatus();
        }
        window->IsOpen.store(a_open);
        return window->IsOpen.load();
    }

    inline bool IsEditorWindowOpen()
    {
        auto* window = GetState().editorWindow;
        return window && window->IsOpen.load();
    }

    inline void RenderIdentity(const EditorState& a_editor)
    {
        const auto hasSource = a_editor.associatedLightRefID != 0;
        const auto& sourceName = hasSource ? a_editor.associatedLightName : a_editor.baseEditorID;
        const auto sourceLabel = Utility::BeautifyLabel(sourceName.empty() ? "Unnamed" : sourceName);
        GUI::Text("Source light: %s [%08X]", sourceLabel.c_str(), hasSource ? a_editor.associatedLightRefID : a_editor.ownerFormID);
        GUI::Text("Category: %s", Category::Name(a_editor.category));
    }

    inline const char* PropertyName(EditProperty a_property)
    {
        switch (a_property) {
            FOREACH_EDIT_PROPERTY(EDIT_PROPERTY_NAME_CASE)
        default:
            return "Property";
        }
    }

    inline bool BeginPropertyTable(const char* a_id)
    {
        constexpr auto flags = GUI::ImGuiTableFlags_SizingStretchProp | GUI::ImGuiTableFlags_NoSavedSettings;
        if (!GUI::BeginTable(a_id, 3, flags)) {
            return false;
        }
        GUI::TableSetupColumn("Property", GUI::ImGuiTableColumnFlags_WidthFixed, 230.0F);
        GUI::TableSetupColumn("Value", GUI::ImGuiTableColumnFlags_WidthStretch);
        GUI::TableSetupColumn("Reset", GUI::ImGuiTableColumnFlags_WidthFixed, 60.0F);
        return true;
    }

    inline void BeginPropertyRow(const char* a_label)
    {
        GUI::TableNextRow(0, 42.0F);
        GUI::TableNextColumn();
        GUI::AlignTextToFramePadding();
        GUI::TextUnformatted(a_label);
        GUI::TableNextColumn();
    }

    inline void RenderPropertyReset(EditProperty a_property, const char* a_id, float a_leftSpacing = 8.0F)
    {
        auto& scanner = Scanner::GetSingleton();
        GUI::TableNextColumn();
        GUI::ImVec2 cursor{};
        GUI::GetCursorPos(&cursor);
        GUI::SetCursorPos(GUI::ImVec2{ cursor.x + a_leftSpacing, cursor.y });
        if (Controls::IconOnlyButton(a_id, scanner.IsSelectedPropertyEdited(a_property), Icons::kReset)) {
            const auto selectedOnly = scanner.GetEditScope() == EditScope::kSelectedLight || a_property == EditProperty::kPosition || a_property == EditProperty::kEnabled;
            if (scanner.ResetSelectedProperty(a_property)) {
                GetEditorWindowState().status = std::format("{} reset for {}.", PropertyName(a_property), selectedOnly ? "this light" : "the active category scope");
            }
            else {
                GetEditorWindowState().status = std::format("Could not reset {}.", PropertyName(a_property));
            }
        }
        const auto tooltip = std::format("Reset {} for the active edit scope.", PropertyName(a_property));
        Controls::Tooltip(tooltip.c_str());
        GUI::TableNextRow();
        GUI::TableNextColumn();
        GUI::Spacing();
        GUI::Spacing();
    }

    inline bool CopyEditorEdits(const EditorState& a_editor)
    {
        if (!std::isfinite(a_editor.defaultRadius) || a_editor.defaultRadius <= 0.0F) {
            return false;
        }
        auto& state = GetEditorWindowState();
        state.copiedEdits.color = a_editor.color;
        state.copiedEdits.intensity = a_editor.intensity;
        state.copiedEdits.radiusScale = a_editor.radius / a_editor.defaultRadius;
        state.copiedEdits.available = std::isfinite(state.copiedEdits.radiusScale) && state.copiedEdits.radiusScale > 0.0F;
        state.status = state.copiedEdits.available ? "Copied color, opacity, intensity, and radius scale." : "Could not copy the selected light edits.";
        return state.copiedEdits.available;
    }

    inline bool PasteEditorEdits(const EditorState& a_editor)
    {
        auto& state = GetEditorWindowState();
        if (!state.copiedEdits.available || !a_editor.enabled || !std::isfinite(a_editor.defaultRadius) || a_editor.defaultRadius <= 0.0F) {
            return false;
        }
        auto& scanner = Scanner::GetSingleton();
        const auto colorApplied = scanner.SetSelectedColor(state.copiedEdits.color);
        const auto intensityApplied = scanner.SetSelectedIntensity(state.copiedEdits.intensity);
        const auto radiusApplied = scanner.SetSelectedRadius(a_editor.defaultRadius * state.copiedEdits.radiusScale);
        const auto pasted = colorApplied && intensityApplied && radiusApplied;
        state.status = pasted ? "Pasted color, opacity, intensity, and radius scale." : "Some copied edits could not be pasted.";
        return pasted;
    }

    inline void RenderClipboardActions(const EditorState& a_editor)
    {
        auto& state = GetEditorWindowState();
        if (Controls::IconCTAButton("Copy Edits", true, Icons::kCopy, 10.0F, "Ctrl+C")) {
            CopyEditorEdits(a_editor);
        }
        Controls::Tooltip("Copies color, opacity, intensity, and radius scale.");
        GUI::SameLine(0.0F, 8.0F);
        if (Controls::IconCTAButton("Paste Edits", state.copiedEdits.available && a_editor.enabled, Icons::kPaste, 10.0F, "Ctrl+V")) {
            PasteEditorEdits(a_editor);
        }
        Controls::Tooltip("Paste the copied appearance and radius scale into the active edit scope.");
    }

    inline void HandleEditorShortcuts(const EditorState& a_editor)
    {
        const auto* io = GUI::GetIO();
        if (!io || !io->KeyCtrl || GUI::IsAnyItemActive() || !GUI::IsWindowFocused(GUI::ImGuiFocusedFlags_RootAndChildWindows)) {
            return;
        }
        if (GUI::IsKeyPressed(GUI::ImGuiKey_C, false)) {
            CopyEditorEdits(a_editor);
        }
        if (GUI::IsKeyPressed(GUI::ImGuiKey_V, false) && GetEditorWindowState().copiedEdits.available && a_editor.enabled) {
            PasteEditorEdits(a_editor);
        }
    }

    void RenderSelection();

    void __stdcall RenderEditorWindow();
}

#undef EDIT_PROPERTY_NAME_CASE
