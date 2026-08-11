#include "menu.hpp"

#include "console.hpp"
#include "editor_window.hpp"
#include "logger.hpp"
#include "translate.hpp"

namespace ParticleLightEditor::Menu
{
    void Register()
    {
        if (!SKSEMenuFramework::IsInstalled()) {
            logger::info("SKSE Menu Framework is not installed; in-game controls are unavailable");
            return;
        }

        SKSEMenuFramework::SetSection(Trans::Tr("Menu.Section").c_str());

        SKSEMenuFramework::AddSectionItem(Trans::Tr("Menu.Scanner").c_str(), RenderScanner);

        SKSEMenuFramework::AddSectionItem(Trans::Tr("Menu.Tools").c_str(), RenderTools);

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
