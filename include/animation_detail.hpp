#pragma once

#include "editor.hpp"
#include "utility.hpp"

#define ANIMATION_PROFILE_DURATION(PROFILE, LABEL, DURATION) case AnimationProfile::PROFILE: return DURATION;

namespace ParticleLightEditor::Animation
{
    inline constexpr float kPi = 3.14159265358979323846F;
    inline constexpr uint32_t kSpeedVariationSeed = 0xA341316CU;
    inline constexpr uint32_t kPhaseSeed = 0xC8013EA4U;
    inline constexpr uint32_t kBrightnessVariationSeed = 0xAD90777DU;
    inline float elapsedTime{ 0.0F };

    inline float Brightness(const RE::NiPoint3& a_color)
    {
        return std::max({ a_color.x, a_color.y, a_color.z });
    }

    inline float Linear(float a_left, float a_right, float a_amount)
    {
        return a_left + (a_right - a_left) * a_amount;
    }

    inline RE::NiColorA Linear(const RE::NiColorA& a_left, const RE::NiColorA& a_right, float a_amount)
    {
        return {
            Linear(a_left.red, a_right.red, a_amount),
            Linear(a_left.green, a_right.green, a_amount),
            Linear(a_left.blue, a_right.blue, a_amount),
            Linear(a_left.alpha, a_right.alpha, a_amount)
        };
    }

    inline float Wrap(float a_value)
    {
        const auto wrapped = a_value - std::floor(a_value);
        return wrapped < 0.0F ? wrapped + 1.0F : wrapped;
    }

    // Lowbias32 integer mixer: https://nullprogram.com/blog/2018/07/31/
    inline uint32_t MixInteger(uint32_t a_value)
    {
        a_value ^= a_value >> 16;
        a_value *= 0x7FEB352DU;
        a_value ^= a_value >> 15;
        a_value *= 0x846CA68BU;
        a_value ^= a_value >> 16;
        return a_value;
    }

    inline float Hash01(const Entry& a_entry, uint32_t a_seed)
    {
        const auto value = MixInteger(static_cast<uint32_t>(a_entry.ownerFormID) ^ static_cast<uint32_t>(a_entry.particleOrdinal * 0x9E3779B9ULL) ^ a_seed);
        return static_cast<float>(value & 0x00FFFFFFU) / static_cast<float>(0x01000000U);
    }

    inline float EvaluatePoints(const AnimationPoint* a_points, size_t a_count, float a_phase)
    {
        if (!a_points || a_count == 0) {
            return 1.0F;
        }
        if (a_count == 1 || a_phase <= a_points[0].time) {
            return Brightness(a_points[0].color);
        }
        for (size_t index = 1; index < a_count; ++index) {
            if (a_phase <= a_points[index].time) {
                const auto range = a_points[index].time - a_points[index - 1].time;
                const auto amount = range > 0.0001F ? (a_phase - a_points[index - 1].time) / range : 0.0F;
                return Linear(Brightness(a_points[index - 1].color), Brightness(a_points[index].color), std::clamp(amount, 0.0F, 1.0F));
            }
        }
        return Brightness(a_points[a_count - 1].color);
    }

    inline float EvaluateOriginal(const AnimationDefault& a_default, float a_phase)
    {
        if (a_default.points.empty()) {
            return 1.0F;
        }

        auto minimum = std::numeric_limits<float>::max();
        auto maximum = std::numeric_limits<float>::lowest();
        for (const auto& point : a_default.points) {
            const auto brightness = Brightness(point.color);
            minimum = std::min(minimum, brightness);
            maximum = std::max(maximum, brightness);
        }

        const auto time = a_default.points.front().time + a_phase * a_default.duration;
        AnimationPoint samplePoints[2];
        auto sampled = Brightness(a_default.points.back().color);
        for (size_t index = 1; index < a_default.points.size(); ++index) {
            if (time <= a_default.points[index].time) {
                samplePoints[0] = a_default.points[index - 1];
                samplePoints[1] = a_default.points[index];
                samplePoints[0].time = 0.0F;
                samplePoints[1].time = 1.0F;
                const auto range = a_default.points[index].time - a_default.points[index - 1].time;
                const auto amount = range > 0.0001F ? (time - a_default.points[index - 1].time) / range : 0.0F;
                sampled = EvaluatePoints(samplePoints, 2, std::clamp(amount, 0.0F, 1.0F));
                break;
            }
        }

        const auto range = maximum - minimum;
        return range > 0.0001F ? std::clamp((sampled - minimum) / range, 0.0F, 1.0F) : 1.0F;
    }

