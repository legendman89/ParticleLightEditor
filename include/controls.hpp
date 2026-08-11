#pragma once

#include "menu.hpp"
#include "color.hpp"
#include "icons.hpp"

#include <algorithm>
#include <string>

namespace ParticleLightEditor::Menu::Controls
{
    inline constexpr auto kEditActionButtonWidth = 220.0F;
    inline constexpr auto kEditActionButtonHeight = 50.0F;
    inline constexpr auto kShortcutFontScale = 0.82F;
    inline constexpr auto kCTAFramePaddingX = 14.0F;
    inline constexpr auto kCTAFramePaddingY = 6.0F;

    struct ActionState
    {
        GUI::ImVec2 start;
    };

    inline ActionState& GetActionState()
    {
        static ActionState state;
        return state;
    }

    inline bool CTAButton(const char* a_label, bool a_enabled)
    {
        GUI::PushStyleVar(GUI::ImGuiStyleVar_FrameRounding, 6.0F);
        GUI::PushStyleVar(GUI::ImGuiStyleVar_FramePadding, GUI::ImVec2{ kCTAFramePaddingX, kCTAFramePaddingY });
        GUI::PushStyleColor(GUI::ImGuiCol_Button, a_enabled ? Color::kCTAOnBackground : Color::kCTAOffBackground);
        GUI::PushStyleColor(GUI::ImGuiCol_ButtonHovered, a_enabled ? Color::kCTAOnHover : Color::kCTAOffHover);
        GUI::PushStyleColor(GUI::ImGuiCol_ButtonActive, a_enabled ? Color::kCTAOnActive : Color::kCTAOffActive);
        GUI::PushStyleColor(GUI::ImGuiCol_Text, a_enabled ? Color::kCTAOnText : Color::kCTAOffText);
        if (!a_enabled) {
            GUI::BeginDisabled();
        }

        const auto clicked = GUI::Button(a_label);
        if (!a_enabled) {
            GUI::EndDisabled();
        }
        GUI::PopStyleColor(4);
        GUI::PopStyleVar(2);
        return clicked && a_enabled;
    }

    inline float CTAButtonWidth(const char* a_label)
    {
        GUI::ImVec2 size{};
        GUI::CalcTextSize(&size, a_label, nullptr, false, -1.0F);
        return size.x + kCTAFramePaddingX * 2.0F;
    }

    inline float IconCTAButtonWidth(const char* a_label, unsigned a_icon, float a_iconTextSpacing = 8.0F, const char* a_shortcut = nullptr)
    {
        GUI::ImVec2 labelSize{};
        GUI::ImVec2 iconSize{};
        GUI::ImVec2 shortcutSize{};
        GUI::CalcTextSize(&labelSize, a_label, nullptr, false, -1.0F);
        FontAwesome::PushSolid();
        const auto iconText = FontAwesome::UnicodeToUtf8(a_icon);
        GUI::CalcTextSize(&iconSize, iconText.c_str(), nullptr, false, -1.0F);
        FontAwesome::Pop();
        if (a_shortcut && *a_shortcut) {
            GUI::CalcTextSize(&shortcutSize, a_shortcut, nullptr, false, -1.0F);
            shortcutSize.x *= kShortcutFontScale;
        }
        return kCTAFramePaddingX * 2.0F + iconSize.x + a_iconTextSpacing + labelSize.x + (shortcutSize.x > 0.0F ? shortcutSize.x + 20.0F : 0.0F);
    }

