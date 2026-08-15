#include "menu.hpp"

#include "controls.hpp"
#include "scanner.hpp"
#include "settings.hpp"
#include "translate.hpp"

namespace ParticleLightEditor::Menu
{
    void RenderDiagnosticMetric(const char* a_label, size_t a_value)
    {
        GUI::TableNextColumn();
        GUI::AlignTextToFramePadding();
        GUI::TextUnformatted(a_label);
        GUI::TableNextColumn();
        GUI::Text("%llu", static_cast<unsigned long long>(a_value));
    }

    
    void RenderToolsActions()
    {
        constexpr auto spacing = 8.0F;
        const auto& saveLabel = Trans::Tr("Common.SaveSettings");
        const auto& defaultsLabel = Trans::Tr("Common.LoadDefaults");
        const auto saveWidth = Controls::IconCTAButtonWidth(saveLabel.c_str(), Icons::kSave);
        const auto defaultsWidth = Controls::IconCTAButtonWidth(defaultsLabel.c_str(), Icons::kReset);
        Controls::AlignActions(saveWidth + defaultsWidth + spacing);

        GUI::Spacing();
        
        if (Controls::IconCTAButton(saveLabel.c_str(), Settings::IsToolsDirty(), Icons::kSave)) {
            Settings::Save();
        }
        Controls::Tooltip(Trans::Tr("Tools.Save.Tooltip").c_str());
        GUI::SameLine(0.0F, spacing);

        if (Controls::IconCTAButton(defaultsLabel.c_str(), !Settings::IsToolsDefault(), Icons::kReset)) {
            Settings::ResetToolsDefaults();
        }
        Controls::Tooltip(Trans::Tr("Tools.Defaults.Tooltip").c_str());
        Controls::FinishActions();
    }

