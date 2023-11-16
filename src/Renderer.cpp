#include "Renderer.hpp"

#include "Utils.hpp"

#include "constants.hpp"

#include "Pair.hpp"

#include "Fix16_Utils.hpp"

#include "RenderUtils.hpp"

#ifndef PC
#   include <sdk/os/lcd.hpp>
#   include <sdk/calc/calc.hpp>
#   include <sdk/os/input.hpp>
#else
#   include "PC_SDL_screen.hpp" // replaces "sdk/os/lcd.hpp"
#endif

Renderer::Renderer()
:   camera_pos({-15.0f, -1.6f, -15.0f}),
    camera_rot({0.6f, 0.4f}),
    FOV(300.0f),
    lightPos({0.0f, 0.0f, 0.0f}),
    lastLightScreenLocation({0, 0}),
    camera_move_dirty(true)
{

}

Renderer::~Renderer()
{
    // Free memory of the created models (modelArray itself has destructor which frees its own memory)
    for(unsigned int i=0; i<modelArray.getSize(); i++){
        delete modelArray[i].first;
    }
}

DynamicArray<Pair<Model*, Fix16>>& Renderer::getModelArray()
{
    return modelArray;
}

// If model has no texture, set it as NO_TEXTURE
Model* Renderer::addModel(char* model_path, char* texture_path, bool centerVertices)
{
    // Create new object
    auto m = new Model(model_path, texture_path, centerVertices);
    modelArray.push_back({m, 0.0f});
    // Return pointer back for reference
    return m;
}

unsigned int Renderer::getModelCount()
{
    return modelArray.getSize();
}

fix16_vec3& Renderer::get_camera_pos(){
    return camera_pos;
}
fix16_vec2& Renderer::get_camera_rot(){
    return camera_rot;
}
Fix16& Renderer::get_FOV(){
    return FOV;
}
fix16_vec3& Renderer::get_lightPos(){
    return lightPos;
}

inline void sort_modelRenderOrder(Pair<Model*, Fix16> a[], int n)
{
    // Bubble sort
    for (int j = n; j > 1; --j)
        for (int i = 1; i < j; ++i)
            if (a[i - 1].second < a[i].second)
                swap(a[i - 1], a[i]);
}

void Renderer::draw_LightLocation()
{
    Fix16 z_depth;
    bool is_valid;
    auto screen_vec2 = getScreenCoordinate(
        FOV, lightPos,
        {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f},
        camera_pos, camera_rot,
        &z_depth, &is_valid
    );
    int16_t x = (int16_t)screen_vec2.x;
    int16_t y = (int16_t)screen_vec2.y;
    draw_center_square(x, y, 9,9, color(238,210,2));
    lastLightScreenLocation.x = x;
    lastLightScreenLocation.y = y;
}

void Renderer::clear_LightLocation(color_t clearColor)
{
    int16_t x = lastLightScreenLocation.x;
    int16_t y = lastLightScreenLocation.y;
    draw_center_square(x, y, 9,9, clearColor);
}

