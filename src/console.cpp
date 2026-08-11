#include "console.hpp"

#include "editor_window.hpp"
#include "logger.hpp"
#include "scanner.hpp"
#include "settings.hpp"
#include "translate.hpp"

namespace ParticleLightEditor::Menu
{
    void UseConsoleSelection(bool a_openEditor)
    {
        auto& scanner = Scanner::GetSingleton();
        auto& state = GetState();
        const auto reference = RE::Console::GetSelectedRef();
        if (!reference) {
            state.consoleStatus = Trans::Tr("Console.NoReference");
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
            state.consoleStatus = Trans::Format("Console.NoLights", reference->GetFormID());
            logger::info(
                "Console-selected reference {:08X}: matched no particle lights; editorOpenRequested={}, editorRegistered={}, editorIsOpen={}",
                reference->GetFormID(),
                a_openEditor,
                state.editorWindow != nullptr,
                IsEditorWindowOpen());
            return;
        }

        if (matchCount == 1) {
            state.consoleStatus = Trans::Format("Console.SelectedOne", reference->GetFormID());
        }
        else {
            state.consoleStatus = Trans::Format("Console.SelectedMany", matchCount, reference->GetFormID());
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
