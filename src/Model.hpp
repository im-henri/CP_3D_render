#pragma once

// TODO: Make separate file for fix16 vectors instead. . .
#include "RenderFP3D.hpp"

struct u_pair {
    unsigned First;
    unsigned Second;
};

class Model
{
private:
    bool loaded_from_file;

public:
    Model(
        fix16_vec3* vertices,
        unsigned vertex_count,
        u_pair* edges,
        unsigned edge_count
    );
    Model(char* fname);
    ~Model();

    fix16_vec3 position;
    fix16_vec2 rotation;
    fix16_vec3 scale;

    fix16_vec3* vertices;
    unsigned    vertex_count;
    u_pair*     edges;
    unsigned    edge_count;

    fix16_vec3& getPosition_ref();
    fix16_vec2& getRotation_ref();
    fix16_vec3& getScale_ref();

    bool load_from_file(char* fname);


};
