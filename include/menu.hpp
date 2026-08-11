#pragma once

#include "SKSEMenuFramework.h"
#include "types.hpp"

namespace GUI = ImGuiMCP;

namespace ParticleLightEditor::Menu
{
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

}
