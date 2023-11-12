#pragma once

// TODO: Make separate file for fix16 vectors instead. . .
#include "RenderFP3D.hpp"

struct u_pair {
    unsigned First;
    unsigned Second;
};

struct u_triple {
    unsigned First;
    unsigned Second;
    unsigned Third;
};

class Model
{
private:
    bool loaded_from_file;

public:
    Model(
        fix16_vec3* vertices,
        unsigned    vertex_count,
        u_triple*   faces,
        unsigned    faces_count
    );
    Model(char* fname, char* ftexture);
    ~Model();

    fix16_vec3 position;
    fix16_vec2 rotation;
    fix16_vec3 scale;

    fix16_vec3* vertices;
    unsigned    vertex_count;
    u_triple*   faces;
    unsigned    faces_count;

    fix16_vec2* uv_coords;
    unsigned    uv_coord_count;
    u_triple*   uv_faces;
    unsigned    uv_face_count;

    int gen_textureWidth;
    int gen_textureHeight;
    uint32_t * gen_uv_tex; // Malloced array of size: gen_textureWidth * gen_textureHeight

    fix16_vec3& getPosition_ref();
    fix16_vec2& getRotation_ref();
    fix16_vec3& getScale_ref();

    // SUPER slow
    bool load_from_raw_obj_file(char* fname);
    // Way faster (Run obj through python script to generate binary format)
    bool load_from_binary_obj_file(char* fname, char* ftexture);

};
