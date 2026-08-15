#include "animation.hpp"

#include "animation_detail.hpp"

#include <cstddef>
#include <cstring>

#define ANIMATION_COLOR_FINITE(NAME, RED, GREEN, BLUE, ALPHA) Utility::IsFiniteColor(a_animation.NAME) &&
#define ANIMATION_FLOAT_FINITE(NAME, DEFAULT_VALUE, MINIMUM, MAXIMUM) std::isfinite(a_animation.NAME) &&
#define ANIMATION_ENUM_VALID(TYPE, NAME, DEFAULT_VALUE) a_animation.NAME < TYPE::kTotal &&
#define ANIMATION_FLOAT_IN_RANGE(NAME, DEFAULT_VALUE, MINIMUM, MAXIMUM) a_animation.NAME >= MINIMUM && a_animation.NAME <= MAXIMUM &&
#define ANIMATION_COLOR_EQUAL(NAME, RED, GREEN, BLUE, ALPHA) Utility::ColorsEqual(a_left.NAME, a_right.NAME) &&
#define ANIMATION_FLOAT_EQUAL(NAME, DEFAULT_VALUE, MINIMUM, MAXIMUM) a_left.NAME == a_right.NAME &&
#define ANIMATION_ENUM_EQUAL(TYPE, NAME, DEFAULT_VALUE) a_left.NAME == a_right.NAME &&
#define ANIMATION_BOOL_EQUAL(NAME, DEFAULT_VALUE) a_left.NAME == a_right.NAME &&

namespace ParticleLightEditor::Animation
{
    void Advance(float a_delta)
    {
        if (std::isfinite(a_delta) && a_delta > 0.0F && a_delta < 1.0F) {
            elapsedTime += a_delta;
            if (elapsedTime > 86400.0F) {
                elapsedTime = std::fmod(elapsedTime, 3600.0F);
            }
        }
    }

    // Based on Wired1up's mesh template of MLO2.
    void CaptureDefault(Entry& a_entry)
    {
        auto* geometry = a_entry.geometry.get();
        auto* shader = geometry ? Utility::GetEffectShader(*geometry) : nullptr;
        if (!shader) {
            return;
        }

        for (auto* controller = shader->GetControllers(); controller; controller = controller->GetNext()) {
            const auto* rtti = controller->GetRTTI();
            const auto* name = rtti ? rtti->GetName() : nullptr;
            if (!name || std::string_view(name) != "BSEffectShaderPropertyColorController") {
                continue;
            }

            auto* interpolatorController = netimmerse_cast<RE::NiInterpController*>(controller);
            auto* interpolator = interpolatorController ? interpolatorController->GetInterpolator() : nullptr;
            auto* keyInterpolator = netimmerse_cast<RE::NiKeyBasedInterpolator*>(interpolator);
            if (!keyInterpolator || keyInterpolator->GetKeyChannelCount() == 0 || keyInterpolator->GetKeyContent(0) != RE::NiAnimationKey::KeyContent::kPos) {
                continue;
            }

            const auto count = keyInterpolator->GetKeyCount(0);
            const auto stride = keyInterpolator->GetKeyStride(0);
            const auto* keys = static_cast<const std::byte*>(keyInterpolator->GetKeyArray(0));
            if (!keys || count == 0 || stride < sizeof(float) + sizeof(RE::NiPoint3)) {
                continue;
            }

            auto& animation = a_entry.defaults.animation;
            animation.points.clear();
            animation.points.reserve(count);
            for (uint32_t index = 0; index < count; ++index) {
                AnimationPoint point;
                std::memcpy(&point.time, keys + static_cast<size_t>(index) * stride, sizeof(point.time));
                std::memcpy(&point.color, keys + static_cast<size_t>(index) * stride + sizeof(point.time), sizeof(point.color));
                if (std::isfinite(point.time) && std::isfinite(point.color.x) && std::isfinite(point.color.y) && std::isfinite(point.color.z)) {
                    animation.points.push_back(point);
                }
            }
            if (animation.points.empty()) {
                continue;
            }
            a_entry.nativeColorController = RE::NiPointer<RE::NiTimeController>(controller);
            animation.frequency = std::isfinite(controller->frequency) && controller->frequency > 0.0F ? controller->frequency : 1.0F;
            animation.duration = animation.points.size() > 1 ? std::max(0.1F, animation.points.back().time - animation.points.front().time) : 1.0F;
            animation.controllerActive = controller->flags.all(RE::NiTimeController::Flag::kActive);
            animation.available = true;
            return;
        }
    }

    AnimationProfile SuggestedProfile(ParticleCategory a_category)
    {
        switch (a_category) {
        case ParticleCategory::kCandle:
        case ParticleCategory::kChandelier:
            return AnimationProfile::kCandle;
        case ParticleCategory::kFireEmber:
        case ParticleCategory::kTorchBrazier:
            return AnimationProfile::kFire;
        case ParticleCategory::kLantern:
            return AnimationProfile::kLantern;
        default:
            return AnimationProfile::kSlowPulse;
        }
    }

