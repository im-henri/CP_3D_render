#include "Fix16_Utils.hpp"

Fix16 calculateDistance(const fix16_vec3& v1, const fix16_vec3& v2) {
    Fix16 dx = v2.x - v1.x;
    Fix16 dy = v2.y - v1.y;
    Fix16 dz = v2.z - v1.z;
    return fix16_sqrt(dx * dx + dy * dy + dz * dz);
}

fix16_vec3 crossProduct(const fix16_vec3& a, const fix16_vec3& b)
{
    fix16_vec3 result;
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    return result;
}

fix16_vec3 calculateNormal(const fix16_vec3& v0, const fix16_vec3& v1, const fix16_vec3& v2)
{
    fix16_vec3 edge1 = {v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
    fix16_vec3 edge2 = {v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};
    return crossProduct(edge1, edge2);
}

// Normalize a vector
void normalize_fix16_vec3(fix16_vec3& vec)
{
    Fix16 length = fix16_sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
    vec.x /= length;
    vec.y /= length;
    vec.z /= length;
}