    inline bool IconCTAButton(const char* a_label, bool a_enabled, unsigned a_icon, float a_iconTextSpacing = 8.0F, const char* a_shortcut = nullptr)
    {
        const auto iconText = FontAwesome::UnicodeToUtf8(a_icon);
        const auto hasShortcut = a_shortcut && *a_shortcut;
        const auto buttonLabel = std::format("##{}IconCTAButton", a_label);
        const GUI::ImVec2 buttonSize{ IconCTAButtonWidth(a_label, a_icon, a_iconTextSpacing, a_shortcut), 0.0F };

        GUI::PushStyleVar(GUI::ImGuiStyleVar_FrameRounding, 6.0F);
        GUI::PushStyleVar(GUI::ImGuiStyleVar_FramePadding, GUI::ImVec2{ kCTAFramePaddingX, kCTAFramePaddingY });
        GUI::PushStyleColor(GUI::ImGuiCol_Button, a_enabled ? Color::kCTAOnBackground : Color::kCTAOffBackground);
        GUI::PushStyleColor(GUI::ImGuiCol_ButtonHovered, a_enabled ? Color::kCTAOnHover : Color::kCTAOffHover);
        GUI::PushStyleColor(GUI::ImGuiCol_ButtonActive, a_enabled ? Color::kCTAOnActive : Color::kCTAOffActive);
        GUI::PushStyleColor(GUI::ImGuiCol_Text, a_enabled ? Color::kCTAOnText : Color::kCTAOffText);

        if (!a_enabled) {
            GUI::BeginDisabled();
        }

        const auto clicked = GUI::Button(buttonLabel.c_str(), buttonSize);

        if (!a_enabled) {
            GUI::EndDisabled();
        }

        GUI::ImVec2 buttonMin{};
        GUI::ImVec2 buttonMax{};
        GUI::GetItemRectMin(&buttonMin);
        GUI::GetItemRectMax(&buttonMax);

        GUI::ImVec2 labelSize{};
        GUI::ImVec2 shortcutSize{};
        const auto* textFont = GUI::GetFont();
        const auto textFontSize = GUI::GetFontSize();
        GUI::CalcTextSize(&labelSize, a_label, nullptr, false, -1.0F);
        if (hasShortcut) {
            GUI::CalcTextSize(&shortcutSize, a_shortcut, nullptr, false, -1.0F);
            shortcutSize.x *= kShortcutFontScale;
            shortcutSize.y *= kShortcutFontScale;
        }

        FontAwesome::PushSolid();
        const auto* iconFont = GUI::GetFont();
        const auto iconFontSize = GUI::GetFontSize();
        const auto iconColor = GUI::GetColorU32(a_enabled ? Color::kCTAOnText : Color::kCTAOffText);
        GUI::ImVec2 iconSize{};
        GUI::CalcTextSize(&iconSize, iconText.c_str(), nullptr, false, -1.0F);
        FontAwesome::Pop();

        const auto contentWidth = iconSize.x + a_iconTextSpacing + labelSize.x;
        const auto contentStart = hasShortcut ? buttonMin.x + kCTAFramePaddingX : buttonMin.x + (buttonMax.x - buttonMin.x - contentWidth) * 0.5F;
        GUI::ImDrawListManager::AddText(
            GUI::GetWindowDrawList(),
            iconFont,
            iconFontSize,
            GUI::ImVec2{ contentStart, buttonMin.y + ((buttonMax.y - buttonMin.y - iconSize.y) * 0.5F) },
            iconColor,
            iconText.c_str());

        GUI::ImDrawListManager::AddText(GUI::GetWindowDrawList(), GUI::ImVec2{ contentStart + iconSize.x + a_iconTextSpacing, buttonMin.y + (buttonMax.y - buttonMin.y - labelSize.y) * 0.5F }, iconColor, a_label);

        if (hasShortcut) {
            GUI::ImDrawListManager::AddText(GUI::GetWindowDrawList(), textFont, textFontSize * kShortcutFontScale, GUI::ImVec2{ buttonMax.x - kCTAFramePaddingX - shortcutSize.x, buttonMin.y + (buttonMax.y - buttonMin.y - shortcutSize.y) * 0.5F }, iconColor, a_shortcut);
        }

        GUI::PopStyleColor(4);
        GUI::PopStyleVar(2);
        return clicked && a_enabled;
    }

