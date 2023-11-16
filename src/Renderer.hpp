#pragma once

#include "Model.hpp"

#include "DynamicArray.hpp"

#include "Pair.hpp"

#define _NO_TEXTURE_IMPL    (char*)NO_TEXTURE_PATH
#define NO_TEXTURE          _NO_TEXTURE_IMPL

const float   ROTATION_VISUALIZER_LINE_WIDTH = 20.0f;
const int16_t ROTATION_VISALIZER_EDGE_OFFSET = 15;

const uint16_t RENDER_MODE_COUNT = 7;

const char NO_TEXTURE_PATH[] = "\0";

#ifdef PC
    typedef uint32_t color_t; // SDL2 uses 32b colors (24b colors + 8b alpha). Alpha not used.
#else
    typedef uint16_t color_t; // ClassPad uses 16b colors
#endif


class Renderer
{
private:
    DynamicArray<Pair<Model*, Fix16>> modelArray;

    fix16_vec3 camera_pos;
    fix16_vec2 camera_rot;
    Fix16 FOV;

    fix16_vec3 lightPos;

    int16_t_vec2 lastLightScreenLocation;

public:

    bool camera_move_dirty;

    DynamicArray<Pair<Model*, Fix16>>& getModelArray();
    // If model has no texture, set as NO_TEXTURE
    Model* addModel(char* model_path, char* texture_path, bool centerVertices=true);
    unsigned int getModelCount();

    void update(int16_t_vec2* bbox_max, int16_t_vec2* bbox_min);

    fix16_vec3& get_camera_pos();
    fix16_vec2& get_camera_rot();
    Fix16     & get_FOV();
    fix16_vec3& get_lightPos();

    // Draws box as light location
    void draw_LightLocation();
    void clear_LightLocation(color_t clearColor);

    Renderer();
    ~Renderer();
};
