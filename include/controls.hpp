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
        GUI::PushStyleVar(GUI::ImGuiStyleVar_FramePadding, GUI::ImVec2{ 14.0F, 6.0F });
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
        return size.x + 28.0F;
    }

    inline float IconCTAButtonWidth(const char* a_label)
    {
        const auto label = "      " + std::string(a_label);
        GUI::ImVec2 size{};
        GUI::CalcTextSize(&size, label.c_str(), nullptr, false, -1.0F);
        return size.x + 28.0F;
    }

    inline bool IconCTAButton(const char* a_label, bool a_enabled, unsigned a_icon)
    {
        const auto iconText = FontAwesome::UnicodeToUtf8(a_icon);

        GUI::PushStyleVar(GUI::ImGuiStyleVar_FrameRounding, 6.0F);
        GUI::PushStyleVar(GUI::ImGuiStyleVar_FramePadding, GUI::ImVec2{ 14.0F, 6.0F });
        GUI::PushStyleColor(GUI::ImGuiCol_Button, a_enabled ? Color::kCTAOnBackground : Color::kCTAOffBackground);
        GUI::PushStyleColor(GUI::ImGuiCol_ButtonHovered, a_enabled ? Color::kCTAOnHover : Color::kCTAOffHover);
        GUI::PushStyleColor(GUI::ImGuiCol_ButtonActive, a_enabled ? Color::kCTAOnActive : Color::kCTAOffActive);
        GUI::PushStyleColor(GUI::ImGuiCol_Text, a_enabled ? Color::kCTAOnText : Color::kCTAOffText);

        if (!a_enabled) {
            GUI::BeginDisabled();
        }

        const auto clicked = GUI::Button(("      " + std::string(a_label)).c_str());

        if (!a_enabled) {
            GUI::EndDisabled();
        }

        GUI::ImVec2 buttonMin{};
        GUI::ImVec2 buttonMax{};
        GUI::GetItemRectMin(&buttonMin);
        GUI::GetItemRectMax(&buttonMax);

        FontAwesome::PushSolid();
        const auto* iconFont = GUI::GetFont();
        const auto iconFontSize = GUI::GetFontSize();
        const auto iconColor = GUI::GetColorU32(a_enabled ? Color::kCTAOnText : Color::kCTAOffText);
        GUI::ImVec2 iconSize{};
        GUI::CalcTextSize(&iconSize, iconText.c_str(), nullptr, false, -1.0F);
        FontAwesome::Pop();

        GUI::ImDrawListManager::AddText(
            GUI::GetWindowDrawList(),
            iconFont,
            iconFontSize,
            GUI::ImVec2{ buttonMin.x + 13.0F, buttonMin.y + ((buttonMax.y - buttonMin.y - iconSize.y) * 0.5F) },
            iconColor,
            iconText.c_str());

        GUI::PopStyleColor(4);
        GUI::PopStyleVar(2);
        return clicked && a_enabled;
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
