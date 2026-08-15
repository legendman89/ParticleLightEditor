#include "draw.hpp"

#include "logger.hpp"
#include "utility.hpp"
#include "vertices.hpp"

#include <CLibUtilsQTR/DrawDebug.hpp>

namespace ParticleLightEditor::Draw
{
    // Testing showed drawing in this plugin was interfering with my own VCD mod.
    // Sol. is to use a dedicated child canvas so clearing our drawings does not affect
    // other plugins' root-canvas graphics, and vice versa.
    bool GetCanvas(RE::GPtr<RE::GFxMovieView>& a_movie, RE::GFxValue& a_canvas)
    {
        static bool reportedUnavailable = false;
        static bool reportedReady = false;

        const auto hud = DebugAPI_IMPL::DebugAPI::GetHUD();
        if (!hud || !hud->uiMovie) {
            if (!reportedUnavailable) {
                logger::warn("Particle-light canvas unavailable: HUD movie is not ready");
                reportedUnavailable = true;
            }
            return false;
        }

        a_movie = hud->uiMovie;
        RE::GFxValue root;
        if (!a_movie->GetVariable(&root, "_root") || !root.IsDisplayObject()) {
            if (!reportedUnavailable) {
                logger::warn("Particle-light canvas unavailable: HUD root was not found");
                reportedUnavailable = true;
            }
            return false;
        }

        if (root.GetMember(kCanvasName, &a_canvas) && a_canvas.IsDisplayObject()) {
            if (!reportedReady) {
                logger::debug("Particle-light drawing canvas is ready");
                reportedReady = true;
            }
            return true;
        }

        const auto created = root.CreateEmptyMovieClip(&a_canvas, kCanvasName);
        if (created && !reportedReady) {
            logger::debug("Created dedicated particle-light drawing canvas");
            reportedReady = true;
        }
        else if (!created && !reportedUnavailable) {
            logger::warn("Particle-light canvas unavailable: child canvas creation failed");
            reportedUnavailable = true;
        }
        return created;
    }

    // Based on CLibUtilsQTR's line drawing.
    void WorldLine(const RE::GPtr<RE::GFxMovieView>& a_movie, RE::GFxValue& a_canvas, const RE::NiPoint3& a_from,
        const RE::NiPoint3& a_to, const RE::NiColorA& a_color, float a_thickness)
    {
        if (DebugAPI_IMPL::IsPosBehindPlayerCamera(a_from) && DebugAPI_IMPL::IsPosBehindPlayerCamera(a_to)) {
            return;
        }

        const auto from = DebugAPI_IMPL::DebugAPI::WorldToScreenLoc(a_movie, a_from);
        const auto to = DebugAPI_IMPL::DebugAPI::WorldToScreenLoc(a_movie, a_to);
        const auto frame = a_movie->GetVisibleFrameRect();
        if (!Utility::IsOnScreen(frame, from) && !Utility::IsOnScreen(frame, to)) {
            return;
        }

        const RE::NiColor rgb{ a_color.red, a_color.green, a_color.blue };
        const auto alpha = std::clamp(a_color.alpha * 100.0F, 0.0F, 100.0F);
        const std::array<RE::GFxValue, 3> lineStyle{ static_cast<double>(a_thickness), static_cast<double>(rgb.ToInt()), static_cast<double>(alpha) };
        const std::array<RE::GFxValue, 2> start{ static_cast<double>(from.x), static_cast<double>(from.y) };
        const std::array<RE::GFxValue, 2> end{ static_cast<double>(to.x), static_cast<double>(to.y) };

        a_canvas.Invoke("lineStyle", lineStyle);
        a_canvas.Invoke("moveTo", start);
        a_canvas.Invoke("lineTo", end);
        a_canvas.Invoke("endFill");
    }

    void Circle(const RE::GPtr<RE::GFxMovieView>& a_movie, RE::GFxValue& a_canvas, const RE::NiPoint3& a_center,
        float a_radius, const RE::NiColorA& a_color, float a_thickness, size_t a_segments, bool a_vertical)
    {
        RE::NiPoint3 previous;
        for (size_t index = 0; index <= a_segments; ++index) {
            const auto angle = static_cast<float>(index) / static_cast<float>(a_segments) * RE::NI_TWO_PI;
            const auto cosine = std::cos(angle) * a_radius;
            const auto sine = std::sin(angle) * a_radius;
            const RE::NiPoint3 current = a_vertical ? a_center + RE::NiPoint3{ cosine, 0.0F, sine } : a_center + RE::NiPoint3{ cosine, sine, 0.0F };
            if (index != 0) {
                WorldLine(a_movie, a_canvas, previous, current, a_color, a_thickness);
            }
            previous = current;
        }
    }

    void Sphere(const RE::GPtr<RE::GFxMovieView>& a_movie, RE::GFxValue& a_canvas, const RE::NiPoint3& a_center, float a_radius, const RE::NiColorA& a_color, float a_thickness, size_t a_segments)
    {
        Circle(a_movie, a_canvas, a_center, a_radius, a_color, a_thickness, a_segments, false);
        Circle(a_movie, a_canvas, a_center, a_radius, a_color, a_thickness, a_segments, true);
    }

