#pragma once

#include "Model.hpp"

#include "DynamicArray.hpp"

#define _NO_TEXTURE_IMPL    (char*)NO_TEXTURE_PATH
#define NO_TEXTURE          _NO_TEXTURE_IMPL

const char NO_TEXTURE_PATH[] = "\0";

class Renderer
{
private:
    DynamicArray<Model*> modelArray;
    fix16_vec3 camera_pos;
    fix16_vec2 camera_rot;
    Fix16 FOV;

    fix16_vec3 lightPos;

public:

    DynamicArray<Model*>& getModelArray();
    // If model has no texture, set as NO_TEXTURE
    Model* addModel(char* model_path, char* texture_path);
    unsigned int getModelCount();

    void update(const uint16_t RENDER_MODE);

    fix16_vec3& get_camera_pos();
    fix16_vec2& get_camera_rot();
    Fix16     & get_FOV();
    fix16_vec3& get_lightPos();

    Renderer();
    ~Renderer();
};
