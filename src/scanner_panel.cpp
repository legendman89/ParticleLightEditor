#include "menu.hpp"

#include "controls.hpp"
#include "editor_window.hpp"
#include "scanner.hpp"
#include "settings.hpp"
#include "translate.hpp"

namespace ParticleLightEditor::Menu
{
    void RenderScannerActions()
    {
        constexpr auto spacing = 8.0F;
        const auto& saveLabel = Trans::Tr("Common.SaveSettings");
        const auto& defaultsLabel = Trans::Tr("Common.LoadDefaults");
        const auto saveWidth = Controls::IconCTAButtonWidth(saveLabel.c_str(), Icons::kSave);
        const auto defaultsWidth = Controls::IconCTAButtonWidth(defaultsLabel.c_str(), Icons::kReset);
        Controls::AlignActions(saveWidth + defaultsWidth + spacing);

        GUI::Spacing();

        if (Controls::IconCTAButton(saveLabel.c_str(), Settings::IsScannerDirty(), Icons::kSave)) {
            Settings::Save();
        }

        Controls::Tooltip(Trans::Tr("Scanner.Save.Tooltip").c_str());

        GUI::SameLine(0.0F, spacing);

        if (Controls::IconCTAButton(defaultsLabel.c_str(), !Settings::IsScannerDefault(), Icons::kReset)) {
            Settings::ResetScannerDefaults();
            Scanner::GetSingleton().RequestRescan(true);
        }
        Controls::Tooltip(Trans::Tr("Scanner.Defaults.Tooltip").c_str());
        Controls::FinishActions();
    }

    void RenderScannerSections()
    {
        auto& settings = Settings::GetSettings();
        auto& scanner = Scanner::GetSingleton();
        const auto stats = scanner.GetStats();
        constexpr auto defaultOpen = GUI::ImGuiTreeNodeFlags_DefaultOpen;

        const auto selectionHeader = std::format("{}##LightSelection", Trans::Tr("Scanner.Selection.Header"));
        if (GUI::CollapsingHeader(selectionHeader.c_str(), defaultOpen)) {

            GUI::Spacing();

            GUI::TextUnformatted(Trans::Tr("Scanner.Selection.Info").c_str());
            if (!GetState().consoleStatus.empty()) {
                GUI::TextWrapped("%s", GetState().consoleStatus.c_str());
            }
            
            GUI::Spacing();

            RenderSelection();

            GUI::Spacing();

            const auto glowLabel = std::format("{}##IncludeGlowNodes", Trans::Tr("Scanner.IncludeGlowNodes"));
            if (GUI::Checkbox(glowLabel.c_str(), &settings.includeGlowNodes)) {
                scanner.RequestRescan();
            }

            const auto consoleLabel = std::format("{}##OpenEditorAfterConsoleSelection", Trans::Tr("Scanner.OpenAfterConsole"));
            GUI::Checkbox(consoleLabel.c_str(), &settings.openEditorAfterConsoleSelection);
            Controls::Tooltip(Trans::Tr("Scanner.OpenAfterConsole.Tooltip").c_str());
            
            GUI::Spacing();

            const auto rangeLabel = std::format("{}##DetectionRange", Trans::Tr("Scanner.DetectionRange"));
            GUI::SliderFloat(rangeLabel.c_str(), &settings.drawRange, 2.0F, 8192.0F, Trans::Tr("Common.Units.Format").c_str());

            GUI::Spacing();

            const auto scanningLabel = std::format("{}##PeriodicScanning", Trans::Tr("Scanner.PeriodicScanning"));
            GUI::SliderFloat(scanningLabel.c_str(), &settings.scanInterval, 0.0F, 10.0F, Trans::Tr("Common.Seconds.Format").c_str());
            Controls::Tooltip(Trans::Tr("Scanner.PeriodicScanning.Tooltip").c_str());

            GUI::Spacing();
            GUI::Spacing();

            GUI::TextUnformatted(Trans::Format("Scanner.Cache", stats.cachedLights, stats.scan.referenceCount, stats.scan.sourceCount).c_str());
        }

        GUI::Spacing();
        GUI::Spacing();

        const auto associationHeader = std::format("{}##SourceAssociation", Trans::Tr("Scanner.Association.Header"));
        if (GUI::CollapsingHeader(associationHeader.c_str(), defaultOpen)) {

            GUI::Spacing();

            bool changed = false;
            const auto associationLabel = std::format("{}##AssociationRange", Trans::Tr("Scanner.Association.Range"));
            changed |= GUI::SliderFloat(associationLabel.c_str(), &settings.associationRange, 8.0F, 512.0F, Trans::Tr("Common.Units.Format").c_str());
            Controls::Tooltip(Trans::Tr("Scanner.Association.Range.Tooltip").c_str());
            
            GUI::Spacing();

            const auto influenceLabel = std::format("{}##RadiusMatchingInfluence", Trans::Tr("Scanner.Association.RadiusInfluence"));
            changed |= GUI::SliderFloat(influenceLabel.c_str(), &settings.radiusMatchWeight, 0.0F, 2.0F, "%.2f");
            Controls::Tooltip(Trans::Tr("Scanner.Association.RadiusInfluence.Tooltip").c_str());
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