    void RenderToolsSections()
    {
        auto& settings = Settings::GetSettings();
        auto& scanner = Scanner::GetSingleton();
        const auto stats = scanner.GetStats();
        constexpr auto defaultOpen = GUI::ImGuiTreeNodeFlags_DefaultOpen;

        const auto visualizationHeader = std::format("{}##Visualization", Trans::Tr("Tools.Visualization.Header"));
        if (GUI::CollapsingHeader(visualizationHeader.c_str(), defaultOpen)) {

            GUI::Spacing();

            const auto drawLabel = std::format("{}##DrawParticleLights", Trans::Tr("Tools.Visualization.Draw"));
            GUI::Checkbox(drawLabel.c_str(), &settings.drawLights);

            GUI::Spacing();

            const auto selectedOnlyLabel = std::format("{}##DrawOnlySelectedLight", Trans::Tr("Tools.Visualization.DrawSelectedOnly"));
            const auto hasSelection = scanner.GetSelectedLightIndex() < scanner.GetLightCount();
            if (!settings.drawLights || !hasSelection) {
                GUI::BeginDisabled();
            }
            GUI::Checkbox(selectedOnlyLabel.c_str(), &settings.drawOnlySelectedLight);
            if (!settings.drawLights || !hasSelection) {
                GUI::EndDisabled();
            }
            Controls::Tooltip(Trans::Tr("Tools.Visualization.DrawSelectedOnly.Tooltip").c_str());

            GUI::Spacing();

            const auto radiusLabel = std::format("{}##DrawRadiusScale", Trans::Tr("Tools.Visualization.RadiusScale"));
            GUI::SliderFloat(radiusLabel.c_str(), &settings.drawRadiusScale, 0.05F, 2.0F, "%.2f");

            GUI::Spacing();

            const auto thicknessLabel = std::format("{}##LineThickness", Trans::Tr("Tools.Visualization.LineThickness"));
            GUI::SliderFloat(thicknessLabel.c_str(), &settings.lineThickness, 0.25F, 5.0F, "%.2f");

            GUI::Spacing();

            const auto segmentsLabel = std::format("{}##CircleSegments", Trans::Tr("Tools.Visualization.CircleSegments"));
            GUI::SliderInt(segmentsLabel.c_str(), &settings.circleSegments, 4, 48);
        }

        GUI::Spacing();
        GUI::Spacing();

        const auto overlayHeader = std::format("{}##OverlayDetails", Trans::Tr("Tools.Overlay.Header"));
        if (GUI::CollapsingHeader(overlayHeader.c_str(), defaultOpen)) {

            GUI::Spacing();

            constexpr auto colorEditFlags = GUI::ImGuiColorEditFlags_DisplayRGB | GUI::ImGuiColorEditFlags_AlphaBar;
            const auto centersLabel = std::format("{}##DrawCenterMarkers", Trans::Tr("Tools.Overlay.DrawCenters"));
            GUI::Checkbox(centersLabel.c_str(), &settings.drawCenterMarkers);

            GUI::Spacing();

            const auto centerRadiusLabel = std::format("{}##CenterMarkerRadius", Trans::Tr("Tools.Overlay.CenterRadius"));
            GUI::SliderFloat(centerRadiusLabel.c_str(), &settings.centerMarkerRadius, 1.0F, 32.0F, "%.1f");

            GUI::Spacing();

            GUI::SetNextItemWidth(280.0F);

            GUI::ColorEdit4("##CenterMarkerColor", settings.centerMarkerColor.data(), colorEditFlags);
            GUI::SameLine();
            GUI::TextUnformatted(Trans::Tr("Tools.Overlay.CenterColor").c_str());

            GUI::Spacing();

            const auto highlightLabel = std::format("{}##HighlightSelectedLight", Trans::Tr("Tools.Overlay.Highlight"));
            GUI::Checkbox(highlightLabel.c_str(), &settings.highlightSelectedLight);

            GUI::Spacing();

            GUI::SetNextItemWidth(280.0F);
            
            GUI::ColorEdit4("##SelectedHighlightColor", settings.selectedHighlightColor.data(), colorEditFlags);
            GUI::SameLine();
            GUI::TextUnformatted(Trans::Tr("Tools.Overlay.HighlightColor").c_str());
        }

        GUI::Spacing();
        GUI::Spacing();

        const auto diagnosticsHeader = std::format("{}##Diagnostics", Trans::Tr("Tools.Diagnostics.Header"));
        if (GUI::CollapsingHeader(diagnosticsHeader.c_str())) {

            GUI::Spacing();

            constexpr auto tableFlags = GUI::ImGuiTableFlags_SizingFixedFit | GUI::ImGuiTableFlags_NoSavedSettings;
            if (GUI::BeginTable("DiagnosticsStats", 4, tableFlags)) {
                GUI::TableSetupColumn("##MetricA", GUI::ImGuiTableColumnFlags_WidthFixed, 220.0F);
                GUI::TableSetupColumn("##ValueA", GUI::ImGuiTableColumnFlags_WidthFixed, 90.0F);
                GUI::TableSetupColumn("##MetricB", GUI::ImGuiTableColumnFlags_WidthFixed, 220.0F);
                GUI::TableSetupColumn("##ValueB", GUI::ImGuiTableColumnFlags_WidthFixed, 90.0F);

                GUI::TableNextRow();
                RenderDiagnosticMetric(Trans::Tr("Tools.Diagnostics.CachedLights").c_str(), stats.cachedLights);
                RenderDiagnosticMetric(Trans::Tr("Tools.Diagnostics.DrawnLights").c_str(), stats.draw.drawnCount);
                GUI::TableNextRow();
                RenderDiagnosticMetric(Trans::Tr("Tools.Diagnostics.InRange").c_str(), Scanner::GetSingleton().GetLightCount());
                RenderDiagnosticMetric(Trans::Tr("Tools.Diagnostics.CellReferences").c_str(), stats.scan.referenceCount);
                GUI::TableNextRow();
                RenderDiagnosticMetric(Trans::Tr("Tools.Diagnostics.LightSources").c_str(), stats.scan.sourceCount);
                RenderDiagnosticMetric(Trans::Tr("Tools.Diagnostics.NamedCandidates").c_str(), stats.scan.nameValidatedCount);
                GUI::TableNextRow();
                RenderDiagnosticMetric(Trans::Tr("Tools.Diagnostics.StructuralCandidates").c_str(), stats.scan.structuralCandidateCount);
                RenderDiagnosticMetric(Trans::Tr("Tools.Diagnostics.SourceMatches").c_str(), stats.scan.sourceMatchCount);
                GUI::TableNextRow();
                RenderDiagnosticMetric(Trans::Tr("Tools.Diagnostics.UnmatchedStructural").c_str(), stats.scan.unmatchedStructuralCount);
                RenderDiagnosticMetric(Trans::Tr("Tools.Diagnostics.RejectedGlow").c_str(), stats.scan.rejectedGlowCount);
                GUI::TableNextRow();
                RenderDiagnosticMetric(Trans::Tr("Tools.Diagnostics.RejectedShader").c_str(), stats.scan.rejectedShaderCount);
                RenderDiagnosticMetric(Trans::Tr("Tools.Diagnostics.RejectedTopology").c_str(), stats.scan.rejectedTopologyCount);
                GUI::EndTable();
            }

            const auto namedLabel = std::format("{}##ShowNameValidated", Trans::Tr("Tools.Diagnostics.ShowNamed"));
            const auto matchedLabel = std::format("{}##ShowRuntimeValidated", Trans::Tr("Tools.Diagnostics.ShowMatched"));
            GUI::Checkbox(namedLabel.c_str(), &settings.showNameValidated);
            GUI::Checkbox(matchedLabel.c_str(), &settings.showRuntimeValidated);

            GUI::Spacing();
            GUI::Spacing();

            if (Controls::CTAButton(Trans::Tr("Tools.Diagnostics.Rescan").c_str(), true)) {
                Scanner::GetSingleton().RequestRescan(true);
            }
            Controls::Tooltip(Trans::Tr("Tools.Diagnostics.Rescan.Tooltip").c_str());
        }

        GUI::Spacing();
        GUI::Spacing();

        const auto loggingHeader = std::format("{}##Logging", Trans::Tr("Tools.Logging.Header"));
        if (GUI::CollapsingHeader(loggingHeader.c_str(), defaultOpen)) {

            GUI::Spacing();

            auto logLevel = Settings::NormalizeLogLevel(settings.logLevel);
            const std::array logLevels{
                Trans::Tr("Log.Trace").c_str(),
                Trans::Tr("Log.Debug").c_str(),
                Trans::Tr("Log.Info").c_str(),
                Trans::Tr("Log.Warning").c_str(),
                Trans::Tr("Log.Error").c_str(),
                Trans::Tr("Log.Critical").c_str(),
                Trans::Tr("Log.Off").c_str()
            };
            GUI::SetNextItemWidth(180.0F);
            const auto logLevelLabel = std::format("{}##LogLevel", Trans::Tr("Tools.Logging.Level"));
            if (GUI::Combo(logLevelLabel.c_str(), &logLevel, logLevels.data(), static_cast<int>(logLevels.size()))) {
                settings.logLevel = logLevel;
                Settings::ApplyLogLevel(logLevel);
            }
            Controls::Tooltip(Trans::Tr("Tools.Logging.Level.Tooltip").c_str());
        }
    }

    void __stdcall RenderTools()
    {
        RenderToolsActions();
        const auto available = GUI::GetContentRegionAvail();
        if (GUI::BeginChild("ToolsScrollRegion", available, GUI::ImGuiChildFlags_None, 0)) {
            RenderToolsSections();
        }
        GUI::EndChild();
    }
}
