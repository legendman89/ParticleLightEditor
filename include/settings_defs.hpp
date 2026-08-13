#pragma once

#define FOREACH_DRAWING_BOOL_SETTING(SETTING) \
    SETTING(drawLights, true) \
    SETTING(drawOnlySelectedLight, false) \
    SETTING(drawCenterMarkers, true) \
    SETTING(highlightSelectedLight, true) \
    SETTING(showNameValidated, true) \
    SETTING(showRuntimeValidated, true)

#define FOREACH_DETECTION_BOOL_SETTING(SETTING) \
    SETTING(includeGlowNodes, false) \
    SETTING(openEditorAfterConsoleSelection, true)

#define FOREACH_BOOL_SETTING(SETTING) \
    FOREACH_DRAWING_BOOL_SETTING(SETTING) \
    FOREACH_DETECTION_BOOL_SETTING(SETTING)

#define FOREACH_DRAWING_FLOAT_SETTING(SETTING) \
    SETTING(drawRadiusScale, 0.5F) \
    SETTING(lineThickness, 1.5F) \
    SETTING(centerMarkerRadius, 8.0F)

#define FOREACH_DETECTION_FLOAT_SETTING(SETTING) \
    SETTING(drawRange, 1024.0F) \
    SETTING(scanInterval, 0.0F) \
    SETTING(associationRange, 128.0F) \
    SETTING(radiusMatchWeight, 0.25F)

#define FOREACH_FLOAT_SETTING(SETTING) \
    FOREACH_DRAWING_FLOAT_SETTING(SETTING) \
    FOREACH_DETECTION_FLOAT_SETTING(SETTING)

#define FOREACH_DRAWING_INT_SETTING(SETTING) \
    SETTING(circleSegments, 16) \
    SETTING(logLevel, 2)

#define FOREACH_DRAWING_COLOR_SETTING(SETTING) \
    SETTING(centerMarkerColor, 1.0F, 1.0F, 1.0F, 0.9F) \
    SETTING(selectedHighlightColor, 0.0F, 1.0F, 1.0F, 0.95F)

#define FOREACH_DETECTION_INT_SETTING(SETTING)

#define FOREACH_INT_SETTING(SETTING) \
    FOREACH_DRAWING_INT_SETTING(SETTING) \
    FOREACH_DETECTION_INT_SETTING(SETTING)

#define FOREACH_COLOR_SETTING(SETTING) \
    FOREACH_DRAWING_COLOR_SETTING(SETTING)
