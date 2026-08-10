#include "console.hpp"

#include "editor_window.hpp"
#include "logger.hpp"
#include "scanner.hpp"
#include "settings.hpp"

namespace ParticleLightEditor::Menu
{
    void UseConsoleSelection(bool a_openEditor)
    {
        auto& scanner = Scanner::GetSingleton();
        auto& state = GetState();
        const auto reference = RE::Console::GetSelectedRef();
        if (!reference) {
            state.consoleStatus = "No reference is selected in the console.";
            logger::info(
                "Console selection failed: no reference is selected; editorOpenRequested={}, editorRegistered={}, editorIsOpen={}",
                a_openEditor,
                state.editorWindow != nullptr,
                IsEditorWindowOpen());
            return;
        }

        const auto matchCount = scanner.SelectReference(reference.get());
        if (matchCount == 0) {
            SetEditorWindowOpen(false);
            ClearEditorStatus();
            state.consoleStatus = std::format("Reference {:08X} has no particle lights inside the current detection range.", reference->GetFormID());
            logger::info(
                "Console-selected reference {:08X}: matched no particle lights; editorOpenRequested={}, editorRegistered={}, editorIsOpen={}",
                reference->GetFormID(),
                a_openEditor,
                state.editorWindow != nullptr,
                IsEditorWindowOpen());
            return;
        }

        if (matchCount == 1) {
            state.consoleStatus = std::format("Selected the particle light owned by reference {:08X}.", reference->GetFormID());
        }
        else {
            state.consoleStatus = std::format("Selected Particle Light 1 of {} owned by reference {:08X}.", matchCount, reference->GetFormID());
        }
        ClearEditorStatus();
        const auto editorIsOpen = a_openEditor ? SetEditorWindowOpen(true) : IsEditorWindowOpen();
        logger::info(
            "Console-selected reference {:08X}: matched {} particle light(s); editorOpenRequested={}, editorRegistered={}, editorIsOpen={}",
            reference->GetFormID(),
            matchCount,
            a_openEditor,
            state.editorWindow != nullptr,
            editorIsOpen);
    }

    RE::BSEventNotifyControl ConsoleSelectionHandler::ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
    {
        if (a_event && a_event->menuName == RE::Console::MENU_NAME && !a_event->opening) {
            const auto automaticOpen = Settings::GetSettings().openEditorAfterConsoleSelection;
            logger::info("Console closed; automatic editor opening is {}", automaticOpen ? "enabled" : "disabled");
            UseConsoleSelection(automaticOpen);
        }
        return RE::BSEventNotifyControl::kContinue;
    }
}
