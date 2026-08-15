#pragma once

#include "types.hpp"

namespace ParticleLightEditor::Animation
{
    void Advance(float a_delta);

    void CaptureDefault(Entry& a_entry);

    AnimationEdit MakeDefault(const ParticleDefault& a_default, ParticleCategory a_category);

    AnimationProfile SuggestedProfile(ParticleCategory a_category);

    bool Apply(Entry& a_entry, const Edit& a_edit);

    void Restore(Entry& a_entry, const Edit& a_edit);

    void RestoreDefault(Entry& a_entry, const Edit& a_edit);

    bool IsFinite(const AnimationEdit& a_animation);

    bool IsValid(const AnimationEdit& a_animation);

    bool Equal(const AnimationEdit& a_left, const AnimationEdit& a_right);
}
