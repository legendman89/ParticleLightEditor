#include "menu.hpp"

#include "controls.hpp"
#include "scanner.hpp"
#include "settings.hpp"

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
        const auto saveWidth = Controls::IconCTAButtonWidth("Save Settings");
        const auto defaultsWidth = Controls::IconCTAButtonWidth("Load Defaults");
        Controls::AlignActions(saveWidth + defaultsWidth + spacing);

        GUI::Spacing();
        
        if (Controls::IconCTAButton("Save Settings", Settings::IsToolsDirty(), Icons::kSave)) {
            Settings::Save();
        }
        Controls::Tooltip("Save visualization, overlay, and logging options.");
        GUI::SameLine(0.0F, spacing);

        if (Controls::IconCTAButton("Load Defaults", !Settings::IsToolsDefault(), Icons::kReset)) {
            Settings::ResetToolsDefaults();
        }
        Controls::Tooltip("Restore default visualization, overlay, and logging options.");
        Controls::FinishActions();
    }

    void RenderToolsSections()
    {
        auto& settings = Settings::GetSettings();
        const auto stats = Scanner::GetSingleton().GetStats();
        constexpr auto defaultOpen = GUI::ImGuiTreeNodeFlags_DefaultOpen;

        if (GUI::CollapsingHeader("Visualization", defaultOpen)) {

            GUI::Spacing();

            GUI::Checkbox("Draw particle lights", &settings.drawLights);

            GUI::Spacing();

            GUI::SliderFloat("Radius scale", &settings.drawRadiusScale, 0.05F, 2.0F, "%.2f");

            GUI::Spacing();

            GUI::SliderFloat("Line thickness", &settings.lineThickness, 0.25F, 5.0F, "%.2f");

            GUI::Spacing();

            GUI::SliderInt("Circle segments", &settings.circleSegments, 4, 48);
        }

        GUI::Spacing();
        GUI::Spacing();

        if (GUI::CollapsingHeader("Overlay Details", defaultOpen)) {

            GUI::Spacing();

            constexpr auto colorEditFlags = GUI::ImGuiColorEditFlags_DisplayRGB | GUI::ImGuiColorEditFlags_AlphaBar;
            GUI::Checkbox("Draw center markers", &settings.drawCenterMarkers);

            GUI::Spacing();

            GUI::SliderFloat("Center marker radius", &settings.centerMarkerRadius, 1.0F, 32.0F, "%.1f");

            GUI::Spacing();

            GUI::SetNextItemWidth(280.0F);

            GUI::ColorEdit4("##CenterMarkerColor", settings.centerMarkerColor.data(), colorEditFlags);
            GUI::SameLine();
            GUI::TextUnformatted("Center marker color");

            GUI::Spacing();

            GUI::Checkbox("Highlight selected light", &settings.highlightSelectedLight);

            GUI::Spacing();

            GUI::SetNextItemWidth(280.0F);
            
            GUI::ColorEdit4("##SelectedHighlightColor", settings.selectedHighlightColor.data(), colorEditFlags);
            GUI::SameLine();
            GUI::TextUnformatted("Selected highlight color");
        }

        GUI::Spacing();
        GUI::Spacing();

        if (GUI::CollapsingHeader("Diagnostics", defaultOpen)) {

            GUI::Spacing();

            constexpr auto tableFlags = GUI::ImGuiTableFlags_SizingFixedFit | GUI::ImGuiTableFlags_NoSavedSettings;
            if (GUI::BeginTable("DiagnosticsStats", 4, tableFlags)) {
                GUI::TableSetupColumn("Metric A", GUI::ImGuiTableColumnFlags_WidthFixed, 200.0F);
                GUI::TableSetupColumn("Value A", GUI::ImGuiTableColumnFlags_WidthFixed, 90.0F);
                GUI::TableSetupColumn("Metric B", GUI::ImGuiTableColumnFlags_WidthFixed, 200.0F);
                GUI::TableSetupColumn("Value B", GUI::ImGuiTableColumnFlags_WidthFixed, 90.0F);

                GUI::TableNextRow();
                RenderDiagnosticMetric("Cached lights", stats.cachedLights);
                RenderDiagnosticMetric("Drawn lights", stats.draw.drawnCount);
                GUI::TableNextRow();
                RenderDiagnosticMetric("In detection range", Scanner::GetSingleton().GetLightCount());
                RenderDiagnosticMetric("Cell references", stats.scan.referenceCount);
                GUI::TableNextRow();
                RenderDiagnosticMetric("Placed light sources", stats.scan.sourceCount);
                RenderDiagnosticMetric("Named candidates", stats.scan.nameValidatedCount);
                GUI::TableNextRow();
                RenderDiagnosticMetric("Structural candidates", stats.scan.structuralCandidateCount);
                RenderDiagnosticMetric("Spatial matches", stats.scan.spatialMatchCount);
                GUI::TableNextRow();
                RenderDiagnosticMetric("Unmatched structural", stats.scan.unmatchedStructuralCount);
                RenderDiagnosticMetric("Rejected glow", stats.scan.rejectedGlowCount);
                GUI::TableNextRow();
                RenderDiagnosticMetric("Rejected shader", stats.scan.rejectedShaderCount);
                RenderDiagnosticMetric("Rejected topology", stats.scan.rejectedTopologyCount);
                GUI::EndTable();
            }

            GUI::Checkbox("Show node-name validated lights", &settings.showNameValidated);
            GUI::Checkbox("Show spatially validated lights", &settings.showRuntimeValidated);

            GUI::Spacing();
            GUI::Spacing();

            if (Controls::CTAButton("Rescan Cell", true)) {
                Scanner::GetSingleton().RequestRescan(true);
            }
            Controls::Tooltip("Discard the current scene cache and scan the attached cell again.");
        }

        GUI::Spacing();
        GUI::Spacing();

        if (GUI::CollapsingHeader("Logging", defaultOpen)) {

            GUI::Spacing();

            auto logLevel = Settings::NormalizeLogLevel(settings.logLevel);
            GUI::SetNextItemWidth(180.0F);
            if (GUI::Combo("Log level", &logLevel, kLogLevels, static_cast<int>(std::size(kLogLevels)))) {
                settings.logLevel = logLevel;
                Settings::ApplyLogLevel(logLevel);
            }
            Controls::Tooltip("Set the minimum severity written to the Particle Light Editor log. Debug includes canvas diagnostics.");
        }
    }

    void __stdcall RenderTools()
    {
        RenderToolsActions();
        GUI::ImVec2 available{};
        GUI::GetContentRegionAvail(&available);
        if (GUI::BeginChild("ToolsScrollRegion", available, GUI::ImGuiChildFlags_None, 0)) {
            RenderToolsSections();
        }
        GUI::EndChild();
    }
}
