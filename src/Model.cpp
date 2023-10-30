#include "Model.hpp"

Model::Model(
    fix16_vec3* vertices,
    unsigned vertex_count,
    u_pair* edges,
    unsigned edge_count
) : position({0.0f, 0.0f, 0.0f}), rotation({0.0f, 0.0f}), scale({1.0f,1.0f,1.0f}),
    vertices(vertices), vertex_count(vertex_count), edges(edges), edge_count(edge_count)
{ }

Model::~Model() { }

fix16_vec3& Model::getPosition_ref()
{
    return this->position;
}

fix16_vec2& Model::getRotation_ref()
{
    return this->rotation;
}

fix16_vec3& Model::getScale_ref()
{
    return this->scale;
}