    inline float EvaluateProfile(AnimationProfile a_profile, const AnimationDefault& a_default, float a_phase)
    {
        // Based on MLO2 templates.
        static const AnimationPoint candle[]{
            { 0.00F, { 0.78F, 0.78F, 0.78F } }, { 0.11F, { 0.45F, 0.45F, 0.45F } },
            { 0.24F, { 0.91F, 0.91F, 0.91F } }, { 0.39F, { 0.61F, 0.61F, 0.61F } },
            { 0.53F, { 1.00F, 1.00F, 1.00F } }, { 0.69F, { 0.54F, 0.54F, 0.54F } },
            { 0.83F, { 0.86F, 0.86F, 0.86F } }, { 1.00F, { 0.78F, 0.78F, 0.78F } }
        };
        static const AnimationPoint fire[]{
            { 0.00F, { 0.72F, 0.72F, 0.72F } }, { 0.08F, { 1.00F, 1.00F, 1.00F } },
            { 0.19F, { 0.42F, 0.42F, 0.42F } }, { 0.31F, { 0.88F, 0.88F, 0.88F } },
            { 0.48F, { 0.51F, 0.51F, 0.51F } }, { 0.62F, { 0.96F, 0.96F, 0.96F } },
            { 0.79F, { 0.37F, 0.37F, 0.37F } }, { 1.00F, { 0.72F, 0.72F, 0.72F } }
        };
        static const AnimationPoint lantern[]{
            { 0.00F, { 0.72F, 0.72F, 0.72F } }, { 0.22F, { 0.90F, 0.90F, 0.90F } },
            { 0.47F, { 0.68F, 0.68F, 0.68F } }, { 0.74F, { 0.84F, 0.84F, 0.84F } },
            { 1.00F, { 0.72F, 0.72F, 0.72F } }
        };

        switch (a_profile) {
        case AnimationProfile::kOriginal:
            return EvaluateOriginal(a_default, a_phase);
        case AnimationProfile::kCandle:
            return EvaluatePoints(candle, std::size(candle), a_phase);
        case AnimationProfile::kFire:
            return EvaluatePoints(fire, std::size(fire), a_phase);
        case AnimationProfile::kLantern:
            return EvaluatePoints(lantern, std::size(lantern), a_phase);
        case AnimationProfile::kSlowPulse:
        case AnimationProfile::kFastPulse:
        case AnimationProfile::kMagical:
            return 0.5F - 0.5F * std::cos(a_phase * 2.0F * kPi);
        default:
            return 1.0F;
        }
    }

    inline float ProfileDuration(AnimationProfile a_profile, const AnimationDefault& a_default)
    {
        switch (a_profile) {
        FOREACH_ANIMATION_PROFILE(ANIMATION_PROFILE_DURATION)
        default:
            return 1.0F;
        }
    }

    inline AnimationProfile ActiveProfile(const Entry& a_entry, const Edit& a_edit)
    {
        const auto profile = a_edit.animation.profile;
        return profile == AnimationProfile::kOriginal && !a_edit.defaults.animation.available ? SuggestedProfile(a_entry.category) : profile;
    }

    inline float CurrentPhase(const Entry& a_entry, const Edit& a_edit, AnimationProfile a_profile)
    {
        const auto& animation = a_edit.animation;
        const auto variation = std::clamp(animation.variation, 0.0F, 1.0F);
        const auto speedVariation = 1.0F + (Hash01(a_entry, kSpeedVariationSeed) * 2.0F - 1.0F) * variation * 0.35F;
        const auto nativeFrequency = a_profile == AnimationProfile::kOriginal && a_edit.defaults.animation.available ? a_edit.defaults.animation.frequency : 1.0F;
        const auto phaseOffset = animation.randomPhase ? Hash01(a_entry, kPhaseSeed) : 0.0F;
        return Wrap(elapsedTime * nativeFrequency * std::max(0.01F, animation.speed) * speedVariation / ProfileDuration(a_profile, a_edit.defaults.animation) + phaseOffset);
    }

    inline float CurrentBrightness(const Entry& a_entry, const Edit& a_edit, AnimationProfile a_profile, float a_phase)
    {
        const auto& animation = a_edit.animation;
        const auto variation = std::clamp(animation.variation, 0.0F, 1.0F);
        auto curve = EvaluateProfile(a_profile, a_edit.defaults.animation, a_phase);
        curve = std::clamp(curve + (Hash01(a_entry, kBrightnessVariationSeed) * 2.0F - 1.0F) * variation * 0.08F, 0.0F, 1.0F);
        const auto minimum = std::min(animation.minimumBrightness, animation.maximumBrightness);
        const auto maximum = std::max(animation.minimumBrightness, animation.maximumBrightness);
        return Linear(minimum, maximum, curve);
    }

    inline float CurrentColorAmount(const AnimationEdit& a_animation, float a_phase)
    {
        return a_animation.useSecondaryColor ? 0.5F - 0.5F * std::cos(a_phase * 2.0F * kPi) : 0.0F;
    }

    inline float BaseIntensity(const Edit& a_edit)
    {
        return std::isfinite(a_edit.intensity) ? a_edit.intensity : a_edit.defaults.intensity;
    }

    inline RE::NiColorA BaseMaterialColor(const Edit& a_edit)
    {
        auto color = a_edit.color;
        if (a_edit.defaults.usesVertexColors) {
            color = a_edit.defaults.materialColor;
            color.alpha = a_edit.color.alpha;
        }
        return color;
    }

    inline void SetNativeActive(Entry& a_entry, bool a_active)
    {
        if (a_entry.nativeColorController) {
            a_entry.nativeColorController->flags.set(a_active, RE::NiTimeController::Flag::kActive);
        }
    }

    inline bool RestoreAppearance(Entry& a_entry, const Edit& a_edit)
    {
        auto* material = Editor::GetEditableMaterial(a_entry);
        if (!material) {
            return false;
        }

        material->baseColor = BaseMaterialColor(a_edit);
        material->baseColorScale = a_edit.intensity;
        if (auto* geometry = a_entry.geometry.get()) {
            geometry->SetMaterialNeedsUpdate(true);
        }
        return true;
    }
}
