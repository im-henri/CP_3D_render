#pragma once

#include "libfixmath/fix16.hpp"

struct fix16_vec2
{
    Fix16 x;
    Fix16 y;
};

struct fix16_vec3
{
    Fix16 x;
    Fix16 y;
    Fix16 z;
};

void rotateOnPlane(Fix16& a, Fix16& b, Fix16 radians);
fix16_vec2 getScreenCoordinate(
    Fix16 FOV,
    fix16_vec3 point, fix16_vec3 translate, fix16_vec2 rotation, fix16_vec3 scale,
    fix16_vec3 camera_pos, fix16_vec2 camera_rot);
