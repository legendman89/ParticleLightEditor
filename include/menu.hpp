#pragma once

#include "SKSEMenuFramework.h"
#include "types.hpp"

namespace GUI = ImGuiMCP;

namespace ParticleLightEditor::Menu
{
    inline constexpr const char* kLogLevels[]{ "Trace", "Debug", "Info", "Warning", "Error", "Critical", "Off" };
    
    struct State
    {
        SKSEMenuFramework::Model::WindowInterface* editorWindow{ nullptr };
        std::string consoleStatus;
        std::array<char, 256> lightFilter{};
        bool editedLightsOnly{ false };
    };

    inline State& GetState()
    {
        static State state;
        return state;
    }

    void Register();

    void __stdcall RenderScanner();

    void __stdcall RenderTools();

    void RenderScannerActions();

    void RenderScannerSections();

    void RenderToolsActions();

    void RenderToolsSections();

    void __stdcall RenderEditorWindow();

    bool SetEditorWindowOpen(bool a_open);

    bool IsEditorWindowOpen();

    void RenderSelection();

    void RenderIdentity(const EditorState& a_editor);
    
    void RenderEditorControls(const EditorState& a_editor);
}
