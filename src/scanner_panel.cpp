#include "menu.hpp"

#include "controls.hpp"
#include "scanner.hpp"
#include "settings.hpp"

namespace ParticleLightEditor::Menu
{
    void RenderScannerActions()
    {
        constexpr auto spacing = 8.0F;
        const auto saveWidth = Controls::IconCTAButtonWidth("Save Settings");
        const auto defaultsWidth = Controls::IconCTAButtonWidth("Load Defaults");
        Controls::AlignActions(saveWidth + defaultsWidth + spacing);

        GUI::Spacing();

        if (Controls::IconCTAButton("Save Settings", Settings::IsScannerDirty(), Icons::kSave)) {
            Settings::Save();
        }

        Controls::Tooltip("Save scanner options and all edited particle lights.");

        GUI::SameLine(0.0F, spacing);

        if (Controls::IconCTAButton("Load Defaults", !Settings::IsScannerDefault(), Icons::kReset)) {
            Settings::ResetScannerDefaults();
            Scanner::GetSingleton().RequestRescan(true);
        }
        Controls::Tooltip("Restore default scanner and association options.");
        Controls::FinishActions();
    }

    void RenderScannerSections()
    {
        auto& settings = Settings::GetSettings();
        auto& scanner = Scanner::GetSingleton();
        const auto stats = scanner.GetStats();
        constexpr auto defaultOpen = GUI::ImGuiTreeNodeFlags_DefaultOpen;

        if (GUI::CollapsingHeader("Particle Lights", defaultOpen)) {

            GUI::Spacing();

            GUI::Text("%llu particle light(s) are available inside the current detection range.", scanner.GetLightCount());
            GUI::Text("Edits are applied live and restored after loading when saved.");
            if (!GetState().consoleStatus.empty()) {
                GUI::TextWrapped("%s", GetState().consoleStatus.c_str());
            }
            
            GUI::Spacing();

            RenderSelection();

            GUI::Spacing();

            if (GUI::Checkbox("Include glow-named nodes", &settings.includeGlowNodes)) {
                scanner.RequestRescan();
            }
            GUI::Checkbox("Open Editor after Console Selection", &settings.openEditorAfterConsoleSelection);
            Controls::Tooltip("The console-selected particle light always appears in the combo when the console closes. Enable this to also open its editor window.");
            
            GUI::Spacing();

            GUI::SliderFloat("Detection range", &settings.drawRange, 2.0F, 8192.0F, "%.0f units");

            GUI::Spacing();

            GUI::SliderFloat("Periodic Scanning", &settings.scanInterval, 0.0F, 10.0F, "%.1f s");
            Controls::Tooltip("Zero disables periodic scanning. Cell changes and manual rescans still refresh immediately.");

            GUI::Spacing();

            GUI::Text(
                "Current cache: %llu lights from %llu cell references and %llu placed light sources.",
                stats.cachedLights,
                stats.scan.referenceCount,
                stats.scan.sourceCount);
        }

        GUI::Spacing();
        GUI::Spacing();

        if (GUI::CollapsingHeader("Source Association", defaultOpen)) {

            GUI::Spacing();

            bool changed = false;
            changed |= GUI::SliderFloat("Association range", &settings.associationRange, 8.0F, 512.0F, "%.0f units");
            Controls::Tooltip("Maximum world-space distance between a particle light and a placed Skyrim light source for association. Increasing it can match more lights, but values that are too high may associate unrelated nearby sources.");
            
            GUI::Spacing();

            changed |= GUI::SliderFloat("Radius matching influence", &settings.radiusMatchWeight, 0.0F, 2.0F, "%.2f");
            Controls::Tooltip("Tie-breaker used when multiple light sources are nearby. Zero uses distance only; higher values prefer sources whose light radius is more similar to the particle radius. This does not change either radius.");
            if (changed) {
                scanner.RequestRescan();
            }
        }

    }

    void __stdcall RenderScanner()
    {
        RenderScannerActions();
        GUI::ImVec2 available{};
        GUI::GetContentRegionAvail(&available);
        if (GUI::BeginChild("ScannerScrollRegion", available, GUI::ImGuiChildFlags_None, 0)) {
            RenderScannerSections();
        }
        GUI::EndChild();
    }
}
