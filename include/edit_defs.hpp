#pragma once

#define FOREACH_EDIT_PROPERTY(PROPERTY) \
    PROPERTY(kColor, color, colorChanged, COLOR, "Editor.Color") \
    PROPERTY(kIntensity, intensity, intensityChanged, VALUE, "Editor.Intensity") \
    PROPERTY(kRadius, radius, radiusChanged, VALUE, "Editor.Radius") \
    PROPERTY(kPosition, localPosition, positionChanged, POINT, "Editor.Position") \
    PROPERTY(kEnabled, enabled, enabledChanged, VALUE, "Editor.EnabledState")

#define FOREACH_CATEGORY_RULE_PROPERTY(PROPERTY) \
    PROPERTY(kColor, color, colorChanged, COLOR, "Editor.Color") \
    PROPERTY(kIntensity, intensity, intensityChanged, VALUE, "Editor.Intensity") \
    PROPERTY(kRadius, radiusScale, radiusChanged, VALUE, "Editor.Radius")
