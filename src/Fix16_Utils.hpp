#pragma once

#include "libfixmath/fix16.hpp"

// TODO: Only needed for structs fix16_vec3. Move these somewhere else...
#include "RenderFP3D.hpp"

Fix16 calculateDistance(const fix16_vec3& v1, const fix16_vec3& v2);
fix16_vec3 crossProduct(const fix16_vec3& a, const fix16_vec3& b);
fix16_vec3 calculateNormal(const fix16_vec3& v0, const fix16_vec3& v1, const fix16_vec3& v2);
void normalize_fix16_vec3(fix16_vec3& vec);