    inline bool IconOnlyButton(const char* a_id, bool a_enabled, unsigned a_icon)
    {
        GUI::PushStyleVar(GUI::ImGuiStyleVar_FrameBorderSize, 0.0F);
        GUI::PushStyleColor(GUI::ImGuiCol_Button, Color::kTransparent);
        GUI::PushStyleColor(GUI::ImGuiCol_ButtonHovered, Color::kTransparent);
        GUI::PushStyleColor(GUI::ImGuiCol_ButtonActive, Color::kTransparent);
        if (!a_enabled) {
            GUI::BeginDisabled();
        }
        const auto iconText = FontAwesome::UnicodeToUtf8(a_icon);
        const auto label = std::format("##{}", a_id);
        FontAwesome::PushSolid();
        const auto* iconFont = GUI::GetFont();
        const auto normalIconSize = GUI::GetFontSize();
        GUI::ImVec2 iconBounds{};
        GUI::CalcTextSize(&iconBounds, iconText.c_str(), nullptr, false, -1.0F);
        FontAwesome::Pop();
        const auto clicked = GUI::Button(label.c_str(), GUI::ImVec2{ 38.0F, GUI::GetFrameHeight() });
        const auto hovered = a_enabled && GUI::IsItemHovered();
        GUI::ImVec2 buttonMin{};
        GUI::ImVec2 buttonMax{};
        GUI::GetItemRectMin(&buttonMin);
        GUI::GetItemRectMax(&buttonMax);
        constexpr auto iconScale = 1.35F;
        const auto iconFontSize = normalIconSize * iconScale;
        const GUI::ImVec2 scaledBounds{ iconBounds.x * iconScale, iconBounds.y * iconScale };
        const GUI::ImVec2 iconPosition{ buttonMin.x + (buttonMax.x - buttonMin.x - scaledBounds.x) * 0.5F, buttonMin.y + (buttonMax.y - buttonMin.y - scaledBounds.y) * 0.5F };
        const auto iconColor = hovered ? GUI::GetColorU32(Color::kIconHover) : GUI::GetColorU32(a_enabled ? GUI::ImGuiCol_Text : GUI::ImGuiCol_TextDisabled);
        GUI::ImDrawListManager::AddText(GUI::GetWindowDrawList(), iconFont, iconFontSize, iconPosition, iconColor, iconText.c_str());
        if (!a_enabled) {
            GUI::EndDisabled();
        }
        GUI::PopStyleColor(3);
        GUI::PopStyleVar();
        return clicked && a_enabled;
    }

    inline void WindowTitleIcon(const char* a_title, unsigned a_icon)
    {
        GUI::ImVec2 windowPosition{};
        GUI::ImVec2 windowSize{};
        GUI::ImVec2 titleSize{};
        GUI::ImVec2 iconSize{};
        GUI::GetWindowPos(&windowPosition);
        GUI::GetWindowSize(&windowSize);
        GUI::CalcTextSize(&titleSize, a_title, nullptr, false, -1.0F);

        const auto iconText = FontAwesome::UnicodeToUtf8(a_icon);
        FontAwesome::PushRegular();
        const auto* iconFont = GUI::GetFont();
        const auto iconFontSize = GUI::GetFontSize();
        GUI::CalcTextSize(&iconSize, iconText.c_str(), nullptr, false, -1.0F);
        FontAwesome::Pop();

        const auto* style = GUI::GetStyle();
        const auto titleBarHeight = GUI::GetFontSize() + (style ? style->FramePadding.y * 2.0F : 8.0F);
        const GUI::ImVec2 iconPosition{ windowPosition.x + (windowSize.x - titleSize.x) * 0.5F - iconSize.x - 8.0F, windowPosition.y + (titleBarHeight - iconSize.y) * 0.5F };
        GUI::ImDrawListManager::AddText(GUI::GetWindowDrawList(), iconFont, iconFontSize, iconPosition, GUI::GetColorU32(Color::kEditActionText), iconText.c_str());
    }

    inline float EditActionButtonWidth(const char* a_label)
    {
        const auto label = "        " + std::string(a_label);
        GUI::ImVec2 size{};
        GUI::CalcTextSize(&size, label.c_str(), nullptr, false, -1.0F);
        return (std::max)(kEditActionButtonWidth, size.x + 28.0F);
    }

