#include "RenderFP3D.hpp"

#include "constants.hpp"

#ifdef PC
#   include <iostream>
#endif

void rotateOnPlane(
    Fix16& a, Fix16& b,
    Fix16 radians
) {
    // Use the table in your rotateOnPlane function
    auto sin = radians.sin();
    auto cos = radians.cos();
    // Temp values
    auto rot_a = a*cos - b*sin;
    auto rot_b = b*cos + a*sin;
    // Overwrite a, b
    a = rot_a;
    b = rot_b;
}

fix16_vec2 getScreenCoordinate(
    Fix16 FOV, fix16_vec3 point,
    fix16_vec3 translate, fix16_vec2 rotation, fix16_vec3 scale,
    fix16_vec3 camera_pos, fix16_vec2 camera_rot,
    Fix16* z_depth_out
) {
    Fix16 sx, sy;

    point.x *= scale.x;
    point.y *= scale.y;
    point.z *= scale.z;

    // Model rotation
    rotateOnPlane(point.x, point.z, rotation.x);
    rotateOnPlane(point.y, point.z, rotation.y);

    // Model translation + camera position
    fix16_vec3 temp({
        point.x + translate.x - camera_pos.x,
        point.y + translate.y - camera_pos.y,
        point.z + translate.z - camera_pos.z,
    });

    // Player camera rotation
    rotateOnPlane(temp.x, temp.z, camera_rot.x);
    rotateOnPlane(temp.y, temp.z, camera_rot.y);

    // Output Z-Depth
    *z_depth_out = temp.z;

    // Make sure there is no division with zero
    if (temp.z == 0.0f){
        temp.z = 0.001f;
    }
    // fov/z
    auto focal = FOV/temp.z;
    auto realx = ((temp.x)*focal);
    auto realy = ((temp.y)*focal);
    // Shift to screen center (from coordinate center)
    sx = Fix16((int16_t) (SCREEN_X/2)) + (realx);
    sy = Fix16((int16_t) (SCREEN_Y/2)) + (realy);

    // Some extra buffer around actual screen area, where we would still consider
    // pixel to be "visible".
    auto extra = 200.0f;
    if( temp.z < 0.0f
        ||
        sx < (0.0f-extra) || sx > ((float)SCREEN_X+extra)
        ||
        sy < (0.0f-extra) || sy > ((float)SCREEN_Y+extra)
    ){
        sx = fix16_minimum;
        //sy = fix16_minimum;
    }

    return fix16_vec2({sx, sy});
}
