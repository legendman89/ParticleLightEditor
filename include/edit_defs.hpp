#pragma once

#define FOREACH_EDIT_PROPERTY(PROPERTY) \
    PROPERTY(color, colorChanged, COLOR) \
    PROPERTY(intensity, intensityChanged, VALUE) \
    PROPERTY(radius, radiusChanged, VALUE) \
    PROPERTY(localPosition, positionChanged, POINT) \
    PROPERTY(enabled, enabledChanged, VALUE)

#define FOREACH_CATEGORY_RULE_PROPERTY(PROPERTY) \
    PROPERTY(color, colorChanged, COLOR) \
    PROPERTY(intensity, intensityChanged, VALUE) \
    PROPERTY(radiusScale, radiusChanged, VALUE)
