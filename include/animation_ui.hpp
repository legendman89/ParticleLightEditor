#pragma once

#include "editor_window.hpp"

namespace ParticleLightEditor::Menu::AnimationUI
{
    struct Clipboard
    {
        AnimationEdit animation;
        bool available{ false };
    };

    inline Clipboard& GetClipboard()
    {
        static Clipboard clipboard;
        return clipboard;
    }

    inline void Apply(Scanner& a_scanner, const AnimationEdit& a_animation, const char* a_success)
    {
        GetEditorWindowState().status = Trans::Tr(a_scanner.SetSelectedAnimation(a_animation) ? a_success : "Editor.Animation.UpdateFailure");
    }

    inline void RenderReset(Scanner& a_scanner)
    {
        GUI::TableNextColumn();
        if (Controls::IconOnlyButton("ResetAnimation", a_scanner.IsSelectedAnimationEdited(), Icons::kReset)) {
            GetEditorWindowState().status = Trans::Tr(a_scanner.ResetSelectedAnimation() ? "Editor.Animation.ResetSuccess" : "Editor.Animation.ResetFailure");
        }
        Controls::Tooltip(Trans::Tr("Editor.Animation.ResetTooltip").c_str());
    }

    void Render(const EditorState& a_editor);
}
