#pragma once

#include "menu.hpp"

namespace ParticleLightEditor::Menu
{
    class ConsoleSelectionHandler : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
    {
    public:

        static ConsoleSelectionHandler& GetSingleton();
        
        RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_source) override;
    };

    inline ConsoleSelectionHandler& ConsoleSelectionHandler::GetSingleton()
    {
        static ConsoleSelectionHandler singleton;
        return singleton;
    }

    void UseConsoleSelection(bool a_openEditor);
}
