#pragma once

#include "settings.hpp"
#include "types.hpp"

namespace ParticleLightEditor::Draw
{
    bool GetCanvas(RE::GPtr<RE::GFxMovieView>& a_movie, RE::GFxValue& a_canvas);

    void WorldLine(const RE::GPtr<RE::GFxMovieView>& a_movie, RE::GFxValue& a_canvas, const RE::NiPoint3& a_from,
        const RE::NiPoint3& a_to, const RE::NiColorA& a_color, float a_thickness);

    void Circle(const RE::GPtr<RE::GFxMovieView>& a_movie, RE::GFxValue& a_canvas, const RE::NiPoint3& a_center,
        float a_radius, const RE::NiColorA& a_color, float a_thickness, size_t a_segments, bool a_vertical);

    void Sphere(const RE::GPtr<RE::GFxMovieView>& a_movie, RE::GFxValue& a_canvas, const RE::NiPoint3& a_center,
        float a_radius, const RE::NiColorA& a_color, float a_thickness, size_t a_segments);

    RE::NiColorA Color(RE::BSGeometry& a_geometry, const Entry& a_entry);
    
    void Lights(const std::vector<Entry>& a_entries, size_t a_selectedIndex, RE::PlayerCharacter* a_player,
        const Settings::RuntimeSettings& a_settings, DrawState& a_state);
}
