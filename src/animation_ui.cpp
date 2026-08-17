#include "animation_ui.hpp"

#include "animation.hpp"

#define ANIMATION_PROFILE_LABEL(PROFILE, LABEL, DURATION) Trans::Tr(LABEL).c_str(),

namespace ParticleLightEditor::Menu::AnimationUI
{
    void Render(const EditorState& a_editor)
    {
        if (!GUI::CollapsingHeader(Trans::Tr("Editor.Animation.Header").c_str())) {
            return;
        }

        GUI::Spacing();
        auto& scanner = Scanner::GetSingleton();
        auto animation = a_editor.animation;
        if (!a_editor.enabled) {
            GUI::BeginDisabled();
        }

        if (BeginPropertyTable("AnimationProperties")) {
            BeginPropertyRow(Trans::Tr("Editor.Animation.Enabled").c_str());
            auto enabled = animation.enabled;
            const auto enabledLabel = std::format("{}##AnimateParticleLight", Trans::Tr("Editor.Animation.Enabled.Checkbox"));
            if (GUI::Checkbox(enabledLabel.c_str(), &enabled)) {
                animation.enabled = enabled;
                if (enabled && !a_editor.animationEdited) {
                    animation.primaryColor = a_editor.color;
                    animation.profile = a_editor.nativeAnimated ? AnimationProfile::kOriginal : Animation::SuggestedProfile(a_editor.category);
                }
                Apply(scanner, animation, enabled ? "Editor.Animation.Enabled.On" : "Editor.Animation.Enabled.Off");
            }
            Controls::Tooltip(Trans::Tr("Editor.Animation.Enabled.Tooltip").c_str());
            RenderReset(scanner);

            if (!animation.enabled) {
                GUI::BeginDisabled();
            }

            BeginPropertyRow(Trans::Tr("Editor.Animation.Pattern").c_str());
            const std::array profiles{
                FOREACH_ANIMATION_PROFILE(ANIMATION_PROFILE_LABEL)
            };
            auto profile = static_cast<int>(animation.profile);
            GUI::SetNextItemWidth(-1.0F);
            if (GUI::Combo("##AnimationProfile", &profile, profiles.data(), static_cast<int>(profiles.size()))) {
                animation.profile = static_cast<AnimationProfile>(profile);
                Apply(scanner, animation, "Editor.Animation.Updated");
            }
            Controls::Tooltip(Trans::Tr(a_editor.nativeAnimated ? "Editor.Animation.Pattern.Tooltip.Native" : "Editor.Animation.Pattern.Tooltip.Static").c_str());
            GUI::TableNextColumn();

            BeginPropertyRow(Trans::Tr("Editor.Animation.Speed").c_str());
            GUI::SetNextItemWidth(-1.0F);
            if (GUI::SliderFloat("##AnimationSpeed", &animation.speed, 0.05F, 5.0F, "%.2fx")) {
                Apply(scanner, animation, "Editor.Animation.Updated");
            }
            Controls::Tooltip(Trans::Tr("Editor.Animation.Speed.Tooltip").c_str());
            GUI::TableNextColumn();

            BeginPropertyRow(Trans::Tr("Editor.Animation.Brightness").c_str());
            GUI::SetNextItemWidth(-1.0F);
            if (GUI::DragFloatRange2("##AnimationBrightness", &animation.minimumBrightness, &animation.maximumBrightness, 0.01F, 0.0F, 2.0F, "Min %.2f", "Max %.2f")) {
                Apply(scanner, animation, "Editor.Animation.Updated");
            }
            Controls::Tooltip(Trans::Tr("Editor.Animation.Brightness.Tooltip").c_str());
            GUI::TableNextColumn();

            if (a_editor.usesVertexColors) {
                BeginPropertyRow(Trans::Tr("Editor.Animation.ShaderColor").c_str());
                const auto shaderColorLabel = std::format("{}##AnimateShaderColor", Trans::Tr("Editor.Animation.ShaderColor.Checkbox"));
                if (GUI::Checkbox(shaderColorLabel.c_str(), &animation.useShaderColor)) {
                    Apply(scanner, animation, "Editor.Animation.Updated");
                }
                Controls::Tooltip(Trans::Tr("Editor.Animation.ShaderColor.Tooltip").c_str());
                GUI::TableNextColumn();
            }

            const auto colorEnabled = !a_editor.usesVertexColors || animation.useShaderColor;
            BeginPropertyRow(Trans::Tr("Editor.Animation.PrimaryColor").c_str());
            std::array primary{ animation.primaryColor.red, animation.primaryColor.green, animation.primaryColor.blue };
            if (!colorEnabled) {
                GUI::BeginDisabled();
            }
            GUI::SetNextItemWidth(-1.0F);
            if (GUI::ColorEdit3("##AnimationPrimaryColor", primary.data(), GUI::ImGuiColorEditFlags_DisplayRGB)) {
                animation.primaryColor = { primary[0], primary[1], primary[2], a_editor.color.alpha };
                Apply(scanner, animation, "Editor.Animation.Updated");
            }
            if (!colorEnabled) {
                GUI::EndDisabled();
            }
            Controls::Tooltip(Trans::Tr(colorEnabled ? "Editor.Animation.PrimaryColor.Tooltip" : "Editor.Animation.Color.Tooltip.Vertex").c_str());
            GUI::TableNextColumn();

            BeginPropertyRow(Trans::Tr("Editor.Animation.SecondaryColor").c_str());
            auto useSecondary = animation.useSecondaryColor;
            if (!colorEnabled) {
                GUI::BeginDisabled();
            }
            if (GUI::Checkbox("##UseAnimationSecondaryColor", &useSecondary)) {
                animation.useSecondaryColor = useSecondary;
                Apply(scanner, animation, "Editor.Animation.Updated");
            }
            if (!colorEnabled) {
                GUI::EndDisabled();
            }
            Controls::Tooltip(Trans::Tr("Editor.Animation.SecondaryColor.ToggleTooltip").c_str());
            GUI::SameLine(0.0F, 10.0F);
            if (!animation.useSecondaryColor || !colorEnabled) {
                GUI::BeginDisabled();
            }
            std::array secondary{ animation.secondaryColor.red, animation.secondaryColor.green, animation.secondaryColor.blue };
            GUI::SetNextItemWidth(-1.0F);
            if (GUI::ColorEdit3("##AnimationSecondaryColor", secondary.data(), GUI::ImGuiColorEditFlags_DisplayRGB)) {
                animation.secondaryColor = { secondary[0], secondary[1], secondary[2], a_editor.color.alpha };
                Apply(scanner, animation, "Editor.Animation.Updated");
            }
            if (!animation.useSecondaryColor || !colorEnabled) {
                GUI::EndDisabled();
            }
            Controls::Tooltip(Trans::Tr(colorEnabled ? "Editor.Animation.SecondaryColor.Tooltip" : "Editor.Animation.Color.Tooltip.Vertex").c_str());
            GUI::TableNextColumn();

            BeginPropertyRow(Trans::Tr("Editor.Animation.Variation").c_str());
            auto variationPercent = animation.variation * 100.0F;
            GUI::SetNextItemWidth(-1.0F);
            if (GUI::SliderFloat("##AnimationVariation", &variationPercent, 0.0F, 100.0F, "%.0f%%")) {
                animation.variation = variationPercent / 100.0F;
                Apply(scanner, animation, "Editor.Animation.Updated");
            }
            Controls::Tooltip(Trans::Tr("Editor.Animation.Variation.Tooltip").c_str());
            GUI::TableNextColumn();

            BeginPropertyRow(Trans::Tr("Editor.Animation.RandomPhase").c_str());
            const auto randomPhaseLabel = std::format("{}##AnimationRandomPhase", Trans::Tr("Editor.Animation.RandomPhase.Checkbox"));
            if (GUI::Checkbox(randomPhaseLabel.c_str(), &animation.randomPhase)) {
                Apply(scanner, animation, "Editor.Animation.Updated");
            }
            Controls::Tooltip(Trans::Tr("Editor.Animation.RandomPhase.Tooltip").c_str());
            GUI::TableNextColumn();

            if (!animation.enabled) {
                GUI::EndDisabled();
            }
            GUI::EndTable();
        }

        if (!a_editor.enabled) {
            GUI::EndDisabled();
        }

        GUI::Spacing();
        auto& clipboard = GetClipboard();
        if (Controls::IconCTAButton(Trans::Tr("Editor.Animation.Copy").c_str(), true, Icons::kCopy)) {
            clipboard.animation = animation;
            clipboard.available = true;
            GetEditorWindowState().status = Trans::Tr("Editor.Animation.CopySuccess");
        }
        Controls::Tooltip(Trans::Tr("Editor.Animation.Copy.Tooltip").c_str());
        GUI::SameLine(0.0F, 8.0F);
        if (Controls::IconCTAButton(Trans::Tr("Editor.Animation.Paste").c_str(), clipboard.available && a_editor.enabled, Icons::kPaste)) {
            Apply(scanner, clipboard.animation, "Editor.Animation.PasteSuccess");
        }
        Controls::Tooltip(Trans::Tr("Editor.Animation.Paste.Tooltip").c_str());

        GUI::Spacing();
        GUI::TextWrapped("%s", Trans::Tr(a_editor.nativeAnimated ? "Editor.Animation.Status.Native" : "Editor.Animation.Status.Static").c_str());
    }
}