    AnimationEdit MakeDefault(const ParticleDefault& a_default, ParticleCategory a_category)
    {
        AnimationEdit edit;
        edit.primaryColor = a_default.color;
        edit.secondaryColor = a_default.color;
        edit.secondaryColor.red = std::clamp(edit.secondaryColor.red * 1.15F, 0.0F, 1.0F);
        edit.secondaryColor.green = std::clamp(edit.secondaryColor.green * 0.75F, 0.0F, 1.0F);
        edit.secondaryColor.blue = std::clamp(edit.secondaryColor.blue * 0.45F, 0.0F, 1.0F);
        edit.profile = a_default.animation.available ? AnimationProfile::kOriginal : SuggestedProfile(a_category);
        edit.enabled = a_default.animation.available && a_default.animation.controllerActive;
        edit.variation = a_default.animation.available ? 0.0F : edit.variation;
        edit.randomPhase = !a_default.animation.available;

        if (a_default.animation.available) {
            auto minimum = std::numeric_limits<float>::max();
            auto maximum = std::numeric_limits<float>::lowest();
            for (const auto& point : a_default.animation.points) {
                const auto brightness = Brightness(point.color);
                minimum = std::min(minimum, brightness);
                maximum = std::max(maximum, brightness);
            }
            edit.minimumBrightness = std::clamp(minimum, 0.0F, 2.0F);
            edit.maximumBrightness = std::clamp(maximum, edit.minimumBrightness, 2.0F);
        }
        return edit;
    }

    bool Apply(Entry& a_entry, const Edit& a_edit)
    {
        if (!a_edit.animationChanged) {
            if (a_entry.animationApplied) {
                Restore(a_entry, a_edit);
            }
            return true;
        }

        SetNativeActive(a_entry, false);
        if (!a_edit.animation.enabled) {
            if (!a_entry.animationApplied || a_entry.animationRunning) {
                a_entry.animationApplied = RestoreAppearance(a_entry, a_edit);
            }
            a_entry.animationRunning = false;
            return a_entry.animationApplied;
        }

        auto* material = Editor::GetEditableMaterial(a_entry);
        auto* geometry = a_entry.geometry.get();
        if (!material || !geometry) {
            return false;
        }

        const auto& animation = a_edit.animation;
        const auto profile = ActiveProfile(a_entry, a_edit);
        const auto phase = CurrentPhase(a_entry, a_edit, profile);
        const auto brightness = CurrentBrightness(a_entry, a_edit, profile, phase);
        const auto colorAmount = CurrentColorAmount(animation, phase);

        auto changed = false;
        if (!a_edit.defaults.usesVertexColors) {
            auto color = Linear(animation.primaryColor, animation.secondaryColor, colorAmount);
            color.alpha = a_edit.color.alpha;
            if (!Utility::ColorsEqual(material->baseColor, color)) {
                material->baseColor = color;
                changed = true;
            }
        }
        const auto intensity = std::clamp(BaseIntensity(a_edit) * brightness, 0.0F, 100.0F);
        if (std::abs(material->baseColorScale - intensity) > 0.0001F) {
            material->baseColorScale = intensity;
            changed = true;
        }
        if (changed) {
            geometry->SetMaterialNeedsUpdate(true);
        }
        a_entry.animationApplied = true;
        a_entry.animationRunning = true;
        return true;
    }

    void Restore(Entry& a_entry, const Edit& a_edit)
    {
        RestoreAppearance(a_entry, a_edit);
        SetNativeActive(a_entry, a_edit.defaults.animation.controllerActive);
        a_entry.animationApplied = false;
        a_entry.animationRunning = false;
    }

    void RestoreDefault(Entry& a_entry, const Edit& a_edit)
    {
        auto* material = Editor::GetEditableMaterial(a_entry);
        if (material && a_edit.defaults.hasMaterial) {
            material->baseColor = a_edit.defaults.materialColor;
            material->baseColorScale = a_edit.defaults.intensity;
            if (auto* geometry = a_entry.geometry.get()) {
                geometry->SetMaterialNeedsUpdate(true);
            }
        }
        SetNativeActive(a_entry, a_edit.defaults.animation.controllerActive);
        a_entry.animationApplied = false;
        a_entry.animationRunning = false;
    }

    bool IsFinite(const AnimationEdit& a_animation)
    {
        return FOREACH_ANIMATION_COLOR_PROPERTY(ANIMATION_COLOR_FINITE)
            FOREACH_ANIMATION_FLOAT_PROPERTY(ANIMATION_FLOAT_FINITE)
            FOREACH_ANIMATION_ENUM_PROPERTY(ANIMATION_ENUM_VALID)
            true;
    }

    bool IsValid(const AnimationEdit& a_animation)
    {
        return IsFinite(a_animation) &&
            FOREACH_ANIMATION_FLOAT_PROPERTY(ANIMATION_FLOAT_IN_RANGE)
            a_animation.maximumBrightness >= a_animation.minimumBrightness;
    }

    bool Equal(const AnimationEdit& a_left, const AnimationEdit& a_right)
    {
        return FOREACH_ANIMATION_COLOR_PROPERTY(ANIMATION_COLOR_EQUAL)
            FOREACH_ANIMATION_FLOAT_PROPERTY(ANIMATION_FLOAT_EQUAL)
            FOREACH_ANIMATION_ENUM_PROPERTY(ANIMATION_ENUM_EQUAL)
            FOREACH_ANIMATION_BOOL_PROPERTY(ANIMATION_BOOL_EQUAL)
            true;
    }
}