    RE::NiColorA Color(RE::BSGeometry& a_geometry, const Entry& a_entry)
    {
        auto* shader = Utility::GetEffectShader(a_geometry);
        auto* material = shader ? shader->GetMaterial() : nullptr;
        if (!material) {
            return kDefaultDrawColor;
        }

        const auto& sourceColor = a_entry.defaults.usesVertexColors && Vertices::IsBlackOrWhite(material->baseColor) ?
            a_entry.currentColor : material->baseColor;
        RE::NiColorA color{ sourceColor.red * material->baseColorScale, sourceColor.green * material->baseColorScale,
            sourceColor.blue * material->baseColorScale, 0.9F };
        if (shader->emittanceColor) {
            color.red *= shader->emittanceColor->red;
            color.green *= shader->emittanceColor->green;
            color.blue *= shader->emittanceColor->blue;
        }

        const auto brightest = std::max({ color.red, color.green, color.blue });
        if (!std::isfinite(brightest) || brightest <= 0.01F) {
            return kDefaultDrawColor;
        }

        const auto normalization = brightest > 1.0F ? 1.0F / brightest : 1.0F;
        color.red = std::clamp(color.red * normalization, 0.0F, 1.0F);
        color.green = std::clamp(color.green * normalization, 0.0F, 1.0F);
        color.blue = std::clamp(color.blue * normalization, 0.0F, 1.0F);
        return color;
    }

    void Lights(const std::vector<Entry>& a_entries, size_t a_selectedIndex, RE::PlayerCharacter* a_player, const Settings::RuntimeSettings& a_settings, DrawState& a_state)
    {
        if (!a_player) {
            return;
        }

        RE::GPtr<RE::GFxMovieView> movie;
        RE::GFxValue canvas;
        if (!GetCanvas(movie, canvas)) {
            return;
        }

        if (canvas.Invoke("clear")) {
            ++a_state.counters.successfulClearCount;
            a_state.reportedClearFailure = false;
        }
        else {
            ++a_state.counters.failedClearCount;
            if (!a_state.reportedClearFailure) {
                logger::warn("Particle-light canvas clear failed; stale debug lines may remain visible");
                a_state.reportedClearFailure = true;
            }
            a_state.counters.drawnCount = 0;
            return;
        }

        a_state.counters.drawnCount = 0;
        if (!a_settings.drawLights) {
            return;
        }

        const auto playerPosition = a_player->GetPosition();
        const auto rangeSquared = a_settings.drawRange * a_settings.drawRange;
        const auto radiusScale = std::clamp(a_settings.drawRadiusScale, 0.05F, 2.0F);
        const auto segments = static_cast<size_t>(std::clamp(a_settings.circleSegments, 4, 48));
        const RE::NiColorA centerColor{ a_settings.centerMarkerColor[0], a_settings.centerMarkerColor[1], a_settings.centerMarkerColor[2], a_settings.centerMarkerColor[3] };
        const RE::NiColorA selectedColor{ a_settings.selectedHighlightColor[0], a_settings.selectedHighlightColor[1], a_settings.selectedHighlightColor[2], a_settings.selectedHighlightColor[3] };
        size_t invalidRadiusCount = 0;
        size_t outOfRangeCount = 0;

        for (size_t index = 0; index < a_entries.size(); ++index) {
            const auto& entry = a_entries[index];
            if (a_settings.drawOnlySelectedLight && index != a_selectedIndex) {
                continue;
            }
            if ((entry.validatedByName && !a_settings.showNameValidated) || (!entry.validatedByName && !a_settings.showRuntimeValidated)) {
                continue;
            }

            auto* geometry = entry.geometry.get();
            if (!geometry) {
                continue;
            }

            const auto particleRadius = geometry->worldBound.radius * 0.5F;
            if (!std::isfinite(particleRadius) || particleRadius <= 0.0F) {
                ++invalidRadiusCount;
                continue;
            }

            const auto center = geometry->worldBound.center;
            const auto distanceSquared = Utility::DistanceSquared(center, playerPosition);
            if (!std::isfinite(distanceSquared) || distanceSquared > rangeSquared) {
                ++outOfRangeCount;
                if (a_state.summaryPending && std::isfinite(distanceSquared) && distanceSquared <= rangeSquared * 4.0F) {
                    logger::debug("Near-range particle light excluded: ownerRef={:08X}, associatedLightRef={:08X}, node='{}', distance={:.2f}, range={:.2f}",
                        entry.ownerFormID, entry.associatedLightRefID, entry.nodeName, std::sqrt(distanceSquared), a_settings.drawRange);
                }
                continue;
            }

            const auto radius = particleRadius * radiusScale;
            const auto particleColor = Color(*geometry, entry);
            ++a_state.counters.drawnCount;
            Sphere(movie, canvas, center, radius, particleColor, a_settings.lineThickness, segments);
            if (a_settings.highlightSelectedLight && index == a_selectedIndex) {
                Sphere(movie, canvas, center, radius + std::max(2.0F, radius * 0.01F), selectedColor, a_settings.lineThickness + 0.75F, segments);
            }
            if (a_settings.drawCenterMarkers) {
                Sphere(movie, canvas, center, a_settings.centerMarkerRadius, centerColor, a_settings.lineThickness, segments);
            }
        }

        if (a_state.summaryPending) {
            logger::debug("Drawing {} particle light(s) at {:.2f} radius scale; {} invalid radius, {} outside {:.0f}-unit range",
                a_state.counters.drawnCount, radiusScale, invalidRadiusCount, outOfRangeCount, a_settings.drawRange);
            a_state.summaryPending = false;
        }
    }
}
