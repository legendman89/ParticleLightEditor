#pragma once

#define FOREACH_EDIT_PROPERTY(PROPERTY) \
    PROPERTY(kColor, color, colorChanged, COLOR, "Color and opacity") \
    PROPERTY(kIntensity, intensity, intensityChanged, VALUE, "Intensity") \
    PROPERTY(kRadius, radius, radiusChanged, VALUE, "Particle radius") \
    PROPERTY(kPosition, localPosition, positionChanged, POINT, "Local position") \
    PROPERTY(kEnabled, enabled, enabledChanged, VALUE, "Enabled state")

#define FOREACH_CATEGORY_RULE_PROPERTY(PROPERTY) \
    PROPERTY(kColor, color, colorChanged, COLOR, "Color and opacity") \
    PROPERTY(kIntensity, intensity, intensityChanged, VALUE, "Intensity") \
    PROPERTY(kRadius, radiusScale, radiusChanged, VALUE, "Particle radius")