void Renderer::update(
    int16_t_vec2* bbox_max,
    int16_t_vec2* bbox_min
) {
    // TODO: Different RENDER_MODEs have a lot in common and could therefore be
    //       combined for cleaner code. BUT cleaner code does not mean faster code here
    //       as we would want to avoid doing bunch of if checks if possible.
    //       -> Too lazy right now to figure this out..

    if (camera_move_dirty)
    {
        camera_move_dirty = false;
        // Sort all models in order from camera. A cheap way to have alteast
        // some kind of order between models. Correct way would be to do this
        // per triangle but calculating square root per triangle takes long time
        // -> Accepting tradeoff of accuracy to gain speed.
        unsigned i = 0;
        for (unsigned m_id=0; m_id<getModelCount(); m_id++){
            Model* m = modelArray[m_id].first;
            Fix16 dist = calculateDistance(m->getPosition_ref(), camera_pos);
            modelArray[m_id].second = dist;
            i++;
        }
        // Sort by camera distance (in reverse order).
        sort_modelRenderOrder(modelArray.getRawArray(), modelArray.getSize());
    }

    bool is_valid;
    for (unsigned m_id=0; m_id<getModelCount(); m_id++)
    {
        auto RENDER_MODE = modelArray[m_id].first->render_mode;

        //
        if (RENDER_MODE == 0){
            Fix16 fix16_sink;
            // Get screen coordinates
            for (unsigned v_id=0; v_id<modelArray[m_id].first->vertex_count; v_id++){
                fix16_vec2 screen_vec2;
                screen_vec2 = getScreenCoordinate(
                    FOV, modelArray[m_id].first->vertices[v_id],
                    modelArray[m_id].first->getPosition_ref(), modelArray[m_id].first->getRotation_ref(),
                    modelArray[m_id].first->getScale_ref(),
                    camera_pos, camera_rot,
                    &fix16_sink, &is_valid
                );
                if(is_valid == false)
                    continue;
                int16_t x = (int16_t)screen_vec2.x;
                int16_t y = (int16_t)screen_vec2.y;
                draw_center_square(x,y,5,5, color(0,0,0));
                // Check bbox
                if (bbox_max->x < x+2) bbox_max->x = x+2;
                if (bbox_max->y < y+2) bbox_max->y = y+2;
                if (bbox_min->x > x-2) bbox_min->x = x-2;
                if (bbox_min->y > y-2) bbox_min->y = y-2;
            }
        } // else if (RENDER_MODE == 0)

        else if (RENDER_MODE == 1)
        {
            // Check first if model has texture
            if (!modelArray[m_id].first->has_texture){
                modelArray[m_id].first->render_mode++;
                continue;
            }

            // Allocate memory
            int16_t_vec2* screen_coords = (int16_t_vec2*) malloc(sizeof(int16_t_vec2) * modelArray[m_id].first->vertex_count);
            Fix16 * vert_z_depths = (Fix16*) malloc(sizeof(Fix16) * modelArray[m_id].first->vertex_count);
            uint_fix16_t * face_draw_order = (uint_fix16_t*) malloc(sizeof(uint_fix16_t) * modelArray[m_id].first->faces_count);

            // Get screen coordinates
            for (unsigned v_id=0; v_id<modelArray[m_id].first->vertex_count; v_id++){
                fix16_vec2 screen_vec2;
                screen_vec2 = getScreenCoordinate(
                    FOV, modelArray[m_id].first->vertices[v_id],
                    modelArray[m_id].first->getPosition_ref(), modelArray[m_id].first->getRotation_ref(),
                    modelArray[m_id].first->getScale_ref(),
                    camera_pos, camera_rot,
                    &vert_z_depths[v_id], &is_valid
                );
                int16_t x = (int16_t)screen_vec2.x;
                int16_t y = (int16_t)screen_vec2.y;
                screen_coords[v_id] = {x, y};
                // Check bbox
                if(is_valid == false)
                    continue;
                if (bbox_max->x < x) bbox_max->x = x;
                if (bbox_max->y < y) bbox_max->y = y;
                if (bbox_min->x > x) bbox_min->x = x;
                if (bbox_min->y > y) bbox_min->y = y;
            }

            // Init the face_draw_order
            for (unsigned f_id=0; f_id<modelArray[m_id].first->faces_count; f_id++)
            {
                unsigned int f_v0_id = modelArray[m_id].first->faces[f_id].First;
                unsigned int f_v1_id = modelArray[m_id].first->faces[f_id].Second;
                unsigned int f_v2_id = modelArray[m_id].first->faces[f_id].Third;
                // Get face z-depth
                Fix16 f_z_depth  = vert_z_depths[f_v0_id]/3.0f;
                f_z_depth       += vert_z_depths[f_v1_id]/3.0f;
                f_z_depth       += vert_z_depths[f_v2_id]/3.0f;

                // Init index = f_id
                face_draw_order[f_id].uint = f_id;
                face_draw_order[f_id].fix16 = f_z_depth;
            }
            // Sorting
            bubble_sort(face_draw_order, modelArray[m_id].first->faces_count);

            // Draw face edges
            for (unsigned int ordered_id=0; ordered_id<modelArray[m_id].first->faces_count; ordered_id++)
            {
                auto f_id = face_draw_order[ordered_id].uint;
                const auto v0 = screen_coords[modelArray[m_id].first->faces[f_id].First];
                const auto v1 = screen_coords[modelArray[m_id].first->faces[f_id].Second];
                const auto v2 = screen_coords[modelArray[m_id].first->faces[f_id].Third];
                const int16_t fix16_cast_int_min = (0xffff & (fix16_minimum>>16)) - 1;
                if( v0.x == fix16_cast_int_min ||
                    v1.x == fix16_cast_int_min ||
                    v2.x == fix16_cast_int_min
                ){
                    continue;
                }
                auto uv0_fix16_norm = modelArray[m_id].first->uv_coords[modelArray[m_id].first->uv_faces[f_id].First];
                auto uv1_fix16_norm = modelArray[m_id].first->uv_coords[modelArray[m_id].first->uv_faces[f_id].Second];
                auto uv2_fix16_norm = modelArray[m_id].first->uv_coords[modelArray[m_id].first->uv_faces[f_id].Third];

                auto v0_u = (int16_t) (uv0_fix16_norm.x * (Fix16((int16_t)modelArray[m_id].first->gen_textureWidth)));
                auto v0_v = (int16_t) (uv0_fix16_norm.y * (Fix16((int16_t)modelArray[m_id].first->gen_textureHeight)));

                auto v1_u = (int16_t) (uv1_fix16_norm.x * (Fix16((int16_t)modelArray[m_id].first->gen_textureWidth)));
                auto v1_v = (int16_t) (uv1_fix16_norm.y * (Fix16((int16_t)modelArray[m_id].first->gen_textureHeight)));

                auto v2_u = (int16_t) (uv2_fix16_norm.x * (Fix16((int16_t)modelArray[m_id].first->gen_textureWidth)));
                auto v2_v = (int16_t) (uv2_fix16_norm.y * (Fix16((int16_t)modelArray[m_id].first->gen_textureHeight)));

                int16_t_Point2d v0_screen = {v0.x,v0.y, v0_u, v0_v};
                int16_t_Point2d v1_screen = {v1.x,v1.y, v1_u, v1_v};
                int16_t_Point2d v2_screen = {v2.x,v2.y, v2_u, v2_v};

                drawTriangle(
                    v0_screen, v1_screen, v2_screen,
                    //gen_uv_tex, gen_textureWidth, gen_textureHeight
                    modelArray[m_id].first->gen_uv_tex,
                    modelArray[m_id].first->gen_textureWidth,
                    modelArray[m_id].first->gen_textureHeight
                );
            }
            free(face_draw_order);
            free(vert_z_depths);
            free(screen_coords);
        }

        else if (RENDER_MODE == 2)
        {
            // Allocate memory
            int16_t_vec2* screen_coords = (int16_t_vec2*) malloc(sizeof(int16_t_vec2) * modelArray[m_id].first->vertex_count);
            Fix16 * vert_z_depths = (Fix16*) malloc(sizeof(Fix16) * modelArray[m_id].first->vertex_count);
            uint_fix16_t * face_draw_order = (uint_fix16_t*) malloc(sizeof(uint_fix16_t) * modelArray[m_id].first->faces_count);
            fix16_vec3* face_normals = (fix16_vec3*) malloc(sizeof(fix16_vec3) * modelArray[m_id].first->faces_count);

            // Get screen coordinates
            for (unsigned v_id=0; v_id<modelArray[m_id].first->vertex_count; v_id++){
                fix16_vec2 screen_vec2;
                screen_vec2 = getScreenCoordinate(
                    FOV, modelArray[m_id].first->vertices[v_id],
                    modelArray[m_id].first->getPosition_ref(), modelArray[m_id].first->getRotation_ref(),
                    modelArray[m_id].first->getScale_ref(),
                    camera_pos, camera_rot,
                    &vert_z_depths[v_id], &is_valid
                );
                int16_t x = (int16_t)screen_vec2.x;
                int16_t y = (int16_t)screen_vec2.y;
                screen_coords[v_id] = {x, y};
                // Check bbox
                if(is_valid == false)
                    continue;
                if (bbox_max->x < x) bbox_max->x = x;
                if (bbox_max->y < y) bbox_max->y = y;
                if (bbox_min->x > x) bbox_min->x = x;
                if (bbox_min->y > y) bbox_min->y = y;
            }
            // Init the face_draw_order
            for (unsigned f_id=0; f_id<modelArray[m_id].first->faces_count; f_id++)
            {
                unsigned int f_v0_id = modelArray[m_id].first->faces[f_id].First;
                unsigned int f_v1_id = modelArray[m_id].first->faces[f_id].Second;
                unsigned int f_v2_id = modelArray[m_id].first->faces[f_id].Third;
                // Get face z-depth
                Fix16 f_z_depth  = vert_z_depths[f_v0_id]/3.0f;
                f_z_depth       += vert_z_depths[f_v1_id]/3.0f;
                f_z_depth       += vert_z_depths[f_v2_id]/3.0f;

                // Init index = f_id
                face_draw_order[f_id].uint = f_id;
                face_draw_order[f_id].fix16 = f_z_depth;

                // ----- Calculate also face normals here

                // Face vertices
                fix16_vec3 v0 = modelArray[m_id].first->vertices[f_v0_id];
                fix16_vec3 v1 = modelArray[m_id].first->vertices[f_v1_id];
                fix16_vec3 v2 = modelArray[m_id].first->vertices[f_v2_id];
                // Model rotation
                rotateOnPlane(v0.x, v0.z, modelArray[m_id].first->rotation.x);
                rotateOnPlane(v0.y, v0.z, modelArray[m_id].first->rotation.y);
                rotateOnPlane(v1.x, v1.z, modelArray[m_id].first->rotation.x);
                rotateOnPlane(v1.y, v1.z, modelArray[m_id].first->rotation.y);
                rotateOnPlane(v2.x, v2.z, modelArray[m_id].first->rotation.x);
                rotateOnPlane(v2.y, v2.z, modelArray[m_id].first->rotation.y);
                // Model translation
                v0.x += modelArray[m_id].first->position.x;
                v0.y += modelArray[m_id].first->position.y;
                v0.z += modelArray[m_id].first->position.z;
                v1.x += modelArray[m_id].first->position.x;
                v1.y += modelArray[m_id].first->position.y;
                v1.z += modelArray[m_id].first->position.z;
                v2.x += modelArray[m_id].first->position.x;
                v2.y += modelArray[m_id].first->position.y;
                v2.z += modelArray[m_id].first->position.z;
                // Calculate face normal
                auto face_norm = calculateNormal(v0, v1, v2);
                normalize_fix16_vec3(face_norm);
                //
                face_normals[f_id] = face_norm;
            }
            // Sorting
            bubble_sort(face_draw_order, modelArray[m_id].first->faces_count);

            // Optimization: Create temporary light position that has negative model position in it.
            //               Reduces addition from once per face to once per model.
            fix16_vec3 shifted_lightPos = {lightPos.x, lightPos.y, lightPos.z};
            shifted_lightPos.x -= modelArray[m_id].first->getPosition_ref().x;
            shifted_lightPos.y -= modelArray[m_id].first->getPosition_ref().y;
            shifted_lightPos.z -= modelArray[m_id].first->getPosition_ref().z;

            // Draw face edges
            for (unsigned int ordered_id=0; ordered_id<modelArray[m_id].first->faces_count; ordered_id++)
            {
                auto f_id = face_draw_order[ordered_id].uint;
                const auto v0 = screen_coords[modelArray[m_id].first->faces[f_id].First];
                const auto v1 = screen_coords[modelArray[m_id].first->faces[f_id].Second];
                const auto v2 = screen_coords[modelArray[m_id].first->faces[f_id].Third];
                const int16_t fix16_cast_int_min = (0xffff & (fix16_minimum>>16)) - 1;
                if( v0.x == fix16_cast_int_min ||
                    v1.x == fix16_cast_int_min ||
                    v2.x == fix16_cast_int_min
                ){
                    continue;
                }

                auto face_pos = modelArray[m_id].first->vertices[modelArray[m_id].first->faces[f_id].First];

                Fix16 lightIntensity = calculateLightIntensity(
                        shifted_lightPos, face_pos, face_normals[f_id], Fix16(1.0f)
                );
                triangle(
                    v0.x,v0.y,v1.x,v1.y,v2.x,v2.y,
                    color((int16_t)(lightIntensity*255.0f),(int16_t)(lightIntensity*255.0f),(int16_t)(lightIntensity*255.0f)),
                    color(0,0,0)
                );
            }
            free(face_draw_order);
            free(vert_z_depths);
            free(screen_coords);
            free(face_normals);

            // Draw sun visualizer
            draw_LightLocation();
        }

        else if (RENDER_MODE == 3)
        {
            // Allocate memory
            int16_t_vec2* screen_coords = (int16_t_vec2*) malloc(sizeof(int16_t_vec2) * modelArray[m_id].first->vertex_count);
            Fix16 * vert_z_depths = (Fix16*) malloc(sizeof(Fix16) * modelArray[m_id].first->vertex_count);
            uint_fix16_t * face_draw_order = (uint_fix16_t*) malloc(sizeof(uint_fix16_t) * modelArray[m_id].first->faces_count);

            // Get screen coordinates
            for (unsigned v_id=0; v_id<modelArray[m_id].first->vertex_count; v_id++){
                fix16_vec2 screen_vec2;
                screen_vec2 = getScreenCoordinate(
                    FOV, modelArray[m_id].first->vertices[v_id],
                    modelArray[m_id].first->getPosition_ref(), modelArray[m_id].first->getRotation_ref(),
                    modelArray[m_id].first->getScale_ref(),
                    camera_pos, camera_rot,
                    &vert_z_depths[v_id], &is_valid
                );
                int16_t x = (int16_t)screen_vec2.x;
                int16_t y = (int16_t)screen_vec2.y;
                screen_coords[v_id] = {x, y};
                // Check bbox
                if(is_valid == false)
                    continue;
                if (bbox_max->x < x) bbox_max->x = x;
                if (bbox_max->y < y) bbox_max->y = y;
                if (bbox_min->x > x) bbox_min->x = x;
                if (bbox_min->y > y) bbox_min->y = y;
            }

            // Init the face_draw_order
            for (unsigned f_id=0; f_id<modelArray[m_id].first->faces_count; f_id++)
            {
                unsigned int f_v0_id = modelArray[m_id].first->faces[f_id].First;
                unsigned int f_v1_id = modelArray[m_id].first->faces[f_id].Second;
                unsigned int f_v2_id = modelArray[m_id].first->faces[f_id].Third;
                // Get face z-depth
                Fix16 f_z_depth  = vert_z_depths[f_v0_id]/3.0f;
                f_z_depth       += vert_z_depths[f_v1_id]/3.0f;
                f_z_depth       += vert_z_depths[f_v2_id]/3.0f;

                // Init index = f_id
                face_draw_order[f_id].uint = f_id;
                face_draw_order[f_id].fix16 = f_z_depth;
            }
            // Sorting
            bubble_sort(face_draw_order, modelArray[m_id].first->faces_count);

            // Draw face edges
            for (unsigned int ordered_id=0; ordered_id<modelArray[m_id].first->faces_count; ordered_id++)
            {
                auto f_id = face_draw_order[ordered_id].uint;
                const auto v0 = screen_coords[modelArray[m_id].first->faces[f_id].First];
                const auto v1 = screen_coords[modelArray[m_id].first->faces[f_id].Second];
                const auto v2 = screen_coords[modelArray[m_id].first->faces[f_id].Third];
                const int16_t fix16_cast_int_min = (0xffff & (fix16_minimum>>16)) - 1;
                if( v0.x == fix16_cast_int_min ||
                    v1.x == fix16_cast_int_min ||
                    v2.x == fix16_cast_int_min
                ){
                    continue;
                }
                uint32_t colorr =
                    0xff << (ordered_id*(24)/modelArray[m_id].first->faces_count);
                triangle(
                    v0.x,v0.y,v1.x,v1.y,v2.x,v2.y,
                    color((colorr>>16)&0xcf, (colorr>>8)&0xcf, (colorr>>0)&0xcf),
                    color(0,0,0)
                );
            }
            free(face_draw_order);
            free(vert_z_depths);
            free(screen_coords);

        }  // else if (RENDER_MODE == 3)

        else if (RENDER_MODE == 4)
        {
            // Allocate memory
            int16_t_vec2* screen_coords = (int16_t_vec2*) malloc(sizeof(int16_t_vec2) * modelArray[m_id].first->vertex_count);

            Fix16 fix16_sink;
            // Get screen coordinates
            for (unsigned v_id=0; v_id<modelArray[m_id].first->vertex_count; v_id++){
                fix16_vec2 screen_vec2;
                screen_vec2 = getScreenCoordinate(
                    FOV, modelArray[m_id].first->vertices[v_id],
                    modelArray[m_id].first->getPosition_ref(), modelArray[m_id].first->getRotation_ref(),
                    modelArray[m_id].first->getScale_ref(),
                    camera_pos, camera_rot,
                    &fix16_sink, &is_valid
                );
                int16_t x = (int16_t)screen_vec2.x;
                int16_t y = (int16_t)screen_vec2.y;
                screen_coords[v_id] = {x, y};
                // Check bbox
                if(is_valid == false)
                    continue;
                if (bbox_max->x < x) bbox_max->x = x;
                if (bbox_max->y < y) bbox_max->y = y;
                if (bbox_min->x > x) bbox_min->x = x;
                if (bbox_min->y > y) bbox_min->y = y;
            }

            for (unsigned int f_id=0; f_id<modelArray[m_id].first->faces_count; f_id++)
            {
                const auto v0 = screen_coords[modelArray[m_id].first->faces[f_id].First];
                const auto v1 = screen_coords[modelArray[m_id].first->faces[f_id].Second];
                const auto v2 = screen_coords[modelArray[m_id].first->faces[f_id].Third];
                const int16_t fix16_cast_int_min = (0xffff & (fix16_minimum>>16)) - 1;
                if( v0.x == fix16_cast_int_min ||
                    v1.x == fix16_cast_int_min ||
                    v2.x == fix16_cast_int_min
                ){
                    continue;
                }
                triangle(
                    v0.x,v0.y, v1.x,v1.y, v2.x,v2.y,
                    color( 255,(f_id*8)%255,(f_id*16)%255 ),
                    color(0,0,0)
                );
            }
            free(screen_coords);

        }  // else if (RENDER_MODE == 4)

        else if (RENDER_MODE == 5)
        {
            // Allocate memory
            int16_t_vec2* screen_coords = (int16_t_vec2*) malloc(sizeof(int16_t_vec2) * modelArray[m_id].first->vertex_count);

            Fix16 fix16_sink;
            // Get screen coordinates
            for (unsigned v_id=0; v_id<modelArray[m_id].first->vertex_count; v_id++){
                fix16_vec2 screen_vec2;
                screen_vec2 = getScreenCoordinate(
                    FOV, modelArray[m_id].first->vertices[v_id],
                    modelArray[m_id].first->getPosition_ref(), modelArray[m_id].first->getRotation_ref(),
                    modelArray[m_id].first->getScale_ref(),
                    camera_pos, camera_rot,
                    &fix16_sink, &is_valid
                );
                int16_t x = (int16_t)screen_vec2.x;
                int16_t y = (int16_t)screen_vec2.y;
                screen_coords[v_id] = {x, y};
                // Check bbox
                if(is_valid == false)
                    continue;
                if (bbox_max->x < x) bbox_max->x = x;
                if (bbox_max->y < y) bbox_max->y = y;
                if (bbox_min->x > x) bbox_min->x = x;
                if (bbox_min->y > y) bbox_min->y = y;
            }

            for (unsigned int f_id=0; f_id<modelArray[m_id].first->faces_count; f_id++)
            {
                const auto v0 = screen_coords[modelArray[m_id].first->faces[f_id].First];
                const auto v1 = screen_coords[modelArray[m_id].first->faces[f_id].Second];
                const auto v2 = screen_coords[modelArray[m_id].first->faces[f_id].Third];
                const int16_t fix16_cast_int_min = (0xffff & (fix16_minimum>>16)) - 1;
                if( v0.x == fix16_cast_int_min ||
                    v1.x == fix16_cast_int_min ||
                    v2.x == fix16_cast_int_min
                ){
                    continue;
                }
                line(v0.x,v0.y, v1.x, v1.y, color(0,0,0));
                line(v1.x,v1.y, v2.x, v2.y, color(0,0,0));
                line(v2.x,v2.y, v0.x, v0.y, color(0,0,0));
            }
            free(screen_coords);

        } // else if (RENDER_MODE == 5)

        if (RENDER_MODE == 6)
        {
            // Check first if model has texture
            if (!modelArray[m_id].first->has_texture){
                modelArray[m_id].first->render_mode = 0;
                continue;
            }

            // Allocate memory
            int16_t_vec2* screen_coords = (int16_t_vec2*) malloc(sizeof(int16_t_vec2) * modelArray[m_id].first->vertex_count);
            Fix16 * vert_z_depths = (Fix16*) malloc(sizeof(Fix16) * modelArray[m_id].first->vertex_count);
            uint_fix16_t * face_draw_order = (uint_fix16_t*) malloc(sizeof(uint_fix16_t) * modelArray[m_id].first->faces_count);
            fix16_vec3* face_normals = (fix16_vec3*) malloc(sizeof(fix16_vec3) * modelArray[m_id].first->faces_count);

            // Get screen coordinates
            for (unsigned v_id=0; v_id<modelArray[m_id].first->vertex_count; v_id++){
                fix16_vec2 screen_vec2;
                screen_vec2 = getScreenCoordinate(
                    FOV, modelArray[m_id].first->vertices[v_id],
                    modelArray[m_id].first->getPosition_ref(), modelArray[m_id].first->getRotation_ref(),
                    modelArray[m_id].first->getScale_ref(),
                    camera_pos, camera_rot,
                    &vert_z_depths[v_id], &is_valid
                );
                int16_t x = (int16_t)screen_vec2.x;
                int16_t y = (int16_t)screen_vec2.y;
                screen_coords[v_id] = {x, y};
                // Check bbox
                if(is_valid == false)
                    continue;
                if (bbox_max->x < x) bbox_max->x = x;
                if (bbox_max->y < y) bbox_max->y = y;
                if (bbox_min->x > x) bbox_min->x = x;
                if (bbox_min->y > y) bbox_min->y = y;
            }

            // Init the face_draw_order
            for (unsigned f_id=0; f_id<modelArray[m_id].first->faces_count; f_id++)
            {
                unsigned int f_v0_id = modelArray[m_id].first->faces[f_id].First;
                unsigned int f_v1_id = modelArray[m_id].first->faces[f_id].Second;
                unsigned int f_v2_id = modelArray[m_id].first->faces[f_id].Third;

                // Get face z-depth
                Fix16 f_z_depth  = vert_z_depths[f_v0_id]/3.0f;
                f_z_depth       += vert_z_depths[f_v1_id]/3.0f;
                f_z_depth       += vert_z_depths[f_v2_id]/3.0f;

                // Init index = f_id
                face_draw_order[f_id].uint = f_id;
                face_draw_order[f_id].fix16 = f_z_depth;
                // ----- Calculate also face normals here

                // Face vertices
                fix16_vec3 v0 = modelArray[m_id].first->vertices[f_v0_id];
                fix16_vec3 v1 = modelArray[m_id].first->vertices[f_v1_id];
                fix16_vec3 v2 = modelArray[m_id].first->vertices[f_v2_id];
                // Model rotation
                rotateOnPlane(v0.x, v0.z, modelArray[m_id].first->rotation.x);
                rotateOnPlane(v0.y, v0.z, modelArray[m_id].first->rotation.y);
                rotateOnPlane(v1.x, v1.z, modelArray[m_id].first->rotation.x);
                rotateOnPlane(v1.y, v1.z, modelArray[m_id].first->rotation.y);
                rotateOnPlane(v2.x, v2.z, modelArray[m_id].first->rotation.x);
                rotateOnPlane(v2.y, v2.z, modelArray[m_id].first->rotation.y);
                // Model translation
                v0.x += modelArray[m_id].first->position.x;
                v0.y += modelArray[m_id].first->position.y;
                v0.z += modelArray[m_id].first->position.z;
                v1.x += modelArray[m_id].first->position.x;
                v1.y += modelArray[m_id].first->position.y;
                v1.z += modelArray[m_id].first->position.z;
                v2.x += modelArray[m_id].first->position.x;
                v2.y += modelArray[m_id].first->position.y;
                v2.z += modelArray[m_id].first->position.z;
                // Calculate face normal
                auto face_norm = calculateNormal(v0, v1, v2);
                normalize_fix16_vec3(face_norm);
                //
                face_normals[f_id] = face_norm;
            }

            // Sorting
            bubble_sort(face_draw_order, modelArray[m_id].first->faces_count);

            // Optimization: Create temporary light position that has negative model position in it.
            //               Reduces addition from once per face to once per model.
            fix16_vec3 shifted_lightPos = {lightPos.x, lightPos.y, lightPos.z};
            shifted_lightPos.x -= modelArray[m_id].first->getPosition_ref().x;
            shifted_lightPos.y -= modelArray[m_id].first->getPosition_ref().y;
            shifted_lightPos.z -= modelArray[m_id].first->getPosition_ref().z;

            // Draw face edges
            for (unsigned int ordered_id=0; ordered_id<modelArray[m_id].first->faces_count; ordered_id++)
            {
                auto f_id = face_draw_order[ordered_id].uint;
                const auto v0 = screen_coords[modelArray[m_id].first->faces[f_id].First];
                const auto v1 = screen_coords[modelArray[m_id].first->faces[f_id].Second];
                const auto v2 = screen_coords[modelArray[m_id].first->faces[f_id].Third];
                const int16_t fix16_cast_int_min = (0xffff & (fix16_minimum>>16)) - 1;
                if( v0.x == fix16_cast_int_min ||
                    v1.x == fix16_cast_int_min ||
                    v2.x == fix16_cast_int_min
                ){
                    continue;
                }
                auto uv0_fix16_norm = modelArray[m_id].first->uv_coords[modelArray[m_id].first->uv_faces[f_id].First];
                auto uv1_fix16_norm = modelArray[m_id].first->uv_coords[modelArray[m_id].first->uv_faces[f_id].Second];
                auto uv2_fix16_norm = modelArray[m_id].first->uv_coords[modelArray[m_id].first->uv_faces[f_id].Third];

                auto v0_u = (int16_t) (uv0_fix16_norm.x * (Fix16((int16_t)modelArray[m_id].first->gen_textureWidth)));
                auto v0_v = (int16_t) (uv0_fix16_norm.y * (Fix16((int16_t)modelArray[m_id].first->gen_textureHeight)));

                auto v1_u = (int16_t) (uv1_fix16_norm.x * (Fix16((int16_t)modelArray[m_id].first->gen_textureWidth)));
                auto v1_v = (int16_t) (uv1_fix16_norm.y * (Fix16((int16_t)modelArray[m_id].first->gen_textureHeight)));

                auto v2_u = (int16_t) (uv2_fix16_norm.x * (Fix16((int16_t)modelArray[m_id].first->gen_textureWidth)));
                auto v2_v = (int16_t) (uv2_fix16_norm.y * (Fix16((int16_t)modelArray[m_id].first->gen_textureHeight)));

                int16_t_Point2d v0_screen = {v0.x,v0.y, v0_u, v0_v};
                int16_t_Point2d v1_screen = {v1.x,v1.y, v1_u, v1_v};
                int16_t_Point2d v2_screen = {v2.x,v2.y, v2_u, v2_v};

                const auto face_pos = modelArray[m_id].first->vertices[modelArray[m_id].first->faces[f_id].First];
                Fix16 lightIntensity = calculateLightIntensity(
                        shifted_lightPos, face_pos, face_normals[f_id], Fix16(1.0f)
                );

                drawTriangle(
                    v0_screen, v1_screen, v2_screen,
                    modelArray[m_id].first->gen_uv_tex,
                    modelArray[m_id].first->gen_textureWidth,
                    modelArray[m_id].first->gen_textureHeight,
                    lightIntensity
                );
            }
            free(face_draw_order);
            free(vert_z_depths);
            free(screen_coords);
            free(face_normals);

            // Draw sun visualizer
            draw_LightLocation();
        }  // if (RENDER_MODE == 6)
    }

    // Some buffer around bbox (as draw lines may draw over the bbox)
    bbox_min->x -= 2;
    bbox_min->y -= 2;
    bbox_max->x += 2;
    bbox_max->y += 2;

    // Check bbox - clamp
    if(bbox_min->x < 0)        bbox_min->x = 0;
    if(bbox_min->y < 0)        bbox_min->y = 0;
    if(bbox_max->x > SCREEN_X) bbox_max->x = SCREEN_X;
    if(bbox_max->y > SCREEN_Y) bbox_max->y = SCREEN_Y;

    // Draw rotation visualizer in corner
    draw_RotationVisualizer(camera_rot);
}