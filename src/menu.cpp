#include "menu.hpp"

#include "console.hpp"
#include "logger.hpp"

namespace ParticleLightEditor::Menu
{
    void Register()
    {
        if (!SKSEMenuFramework::IsInstalled()) {
            logger::info("SKSE Menu Framework is not installed; in-game controls are unavailable");
            return;
        }

        SKSEMenuFramework::SetSection("Particle Light Editor");

        SKSEMenuFramework::AddSectionItem("Scanner", RenderScanner);

        SKSEMenuFramework::AddSectionItem("Tools", RenderTools);

        auto& state = GetState();
        state.editorWindow = SKSEMenuFramework::AddWindow(RenderEditorWindow, true);
        if (state.editorWindow) {
            logger::info("Registered standalone Particle Light Editor window with SKSE Menu Framework");
        }
        else {
            logger::error("Could not register the standalone Particle Light Editor window with SKSE Menu Framework");
        }

        if (auto* ui = RE::UI::GetSingleton()) {
            ui->AddEventSink(&ConsoleSelectionHandler::GetSingleton());
        }
        
        logger::info("Registered Particle Light Editor menu with SKSE Menu Framework");
    }
}
