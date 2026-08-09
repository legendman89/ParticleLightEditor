#include "hooks.hpp"
#include "reference.hpp"
#include "scanner.hpp"
#include "logger.hpp"
#include "menu.hpp"
#include "registry.hpp"
#include "settings.hpp"

namespace ParticleLightEditor
{
    void MessageHandler(SKSE::MessagingInterface::Message* a_message)
    {
        switch (a_message->type) {

        case SKSE::MessagingInterface::kPreLoadGame:
        case SKSE::MessagingInterface::kNewGame:

            logger::info("Clearing runtime references and scheduling saved edits for reapplication");
            ParticleLightEditor::Registry::GetSingleton().Reset();
            ParticleLightEditor::ReferenceManager::GetSingleton().ClearMaterials();
            ParticleLightEditor::Scanner::GetSingleton().Reset();
            Settings::RestoreEdits();
            ParticleLightEditor::Scanner::GetSingleton().RequestRescan(true);
            break;

        case SKSE::MessagingInterface::kPostLoadGame:

            logger::info("Scheduling saved edits for reapplication");
            ParticleLightEditor::ReferenceManager::GetSingleton().ClearMaterials();
            ParticleLightEditor::Scanner::GetSingleton().Reset();
            Settings::RestoreEdits();
            ParticleLightEditor::Scanner::GetSingleton().RequestRescan(true);
            break;

        case SKSE::MessagingInterface::kDataLoaded:

            if (Settings::Load()) {
                Scanner::GetSingleton().RequestRescan(true);
            }
            logger::info("Data loaded, particle-light detection is active");
            break;
            
        default:
            break;

        }
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
    SKSE::Init(a_skse);
    
    SetupLog();

    SKSE::GetMessagingInterface()->RegisterListener(ParticleLightEditor::MessageHandler);

    ParticleLightEditor::Menu::Register();

    Hooks::Install();

    logger::info("Particle Light Editor loaded");

    return true;
}
