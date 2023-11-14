#pragma once

#include "libfixmath/fix16.hpp"

struct int16_t_vec2
{
    int16_t x;
    int16_t y;
};

struct int16_t_Point2d
{
    int16_t x;
    int16_t y;
    int16_t u;
    int16_t v;
};

struct color8_vec
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

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

struct uint_fix16_t
{
    unsigned int uint;
    Fix16        fix16;
};

void rotateOnPlane(Fix16& a, Fix16& b, Fix16 radians);

fix16_vec2 getScreenCoordinate(
    Fix16 FOV, fix16_vec3 point,
    fix16_vec3 translate, fix16_vec2 rotation, fix16_vec3 scale,
    fix16_vec3 camera_pos, fix16_vec2 camera_rot,
    Fix16* z_depth
);
