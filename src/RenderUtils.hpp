#pragma once

#include "libfixmath/fix16.hpp"

// TODO: Only needed for structs fix16_vec3. Move these somewhere else...
#include "RenderFP3D.hpp"

// color_t
#include "Renderer.hpp"

Fix16 calculateLightIntensity(const fix16_vec3& lightPos, const fix16_vec3& surfacePos, const fix16_vec3& normal, Fix16 lightIntensity);

void drawHorizontalLine(
    int x0, int x1, int y,
    int u0, int u1, int v0, int v1,
    uint32_t *texture, int textureWidth, int textureHeight,
    Fix16 lightInstensity = 1.0f
);

void drawTriangle(
    int16_t_Point2d v0, int16_t_Point2d v1, int16_t_Point2d v2,
    uint32_t *texture, int textureWidth, int textureHeight,
    Fix16 lightInstensity = 1.0f
);
void draw_center_square(int16_t cx, int16_t cy, int16_t sx, int16_t sy, color_t color);

void draw_RotationVisualizer(fix16_vec2 camera_rot);