#pragma once

#define FOREACH_ANIMATION_PROFILE(PROFILE) \
    PROFILE(kOriginal, "Editor.Animation.Pattern.Original", a_default.available ? std::max(0.1F, a_default.duration) : 1.0F) \
    PROFILE(kCandle, "Editor.Animation.Pattern.Candle", 1.25F) \
    PROFILE(kFire, "Editor.Animation.Pattern.Fire", 0.90F) \
    PROFILE(kLantern, "Editor.Animation.Pattern.Lantern", 2.50F) \
    PROFILE(kSlowPulse, "Editor.Animation.Pattern.SlowPulse", 4.00F) \
    PROFILE(kFastPulse, "Editor.Animation.Pattern.FastPulse", 1.50F) \
    PROFILE(kMagical, "Editor.Animation.Pattern.Magical", 5.00F)

#define FOREACH_ANIMATION_COLOR_PROPERTY(PROPERTY) \
    PROPERTY(primaryColor, 1.0F, 1.0F, 1.0F, 1.0F) \
    PROPERTY(secondaryColor, 1.0F, 0.55F, 0.15F, 1.0F)

#define FOREACH_ANIMATION_FLOAT_PROPERTY(PROPERTY) \
    PROPERTY(speed, 1.0F, 0.05F, 10.0F) \
    PROPERTY(minimumBrightness, 0.55F, 0.0F, 5.0F) \
    PROPERTY(maximumBrightness, 0.80F, 0.0F, 5.0F) \
    PROPERTY(variation, 0.20F, 0.0F, 1.0F)

#define FOREACH_ANIMATION_ENUM_PROPERTY(PROPERTY) \
    PROPERTY(AnimationProfile, profile, AnimationProfile::kCandle)

#define FOREACH_ANIMATION_REQUIRED_BOOL_PROPERTY(PROPERTY) \
    PROPERTY(enabled, false)

#define FOREACH_ANIMATION_OPTIONAL_BOOL_PROPERTY(PROPERTY) \
    PROPERTY(useSecondaryColor, false) \
    PROPERTY(randomPhase, true)

#define FOREACH_ANIMATION_BOOL_PROPERTY(PROPERTY) \
    FOREACH_ANIMATION_REQUIRED_BOOL_PROPERTY(PROPERTY) \
    FOREACH_ANIMATION_OPTIONAL_BOOL_PROPERTY(PROPERTY)