    inline bool EditActionButton(const char* a_label, bool a_enabled)
    {
        static const auto editText = FontAwesome::UnicodeToUtf8(Icons::kEdit);
        const auto width = EditActionButtonWidth(a_label);

        if (!a_enabled) {
            GUI::BeginDisabled();
        }

        GUI::PushStyleVar(GUI::ImGuiStyleVar_FrameRounding, 5.0F);
        GUI::PushStyleVar(GUI::ImGuiStyleVar_FrameBorderSize, 1.0F);
        GUI::PushStyleColor(GUI::ImGuiCol_Button, Color::kEditActionBackground);
        GUI::PushStyleColor(GUI::ImGuiCol_ButtonHovered, Color::kEditActionHover);
        GUI::PushStyleColor(GUI::ImGuiCol_ButtonActive, Color::kEditActionActive);
        GUI::PushStyleColor(GUI::ImGuiCol_Text, Color::kEditActionText);
        GUI::PushStyleColor(GUI::ImGuiCol_Border, Color::kEditActionBorder);

        const auto clicked = GUI::Button(
            ("        " + std::string(a_label) + "##EditAction").c_str(),
            GUI::ImVec2{ width, kEditActionButtonHeight });

        GUI::ImVec2 buttonMin{};
        GUI::ImVec2 buttonMax{};
        GUI::ImVec2 nextItemPosition{};
        GUI::GetItemRectMin(&buttonMin);
        GUI::GetItemRectMax(&buttonMax);
        GUI::GetCursorScreenPos(&nextItemPosition);

        FontAwesome::PushRegular();
        GUI::ImVec2 iconSize{};
        GUI::CalcTextSize(&iconSize, editText.c_str(), nullptr, false, -1.0F);
        GUI::SetCursorScreenPos(GUI::ImVec2{
            buttonMin.x + 13.0F,
            buttonMin.y + ((buttonMax.y - buttonMin.y - iconSize.y) * 0.5F) });
        GUI::TextUnformatted(editText.c_str());
        FontAwesome::Pop();
        GUI::SetCursorScreenPos(nextItemPosition);

        GUI::PopStyleColor(5);
        GUI::PopStyleVar(2);

        if (!a_enabled) {
            GUI::EndDisabled();
        }
        return clicked && a_enabled;
    }

    inline void AlignActions(float a_width)
    {
        GUI::ImVec2 cursor{};
        GUI::ImVec2 available{};
        GUI::GetCursorPos(&cursor);
        GetActionState().start = cursor;
        GUI::GetContentRegionAvail(&available);
        GUI::SetCursorPos(GUI::ImVec2{ cursor.x + (std::max)(0.0F, available.x - a_width), cursor.y });
    }

    inline void AlignTextToCTAButton()
    {
        const auto* style = GUI::GetStyle();
        GUI::PushStyleVar(GUI::ImGuiStyleVar_FramePadding, GUI::ImVec2{ style ? style->FramePadding.x : 4.0F, kCTAFramePaddingY });
        GUI::AlignTextToFramePadding();
        GUI::PopStyleVar();
    }

    inline void FinishActions()
    {
        GUI::ImVec2 cursor{};
        GUI::GetCursorPos(&cursor);
        GUI::SetCursorPos(GUI::ImVec2{ GetActionState().start.x, cursor.y });
        GUI::Spacing();
        GUI::Spacing();
        GUI::Separator();
        GUI::Spacing();
    }

    inline void SpacedSeparator()
    {
        GUI::Spacing();
        GUI::Spacing();
        const auto* headerColor = GUI::GetStyleColorVec4(GUI::ImGuiCol_Header);
        const auto separatorColor = headerColor ? *headerColor : GUI::ImVec4{ 0.35F, 0.35F, 0.35F, 1.0F };
        GUI::PushStyleColor(GUI::ImGuiCol_Separator, separatorColor);
        GUI::Separator();
        GUI::PopStyleColor();
        GUI::Spacing();
        GUI::Spacing();
    }

    inline void Tooltip(const char* a_text, float a_width = 420.0F)
    {
        if (GUI::IsItemHovered()) {
            GUI::BeginTooltip();
            GUI::PushTextWrapPos(a_width);
            GUI::TextUnformatted(a_text);
            GUI::PopTextWrapPos();
            GUI::EndTooltip();
        }
    }

    inline void CenterNextWindow()
    {
        const auto* io = GUI::GetIO();
        if (io) {
            GUI::SetNextWindowPos(GUI::ImVec2{ io->DisplaySize.x * 0.5F, io->DisplaySize.y * 0.5F }, GUI::ImGuiCond_Appearing, GUI::ImVec2{ 0.5F, 0.5F });
        }
    }
}
