//------------------------------------------------------------------------------
// light_comp.h - Light component for ECS entities
//------------------------------------------------------------------------------
#ifndef LIGHT_COMP_H
#define LIGHT_COMP_H

#include "component_types.h"

typedef enum {
    LIGHT_DIRECTIONAL,  // Direction derived from transform rotation
    LIGHT_POINT         // Position from transform position
} light_type_t;

typedef struct {
    light_type_t type;
    vec4_t color;       // RGB color (alpha unused)
    float intensity;    // Multiplier (0.0 - 10.0)
} LightComp;

static inline LightComp light_comp_directional(vec4_t color, float intensity) {
    return (LightComp){
        .type = LIGHT_DIRECTIONAL,
        .color = color,
        .intensity = intensity
    };
}

static inline LightComp light_comp_point(vec4_t color, float intensity) {
    return (LightComp){
        .type = LIGHT_POINT,
        .color = color,
        .intensity = intensity
    };
}

#endif // LIGHT_COMP_H
