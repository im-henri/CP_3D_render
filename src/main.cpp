
#include "libfixmath/fix16.hpp"

#include "RenderFP3D.hpp"

#include "constants.hpp"

#include "Model.hpp"

#include "StringUtils.hpp"

#include "Utils.hpp"

#ifndef PC
#   include "app_description.hpp"
#   include <sdk/calc/calc.hpp>
#   include <sdk/os/lcd.hpp>
#   include <sdk/os/debug.hpp>
#   include <sdk/os/mem.hpp>
#   include <sdk/os/file.hpp>
#   include "fps_functions.hpp"
#else
    // SDL2 as our graphics library
#   include <SDL2/SDL.h>
    // This is not a standard "header"!
    // These functions are pretty much 1-to-1 copied from hollyhock2
    // sdk but instead of drawing to calculator screen (vram)
    // it draws to SDL2 screen (texture).
#   include "PC_SDL_screen.hpp" // replaces "sdk/os/lcd.hpp"
#   include <iostream>  // std::string
#   include <unistd.h>  // File open & close
#   include <fcntl.h>   // File open & close
#endif

#ifndef PC
#   define KEY_MOVE_LEFT       testKey(k1,k2,KEY_4)
#   define KEY_MOVE_RIGHT      testKey(k1,k2,KEY_6)
#   define KEY_MOVE_FORWARD    testKey(k1,k2,KEY_8)
#   define KEY_MOVE_BACKWARD   testKey(k1,k2,KEY_2)
#   define KEY_MOVE_UP         testKey(k1,k2,KEY_9)
#   define KEY_MOVE_DOWN       testKey(k1,k2,KEY_3)
#   define KEY_MOVE_FOV_ADD    testKey(k1,k2,KEY_ADD)
#   define KEY_MOVE_FOV_SUB    testKey(k1,k2,KEY_SUBTRACT)
#   define KEY_MOVE_REND_MODE  testKey(k1,k2,KEY_0)
#   define KEY_ROTATE_LEFT     testKey(k1,k2,KEY_LEFT)
#   define KEY_ROTATE_RIGHT    testKey(k1,k2,KEY_RIGHT)
#   define KEY_ROTATE_UP       testKey(k1,k2,KEY_UP)
#   define KEY_ROTATE_DOWN     testKey(k1,k2,KEY_DOWN)
#else
#   define KEY_MOVE_LEFT       key_left
#   define KEY_MOVE_RIGHT      key_right
#   define KEY_MOVE_FORWARD    key_up
#   define KEY_MOVE_BACKWARD   key_down
#   define KEY_MOVE_UP         key_r
#   define KEY_MOVE_DOWN       key_f
#   define KEY_MOVE_FOV_ADD    key_1
#   define KEY_MOVE_FOV_SUB    key_2
#   define KEY_MOVE_REND_MODE  key_e
#   define KEY_ROTATE_LEFT     key_a
#   define KEY_ROTATE_RIGHT    key_d
#   define KEY_ROTATE_UP       key_w
#   define KEY_ROTATE_DOWN     key_s
#endif


#ifdef PC
    typedef uint32_t color_t; // SDL2 uses 32b colors (24b colors + 8b alpha). Alpha not used.
#else
    typedef uint16_t color_t; // ClassPad uses 16b colors
#endif

#ifndef PC
// fps10 from "fps_functions.hpp" in calculator case
extern int fps10;
#endif

#define MOVEMENT_SPEED   20.0f
#define CAMERA_SPEED      1.5f
#define FOV_UPDATE_SPEED 50.0f

#define FILL_SCREEN_COLOR color(190,190,190)

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

Fix16 calculateLightIntensity(const fix16_vec3& lightPos, const fix16_vec3& surfacePos, const fix16_vec3& normal, Fix16 lightIntensity)
{
    fix16_vec3 lightDir = {lightPos.x - surfacePos.x, lightPos.y - surfacePos.y, lightPos.z - surfacePos.z};
    normalize_fix16_vec3(lightDir);

    Fix16 intensity = lightIntensity * fix16_max(0, lightDir.x * normal.x + lightDir.y * normal.y + lightDir.z * normal.z);

    return intensity;
}

void draw_center_square(int16_t cx, int16_t cy, int16_t sx, int16_t sy, color_t color)
{
    for(int16_t i=-sx/2; i<sx/2; i++)
    {
        for(int16_t j=-sy/2; j<sy/2; j++)
        {
            setPixel(cx+i, cy+j, color);
        }
    }
}

void drawHorizontalLine(
    int x0, int x1, int y,
    int u0, int u1, int v0, int v1,
    uint32_t *texture, int textureWidth, int textureHeight,
    Fix16 lightInstensity = 1.0f
) {
    if (x0 > x1) {
        swap(x0, x1);
        swap(u0, u1);
        swap(v0, v1);
    }

    if (x0 == x1) {
        int u = u0;
        int v = v0;

        if (u >= 0 && u < textureWidth && v >= 0 && v < textureHeight) {
            auto texel = texture[u + v * textureWidth];
            uint8_t r = (0xff & (texel>>16));
            uint8_t g = (0xff & (texel>>8));
            uint8_t b = (0xff & texel);
            r = (uint8_t) ((int16_t)(Fix16((int16_t)r) * lightInstensity));
            g = (uint8_t) ((int16_t)(Fix16((int16_t)g) * lightInstensity));
            b = (uint8_t) ((int16_t)(Fix16((int16_t)b) * lightInstensity));
            auto c = color(r,g,b);
            setPixel(x0, y, c);
        }
        return;
    }

    for (int x = x0; x <= x1; x++) {
        int alpha = (x - x0) * 65536 / (x1 - x0);
        int u = ((u1 - u0) * alpha + u0 * 65536) >> 16;
        int v = ((v1 - v0) * alpha + v0 * 65536) >> 16;

        if (u >= 0 && u < textureWidth && v >= 0 && v < textureHeight) {
            auto texel = texture[u + v * textureWidth];
            uint8_t r = (0xff & (texel>>16));
            uint8_t g = (0xff & (texel>>8));
            uint8_t b = (0xff & texel);
            r = (uint8_t) ((int16_t)(Fix16((int16_t)r) * lightInstensity));
            g = (uint8_t) ((int16_t)(Fix16((int16_t)g) * lightInstensity));
            b = (uint8_t) ((int16_t)(Fix16((int16_t)b) * lightInstensity));
            auto c = color(r,g,b);
            setPixel(x, y, c);
        }
    }
}

void drawTriangle(
    Point2d v0, Point2d v1, Point2d v2,
    uint32_t *texture, int textureWidth, int textureHeight,
    Fix16 lightInstensity = 1.0f
) {
    if (v0.y > v1.y) swap(v0, v1);
    if (v0.y > v2.y) swap(v0, v2);
    if (v1.y > v2.y) swap(v1, v2);

    int totalHeight = v2.y - v0.y;

    // Avoid division by zero
    if (totalHeight == 0) return;

    // Drawing the upper part of the triangle
    for (int y = v0.y; y <= v1.y; y++) {
        int segmentHeight = v1.y - v0.y + 1;
        int alpha = ((y - v0.y) << 16) / totalHeight;
        int beta = ((y - v0.y) << 16) / segmentHeight;

        int x0 = v0.x + ((v2.x - v0.x) * alpha >> 16);
        int x1 = v0.x + ((v1.x - v0.x) * beta >> 16);

        int u0 = v0.u + ((v2.u - v0.u) * alpha >> 16);
        int u1 = v0.u + ((v1.u - v0.u) * beta >> 16);

        int v0_coord = v0.v + ((v2.v - v0.v) * alpha >> 16);
        int v1_coord = v0.v + ((v1.v - v0.v) * beta >> 16);

        drawHorizontalLine(x0, x1, y, u0, u1, v0_coord, v1_coord, texture, textureWidth, textureHeight, lightInstensity);
    }

    // Drawing the lower part of the triangle
    for (int y = v1.y + 1; y <= v2.y; y++) {
        int segmentHeight = v2.y - v1.y + 1;
        int alpha = ((y - v0.y) << 16) / totalHeight;
        int beta = ((y - v1.y) << 16) / segmentHeight;

        int x0 = v0.x + ((v2.x - v0.x) * alpha >> 16);
        int x1 = v1.x + ((v2.x - v1.x) * beta >> 16);

        int u0 = v0.u + ((v2.u - v0.u) * alpha >> 16);
        int u1 = v1.u + ((v2.u - v1.u) * beta >> 16);

        int v0_coord = v0.v + ((v2.v - v0.v) * alpha >> 16);
        int v1_coord = v1.v + ((v2.v - v1.v) * beta >> 16);

        drawHorizontalLine(x0, x1, y, u0, u1, v0_coord, v1_coord, texture, textureWidth, textureHeight, lightInstensity);
    }
}

bool DEBUG_TEST(
#ifdef PC
    SDL_Texture  *texture,
    SDL_Renderer *renderer
#endif
)
{
    return false;

    // True  = RUN ONLY THIS TEST AND STOP AFTERWARDS
    // False = CONTINUE WITH NORMAL MAIN
//     const int16_t OFFSET = 160;
//     const int16_t SIZE = 150;
//     /*
//                    v2
//                 _ /|
//             _ /    |
//         _ /        |
//       /            |
//     v0 ------------ v1
//     texture png corners
//         (0,0)  (1,0)
//
//         (0,1)  (1,1)
//     */
// #define UPPER_HALF
// #ifdef LOWER_HALF
//     // Lower half
//     const float rot_offset = -0.8f;
//     Point2d v0 = {0,0,  gen_textureWidth*0,gen_textureHeight*1};
//     Point2d v1 = {0,0,  gen_textureWidth*1,gen_textureHeight*1};
//     Point2d v2 = {0,0,  gen_textureWidth*1,gen_textureHeight*0};
// #else
//     // Upper half
//     const float rot_offset = +2.35f;
//     Point2d v0 = {0,0,  gen_textureWidth * 1, gen_textureHeight * 0};
//     Point2d v1 = {0,0,  gen_textureWidth * 0, gen_textureHeight * 0};
//     Point2d v2 = {0,0,  gen_textureWidth * 0, gen_textureHeight * 1};
// #endif
// #ifdef PC
//     std::cout << "rd_bytes:   0x" << std::hex << 123 << std::endl;
// #else
//     Debug_Printf(1,1, false, 0, "rd_bytes:   0x%x", 123);
// #endif
//     Fix16 rad1 = Fix16(0.0f+rot_offset);
//     Fix16 rad2 = Fix16(1.57f+rot_offset);
//     Fix16 rad3 = Fix16(3.14f+rot_offset);
// #ifdef PC
//     SDL_Event event;
// #endif
//     bool done = false;
//     while (!done)
//     {
// #ifdef PC
//         rad1 += 0.0011f;
//         rad2 += 0.0011f;
//         rad3 += 0.0011f;
// #else
//         rad1 += 0.0011f*29.0f;
//         rad2 += 0.0011f*29.0f;
//         rad3 += 0.0011f*29.0f;
// #endif
//         Fix16 v0_x = rad1.sin() * (float) SIZE;
//         Fix16 v0_y = rad1.cos() * (float) SIZE;
//         Fix16 v1_x = rad2.sin() * (float) SIZE;
//         Fix16 v1_y = rad2.cos() * (float) SIZE;
//         Fix16 v2_x = rad3.sin() * (float) SIZE;
//         Fix16 v2_y = rad3.cos() * (float) SIZE;
//         v0.x = (int16_t) v0_x + OFFSET;
//         v0.y = (int16_t) v0_y + OFFSET;
//         v1.x = (int16_t) v1_x + OFFSET;
//         v1.y = (int16_t) v1_y + OFFSET;
//         v2.x = (int16_t) v2_x + OFFSET;
//         v2.y = (int16_t) v2_y + OFFSET;
//         fillScreen(FILL_SCREEN_COLOR);
// #ifdef PC
//         drawTriangle(v0, v1, v2, gen_uv_tex, gen_textureWidth, gen_textureHeight);
//         SDL_UpdateTexture(texture, NULL, screenPixels, SCREEN_X * sizeof(Uint32));
//         SDL_RenderClear(renderer);
//         SDL_RenderCopy(renderer, texture, NULL, NULL);
//         SDL_RenderPresent(renderer);
//         SDL_PollEvent(&event);
//         switch( event.type ){
//             case SDL_KEYDOWN:
//                 switch( event.key.keysym.sym ){
//                     case SDLK_ESCAPE:
//                         done = true;
//                         break;
//                     default:
//                         break;
//                 }
//                 break;
//             /* Keyboard event */
//             case SDL_QUIT:
//                 done = true;
//                 break;
//             default:
//                 break;
//         }
// #else
//         drawTriangle(v0, v1, v2, gen_uv_tex, gen_textureWidth, gen_textureHeight);
//         uint32_t k1,k2; getKey(&k1,&k2);
//         if(testKey(k1,k2,KEY_CLEAR)) {
//             done = true;
//         }
//         LCD_Refresh();
// #endif
//     }
//     return true;
//     // True  = EXIT
//     // False = CONTINUE
}

void draw_RotationVisualizer(fix16_vec2 camera_rot)
{
    const float line_width = 20.0f;
    const int16_t right_edge_offset = 15;
    // Points to rotate
    fix16_vec3 p_x     = {line_width,  0.0f,  0.0f};
    fix16_vec3 p_y     = { 0.0f, -line_width,  0.0f};
    fix16_vec3 p_z     = { 0.0f,  0.0f, line_width};
    // Rotations - x
    rotateOnPlane(p_x.x, p_x.z, camera_rot.x);
    rotateOnPlane(p_x.y, p_x.z, camera_rot.y);
    // Rotations - y
    rotateOnPlane(p_y.x, p_y.z, camera_rot.x);
    rotateOnPlane(p_y.y, p_y.z, camera_rot.y);
    // Rotations - z
    rotateOnPlane(p_z.x, p_z.z, camera_rot.x);
    rotateOnPlane(p_z.y, p_z.z, camera_rot.y);
    // Make sure there is no division with zero
    if (p_x.z == 0.0f) p_x.z = 0.001f;
    if (p_y.z == 0.0f) p_y.z = 0.001f;
    if (p_z.z == 0.0f) p_z.z = 0.001f;
    // Where to draw
    const auto offset_x = SCREEN_X - right_edge_offset - (int16_t) line_width;
    const auto offset_y =            right_edge_offset + (int16_t) line_width;
    // Draw actual lines
    line(((int16_t) p_x.x)+offset_x,((int16_t) p_x.y)+offset_y, offset_x, offset_y, color(255,0,0));
    line(((int16_t) p_y.x)+offset_x,((int16_t) p_y.y)+offset_y, offset_x, offset_y, color(0,255,0));
    line(((int16_t) p_z.x)+offset_x,((int16_t) p_z.y)+offset_y, offset_x, offset_y, color(0,0,255));
}

void draw_SunLocation(Fix16 FOV, fix16_vec3 lightPos, fix16_vec3 camera_pos, fix16_vec2 camera_rot)
{
    Fix16 z_depth;
    auto screen_vec2 = getScreenCoordinate(
        FOV, lightPos,
        {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f},
        camera_pos, camera_rot,
        &z_depth
    );
    int16_t x = (int16_t)screen_vec2.x;
    int16_t y = (int16_t)screen_vec2.y;
    draw_center_square(x, y, 9,9, color(238,210,2));
}

#ifndef PC
void custom_init()
{
    calcInit(); //backup screen and init some variables
    if (DEBUG_TEST()){
        while(true){
            uint32_t k1,k2; getKey(&k1,&k2);
            if(testKey(k1,k2,KEY_CLEAR)) {
                break;
            }
        }
        calcEnd(); //restore screen and do stuff
        return;
    }
}
#else
int custom_init(SDL_Window **window, SDL_Renderer **renderer, SDL_Texture ** texture)
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
        return -1;

    // Create a window
    *window = SDL_CreateWindow("Classpad II PC demo",
                                SDL_WINDOWPOS_UNDEFINED,
                                SDL_WINDOWPOS_UNDEFINED,
                                SCREEN_X*2,
                                SCREEN_Y*2,
                                SDL_WINDOW_OPENGL);
    if (*window == nullptr)
    {
        SDL_Log("Could not create a window: %s", SDL_GetError());
        return -1;
    }

    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr)
    {
        SDL_Log("Could not create a renderer: %s", SDL_GetError());
        return -1;
    }

    *texture = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, SCREEN_X, SCREEN_Y);

    // IN Debug test we can try out things
    if (DEBUG_TEST(*texture, *renderer))
        done = true;

    return 0;
}
#endif

#ifndef PC
extern "C" void main()
{
    bool done = false;
    custom_init();
#else // ifdef PC
int main(int argc, const char * argv[])
{
    bool done = false;

    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture * texture;
    screenPixels = new Uint32[SCREEN_X * SCREEN_Y];
    int status = custom_init(&window, &renderer, &texture);
    if (status != 0) return status;

    bool key_left = false;
    bool key_right = false;
    bool key_up = false;
    bool key_down = false;
    bool key_r = false;
    bool key_f = false;
    bool key_1 = false;
    bool key_2 = false;
    bool key_w = false;
    bool key_s = false;
    bool key_a = false;
    bool key_d = false;
    bool key_e = false;
#endif // PC

    Fix16 last_dt = Fix16((int16_t) 0.01f);

    char model_path[] =
#ifdef PC
        "./little_endian.pkObj";
#else
        "\\fls0\\big_endian.pkObj";
#endif

    char model_texture_path[] =
#ifdef PC
        "./little_endian.texture";
#else
        "\\fls0\\big_endian.texture";
#endif

    fillScreen(FILL_SCREEN_COLOR);
#ifndef PC
    Debug_SetCursorPosition(1,1);
    Debug_PrintString("Load obj", false);
    LCD_Refresh();
#else
    SDL_UpdateTexture(texture, NULL, screenPixels, SCREEN_X * sizeof(Uint32));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
#endif

    // Test obj model
    Model model_test  = Model(model_path, model_texture_path);
    model_test.getRotation_ref().y = Fix16(3.145f/2.0f);

    // -----------------
    // Models to render
    Model* all_models[] = {
        //&model_floor,
        // &cube1, &testmodel, &model_test
        &model_test
    };
    const unsigned all_model_count = sizeof(all_models) / sizeof(all_models[0]);

    fix16_vec3 camera_pos = {-6.0f, -1.6f, -8.0f};
    fix16_vec2 camera_rot = {0.6f, 0.4f};

    Fix16 FOV = 300.0f; // Does not mean 300 degrees, some "arbitrary" meaning
#ifdef PC
    uint32_t startTime = SDL_GetTicks();
    int frames = 0;
    // last_dt = 1.0f / last_fps -> last_fps can not be zero. Initializing to some random value.
    // last_dt can never be zero anyway so its fine.
    uint32_t last_fps = 500; // Some high value is better as then there wont be super fast movements in beginning
#endif

    bool KEY_RENDER_MODE_prev = false;

    fix16_vec3 lightPos = {0.0f, 0.0f, 0.0f};
    Fix16 lightRotation = 0.0f;

    const uint16_t RENDER_MODE_COUNT = 7;
    uint16_t RENDER_MODE = RENDER_MODE_COUNT-1;
    while(!done)
    {
        #ifdef PC
        uint32_t start = SDL_GetTicks();
        #endif

        #ifndef PC
        fps_update();
        #endif

        fillScreen(FILL_SCREEN_COLOR);
        // --------------------------
        lightRotation += last_dt * 1.2f;

        lightPos.x = lightRotation.sin() * -8.0f;
        lightPos.y = -10.0f;
        lightPos.z = lightRotation.cos() * -8.0f;

        // ----- Check for exit -----
#ifndef PC
        uint32_t k1,k2; getKey(&k1,&k2);
        if(testKey(k1,k2,KEY_CLEAR)) {
            done = true;
        }
#endif // !PC

#ifdef PC
        // Get the next event
        SDL_Event event;
        if (SDL_PollEvent(&event))
        {
            switch( event.type ){
                /* Keyboard event */
                /* Pass the event data onto PrintKeyInfo() */
                case SDL_KEYDOWN:
                    switch( event.key.keysym.sym ){
                        case SDLK_LEFT:  key_left  = true; break;
                        case SDLK_RIGHT: key_right = true; break;
                        case SDLK_UP:    key_up    = true; break;
                        case SDLK_DOWN:  key_down  = true; break;
                        case SDLK_r:     key_r     = true; break;
                        case SDLK_f:     key_f     = true; break;
                        case SDLK_a:     key_a     = true; break;
                        case SDLK_d:     key_d     = true; break;
                        case SDLK_w:     key_w     = true; break;
                        case SDLK_s:     key_s     = true; break;
                        case SDLK_1:     key_1     = true; break;
                        case SDLK_2:     key_2     = true; break;
                        case SDLK_e:     key_e     = true; break;
                        // STOP
                        case SDLK_ESCAPE:
                            done = true;
                            break;
                        default:                           break;
                    }
                    break;
                case SDL_KEYUP:
                    switch( event.key.keysym.sym ){
                        case SDLK_LEFT:  key_left  = false; break;
                        case SDLK_RIGHT: key_right = false; break;
                        case SDLK_UP:    key_up    = false; break;
                        case SDLK_DOWN:  key_down  = false; break;
                        case SDLK_r:     key_r     = false; break;
                        case SDLK_f:     key_f     = false; break;
                        case SDLK_a:     key_a     = false; break;
                        case SDLK_d:     key_d     = false; break;
                        case SDLK_w:     key_w     = false; break;
                        case SDLK_s:     key_s     = false; break;
                        case SDLK_1:     key_1     = false; break;
                        case SDLK_2:     key_2     = false; break;
                        case SDLK_e:     key_e     = false; break;
                        default:                           break;
                    }
                    break;
                case SDL_QUIT:
                    done = true;
                    break;
                default:
                    break;
            }
        }
#endif // PC

// ------------------ KEY PRESSES ----------------------

        if(KEY_MOVE_LEFT)  {
            camera_pos.z += camera_rot.x.sin() * last_dt * MOVEMENT_SPEED;
            camera_pos.x -= camera_rot.x.cos() * last_dt * MOVEMENT_SPEED;
        }
        if(KEY_MOVE_RIGHT) {
            camera_pos.z -= camera_rot.x.sin() * last_dt * MOVEMENT_SPEED;
            camera_pos.x += camera_rot.x.cos() * last_dt * MOVEMENT_SPEED;
        }
        if(KEY_MOVE_FORWARD)    {
            camera_pos.x += camera_rot.x.sin() * last_dt * MOVEMENT_SPEED;
            camera_pos.z += camera_rot.x.cos() * last_dt * MOVEMENT_SPEED;
        }
        if(KEY_MOVE_BACKWARD)  {
            camera_pos.x -= camera_rot.x.sin() * last_dt * MOVEMENT_SPEED;
            camera_pos.z -= camera_rot.x.cos() * last_dt * MOVEMENT_SPEED;
        }
        if(KEY_MOVE_UP)
            camera_pos.y -= last_dt * MOVEMENT_SPEED;
        if(KEY_MOVE_DOWN)
            camera_pos.y += last_dt * MOVEMENT_SPEED;

        if(KEY_MOVE_FOV_ADD)
            FOV += last_dt * FOV_UPDATE_SPEED;
        if(KEY_MOVE_FOV_SUB)
            FOV -= last_dt * FOV_UPDATE_SPEED;

        if(KEY_MOVE_REND_MODE){
            if(KEY_RENDER_MODE_prev == false){
                if(RENDER_MODE > 0)
                    RENDER_MODE = RENDER_MODE - 1;
                else
                    RENDER_MODE = RENDER_MODE_COUNT - 1;
            }
            KEY_RENDER_MODE_prev = true;
        }else{
            KEY_RENDER_MODE_prev = false;
        }

        if(KEY_ROTATE_LEFT)
            camera_rot.x -= last_dt * CAMERA_SPEED;
        if(KEY_ROTATE_RIGHT)
            camera_rot.x += last_dt * CAMERA_SPEED;
        if(KEY_ROTATE_UP)
            camera_rot.y -= last_dt * CAMERA_SPEED;
        if(KEY_ROTATE_DOWN)
            camera_rot.y += last_dt * CAMERA_SPEED;

// --------------------------------------------------

        all_models[all_model_count-1]->getRotation_ref().x += last_dt * 0.5f;
        //all_models[all_model_count-1]->getRotation_ref().y += 0.0003f;

        if (RENDER_MODE == 0)
        {
            for (unsigned m_id=0; m_id<all_model_count; m_id++)
            {
                // Check first if model has texture
                if (all_models[m_id]->uv_coord_count == 0){
                    continue;
                }
                // For each model..
                // Allocate memory first:
                //   1. Screen coordinates
                int16_t_vec2* screen_coords = (int16_t_vec2*) malloc(sizeof(int16_t_vec2) * all_models[m_id]->vertex_count);
                //   2. Draw order list
                Fix16 * vert_z_depths = (Fix16*) malloc(sizeof(Fix16) * all_models[m_id]->vertex_count);
                //   3. Face draw order (to be created)
                uint_fix16_t * face_draw_order = (uint_fix16_t*) malloc(sizeof(uint_fix16_t) * all_models[m_id]->faces_count);
                //   4. Face Normals
                fix16_vec3* face_normals = (fix16_vec3*) malloc(sizeof(fix16_vec3) * all_models[m_id]->faces_count);
                // Get screen coordinates
                for (unsigned v_id=0; v_id<all_models[m_id]->vertex_count; v_id++){
                    fix16_vec2 screen_vec2;
                    screen_vec2 = getScreenCoordinate(
                        FOV, all_models[m_id]->vertices[v_id],
                        all_models[m_id]->getPosition_ref(), all_models[m_id]->getRotation_ref(),
                        all_models[m_id]->getScale_ref(),
                        camera_pos, camera_rot,
                        &vert_z_depths[v_id]
                    );
                    int16_t x = (int16_t)screen_vec2.x;
                    int16_t y = (int16_t)screen_vec2.y;
                    screen_coords[v_id] = {x, y};
                }

                // Init the face_draw_order
                // -- CRUDE ORDERING (Work In Progress)
                for (unsigned f_id=0; f_id<all_models[m_id]->faces_count; f_id++)
                {
                    // CRUDELY only choosing first vertex
                    unsigned int f_v0_id = all_models[m_id]->faces[f_id].First;
                    unsigned int f_v1_id = all_models[m_id]->faces[f_id].Second;
                    unsigned int f_v2_id = all_models[m_id]->faces[f_id].Third;
                    // Get face z-depth
                    Fix16 f_z_depth  = vert_z_depths[f_v0_id]/3.0f;
                    f_z_depth       += vert_z_depths[f_v1_id]/3.0f;
                    f_z_depth       += vert_z_depths[f_v2_id]/3.0f;

                    // Init index = f_id
                    face_draw_order[f_id].uint = f_id;
                    face_draw_order[f_id].fix16 = f_z_depth;

                    // ----- Calculate also face normals here

                    // Face vertices
                    fix16_vec3 v0 = all_models[m_id]->vertices[f_v0_id];
                    fix16_vec3 v1 = all_models[m_id]->vertices[f_v1_id];
                    fix16_vec3 v2 = all_models[m_id]->vertices[f_v2_id];
                    // Model rotation
                    rotateOnPlane(v0.x, v0.z, all_models[m_id]->rotation.x);
                    rotateOnPlane(v0.y, v0.z, all_models[m_id]->rotation.y);
                    rotateOnPlane(v1.x, v1.z, all_models[m_id]->rotation.x);
                    rotateOnPlane(v1.y, v1.z, all_models[m_id]->rotation.y);
                    rotateOnPlane(v2.x, v2.z, all_models[m_id]->rotation.x);
                    rotateOnPlane(v2.y, v2.z, all_models[m_id]->rotation.y);
                    // Model traslation
                    v0.x += all_models[m_id]->position.x;
                    v0.y += all_models[m_id]->position.y;
                    v0.z += all_models[m_id]->position.z;
                    v1.x += all_models[m_id]->position.x;
                    v1.y += all_models[m_id]->position.y;
                    v1.z += all_models[m_id]->position.z;
                    v2.x += all_models[m_id]->position.x;
                    v2.y += all_models[m_id]->position.y;
                    v2.z += all_models[m_id]->position.z;
                    // Calculate face normal
                    auto face_norm = calculateNormal(v0, v1, v2);
                    normalize_fix16_vec3(face_norm);
                    //
                    face_normals[f_id] = face_norm;
                }
                // Sorting
                bubble_sort(face_draw_order, all_models[m_id]->faces_count);
                //heapSort(face_draw_order, all_models[m_id]->faces_count);
                //shellSort(face_draw_order, all_models[m_id]->faces_count);

                // Draw face edges
                for (unsigned int ordered_id=0; ordered_id<all_models[m_id]->faces_count; ordered_id++)
                {
                    auto f_id = face_draw_order[ordered_id].uint;
                    const auto v0 = screen_coords[all_models[m_id]->faces[f_id].First];
                    const auto v1 = screen_coords[all_models[m_id]->faces[f_id].Second];
                    const auto v2 = screen_coords[all_models[m_id]->faces[f_id].Third];
                    const int16_t fix16_cast_int_min = (0xffff & (fix16_minimum>>16)) - 1;
                    if( v0.x == fix16_cast_int_min ||
                        v1.x == fix16_cast_int_min ||
                        v2.x == fix16_cast_int_min
                    ){
                        continue;
                    }
                    auto uv0_fix16_norm = all_models[m_id]->uv_coords[all_models[m_id]->uv_faces[f_id].First];
                    auto uv1_fix16_norm = all_models[m_id]->uv_coords[all_models[m_id]->uv_faces[f_id].Second];
                    auto uv2_fix16_norm = all_models[m_id]->uv_coords[all_models[m_id]->uv_faces[f_id].Third];

                    auto v0_u = (int16_t) (uv0_fix16_norm.x * (Fix16((int16_t)all_models[m_id]->gen_textureWidth)));
                    auto v0_v = (int16_t) (uv0_fix16_norm.y * (Fix16((int16_t)all_models[m_id]->gen_textureHeight)));

                    auto v1_u = (int16_t) (uv1_fix16_norm.x * (Fix16((int16_t)all_models[m_id]->gen_textureWidth)));
                    auto v1_v = (int16_t) (uv1_fix16_norm.y * (Fix16((int16_t)all_models[m_id]->gen_textureHeight)));

                    auto v2_u = (int16_t) (uv2_fix16_norm.x * (Fix16((int16_t)all_models[m_id]->gen_textureWidth)));
                    auto v2_v = (int16_t) (uv2_fix16_norm.y * (Fix16((int16_t)all_models[m_id]->gen_textureHeight)));

                    Point2d v0_screen = {v0.x,v0.y, v0_u, v0_v};
                    Point2d v1_screen = {v1.x,v1.y, v1_u, v1_v};
                    Point2d v2_screen = {v2.x,v2.y, v2_u, v2_v};

                    const auto face_pos = all_models[m_id]->vertices[all_models[m_id]->faces[f_id].First];
                    Fix16 lightIntensity = calculateLightIntensity(
                         lightPos, face_pos, face_normals[f_id], Fix16(1.0f)
                    );

                    drawTriangle(
                        v0_screen, v1_screen, v2_screen,
                        //gen_uv_tex, gen_textureWidth, gen_textureHeight
                        all_models[m_id]->gen_uv_tex,
                        all_models[m_id]->gen_textureWidth,
                        all_models[m_id]->gen_textureHeight,
                        lightIntensity
                    );
                }
                free(face_draw_order);
                free(vert_z_depths);
                free(screen_coords);
                free(face_normals);
            }

            // Draw sun visualizer
            draw_SunLocation(FOV, lightPos, camera_pos, camera_rot);

        }  // if (RENDER_MODE == 0)

        else if (RENDER_MODE == 1){
            for (unsigned m_id=0; m_id<all_model_count; m_id++)
            {
                // Check first if model has texture
                if (all_models[m_id]->uv_coord_count == 0){
                    continue;
                }
                // For each model..
                // Allocate memory first:
                //   1. Screen coordinates
                int16_t_vec2* screen_coords = (int16_t_vec2*) malloc(sizeof(int16_t_vec2) * all_models[m_id]->vertex_count);
                //   2. Draw order list
                Fix16 * vert_z_depths = (Fix16*) malloc(sizeof(Fix16) * all_models[m_id]->vertex_count);
                //   3. Face draw order (to be created)
                uint_fix16_t * face_draw_order = (uint_fix16_t*) malloc(sizeof(uint_fix16_t) * all_models[m_id]->faces_count);

                // Get screen coordinates
                for (unsigned v_id=0; v_id<all_models[m_id]->vertex_count; v_id++){
                    fix16_vec2 screen_vec2;
                    screen_vec2 = getScreenCoordinate(
                        FOV, all_models[m_id]->vertices[v_id],
                        all_models[m_id]->getPosition_ref(), all_models[m_id]->getRotation_ref(),
                        all_models[m_id]->getScale_ref(),
                        camera_pos, camera_rot,
                        &vert_z_depths[v_id]
                    );
                    int16_t x = (int16_t)screen_vec2.x;
                    int16_t y = (int16_t)screen_vec2.y;
                    screen_coords[v_id] = {x, y};
                }

                // Init the face_draw_order
                // -- CRUDE ORDERING (Work In Progress)
                for (unsigned f_id=0; f_id<all_models[m_id]->faces_count; f_id++)
                {
                    // CRUDELY only choosing first vertex
                    unsigned int f_v0_id = all_models[m_id]->faces[f_id].First;
                    unsigned int f_v1_id = all_models[m_id]->faces[f_id].Second;
                    unsigned int f_v2_id = all_models[m_id]->faces[f_id].Third;
                    // Get face z-depth
                    Fix16 f_z_depth  = vert_z_depths[f_v0_id]/3.0f;
                    f_z_depth       += vert_z_depths[f_v1_id]/3.0f;
                    f_z_depth       += vert_z_depths[f_v2_id]/3.0f;

                    // Init index = f_id
                    face_draw_order[f_id].uint = f_id;
                    face_draw_order[f_id].fix16 = f_z_depth;
                }
                // Sorting
                bubble_sort(face_draw_order, all_models[m_id]->faces_count);
                //heapSort(face_draw_order, all_models[m_id]->faces_count);
                //shellSort(face_draw_order, all_models[m_id]->faces_count);

                // Draw face edges
                for (unsigned int ordered_id=0; ordered_id<all_models[m_id]->faces_count; ordered_id++)
                {
                    auto f_id = face_draw_order[ordered_id].uint;
                    const auto v0 = screen_coords[all_models[m_id]->faces[f_id].First];
                    const auto v1 = screen_coords[all_models[m_id]->faces[f_id].Second];
                    const auto v2 = screen_coords[all_models[m_id]->faces[f_id].Third];
                    const int16_t fix16_cast_int_min = (0xffff & (fix16_minimum>>16)) - 1;
                    if( v0.x == fix16_cast_int_min ||
                        v1.x == fix16_cast_int_min ||
                        v2.x == fix16_cast_int_min
                    ){
                        continue;
                    }
                    auto uv0_fix16_norm = all_models[m_id]->uv_coords[all_models[m_id]->uv_faces[f_id].First];
                    auto uv1_fix16_norm = all_models[m_id]->uv_coords[all_models[m_id]->uv_faces[f_id].Second];
                    auto uv2_fix16_norm = all_models[m_id]->uv_coords[all_models[m_id]->uv_faces[f_id].Third];

                    auto v0_u = (int16_t) (uv0_fix16_norm.x * (Fix16((int16_t)all_models[m_id]->gen_textureWidth)));
                    auto v0_v = (int16_t) (uv0_fix16_norm.y * (Fix16((int16_t)all_models[m_id]->gen_textureHeight)));

                    auto v1_u = (int16_t) (uv1_fix16_norm.x * (Fix16((int16_t)all_models[m_id]->gen_textureWidth)));
                    auto v1_v = (int16_t) (uv1_fix16_norm.y * (Fix16((int16_t)all_models[m_id]->gen_textureHeight)));

                    auto v2_u = (int16_t) (uv2_fix16_norm.x * (Fix16((int16_t)all_models[m_id]->gen_textureWidth)));
                    auto v2_v = (int16_t) (uv2_fix16_norm.y * (Fix16((int16_t)all_models[m_id]->gen_textureHeight)));

                    Point2d v0_screen = {v0.x,v0.y, v0_u, v0_v};
                    Point2d v1_screen = {v1.x,v1.y, v1_u, v1_v};
                    Point2d v2_screen = {v2.x,v2.y, v2_u, v2_v};

                    drawTriangle(
                        v0_screen, v1_screen, v2_screen,
                        //gen_uv_tex, gen_textureWidth, gen_textureHeight
                        all_models[m_id]->gen_uv_tex,
                        all_models[m_id]->gen_textureWidth,
                        all_models[m_id]->gen_textureHeight
                    );
                }
                free(face_draw_order);
                free(vert_z_depths);
                free(screen_coords);
            }
        }  // else if (RENDER_MODE == 1)

        else if (RENDER_MODE == 2){
            for (unsigned m_id=0; m_id<all_model_count; m_id++)
            {
                // For each model..
                // Allocate memory first:
                //   1. Screen coordinates
                int16_t_vec2* screen_coords = (int16_t_vec2*) malloc(sizeof(int16_t_vec2) * all_models[m_id]->vertex_count);
                //   2. Draw order list
                Fix16 * vert_z_depths = (Fix16*) malloc(sizeof(Fix16) * all_models[m_id]->vertex_count);
                //   3. Face draw order (to be created)
                uint_fix16_t * face_draw_order = (uint_fix16_t*) malloc(sizeof(uint_fix16_t) * all_models[m_id]->faces_count);
                //   4. Face Normals
                fix16_vec3* face_normals = (fix16_vec3*) malloc(sizeof(fix16_vec3) * all_models[m_id]->faces_count);

                // Get screen coordinates
                for (unsigned v_id=0; v_id<all_models[m_id]->vertex_count; v_id++){
                    fix16_vec2 screen_vec2;
                    screen_vec2 = getScreenCoordinate(
                        FOV, all_models[m_id]->vertices[v_id],
                        all_models[m_id]->getPosition_ref(), all_models[m_id]->getRotation_ref(),
                        all_models[m_id]->getScale_ref(),
                        camera_pos, camera_rot,
                        &vert_z_depths[v_id]
                    );
                    int16_t x = (int16_t)screen_vec2.x;
                    int16_t y = (int16_t)screen_vec2.y;
                    screen_coords[v_id] = {x, y};
                }
                // Init the face_draw_order
                // -- CRUDE ORDERING (Work In Progress)
                for (unsigned f_id=0; f_id<all_models[m_id]->faces_count; f_id++)
                {
                    // CRUDELY only choosing first vertex
                    unsigned int f_v0_id = all_models[m_id]->faces[f_id].First;
                    unsigned int f_v1_id = all_models[m_id]->faces[f_id].Second;
                    unsigned int f_v2_id = all_models[m_id]->faces[f_id].Third;
                    // Get face z-depth
                    Fix16 f_z_depth  = vert_z_depths[f_v0_id]/3.0f;
                    f_z_depth       += vert_z_depths[f_v1_id]/3.0f;
                    f_z_depth       += vert_z_depths[f_v2_id]/3.0f;

                    // Init index = f_id
                    face_draw_order[f_id].uint = f_id;
                    face_draw_order[f_id].fix16 = f_z_depth;

                    // ----- Calculate also face normals here

                    // Face vertices
                    fix16_vec3 v0 = all_models[m_id]->vertices[f_v0_id];
                    fix16_vec3 v1 = all_models[m_id]->vertices[f_v1_id];
                    fix16_vec3 v2 = all_models[m_id]->vertices[f_v2_id];
                    // Model rotation
                    rotateOnPlane(v0.x, v0.z, all_models[m_id]->rotation.x);
                    rotateOnPlane(v0.y, v0.z, all_models[m_id]->rotation.y);
                    rotateOnPlane(v1.x, v1.z, all_models[m_id]->rotation.x);
                    rotateOnPlane(v1.y, v1.z, all_models[m_id]->rotation.y);
                    rotateOnPlane(v2.x, v2.z, all_models[m_id]->rotation.x);
                    rotateOnPlane(v2.y, v2.z, all_models[m_id]->rotation.y);
                    // Model traslation
                    v0.x += all_models[m_id]->position.x;
                    v0.y += all_models[m_id]->position.y;
                    v0.z += all_models[m_id]->position.z;
                    v1.x += all_models[m_id]->position.x;
                    v1.y += all_models[m_id]->position.y;
                    v1.z += all_models[m_id]->position.z;
                    v2.x += all_models[m_id]->position.x;
                    v2.y += all_models[m_id]->position.y;
                    v2.z += all_models[m_id]->position.z;
                    // Calculate face normal
                    auto face_norm = calculateNormal(v0, v1, v2);
                    normalize_fix16_vec3(face_norm);
                    //
                    face_normals[f_id] = face_norm;
                }
                // Sorting
                bubble_sort(face_draw_order, all_models[m_id]->faces_count);

                // Draw face edges
                for (unsigned int ordered_id=0; ordered_id<all_models[m_id]->faces_count; ordered_id++)
                {
                    auto f_id = face_draw_order[ordered_id].uint;
                    const auto v0 = screen_coords[all_models[m_id]->faces[f_id].First];
                    const auto v1 = screen_coords[all_models[m_id]->faces[f_id].Second];
                    const auto v2 = screen_coords[all_models[m_id]->faces[f_id].Third];
                    const int16_t fix16_cast_int_min = (0xffff & (fix16_minimum>>16)) - 1;
                    if( v0.x == fix16_cast_int_min ||
                        v1.x == fix16_cast_int_min ||
                        v2.x == fix16_cast_int_min
                    ){
                        continue;
                    }

                    const auto face_pos = all_models[m_id]->vertices[all_models[m_id]->faces[f_id].First];
                    Fix16 lightIntensity = calculateLightIntensity(
                         lightPos, face_pos, face_normals[f_id], Fix16(1.0f)
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
            }
            // Draw sun visualizer
            draw_SunLocation(FOV, lightPos, camera_pos, camera_rot);
        }  // else if (RENDER_MODE == 2)

        else if (RENDER_MODE == 3){
            for (unsigned m_id=0; m_id<all_model_count; m_id++)
            {
                // For each model..
                // Allocate memory first:
                //   1. Screen coordinates
                int16_t_vec2* screen_coords = (int16_t_vec2*) malloc(sizeof(int16_t_vec2) * all_models[m_id]->vertex_count);
                //   2. Draw order list
                Fix16 * vert_z_depths = (Fix16*) malloc(sizeof(Fix16) * all_models[m_id]->vertex_count);
                //   3. Face draw order (to be created)
                uint_fix16_t * face_draw_order = (uint_fix16_t*) malloc(sizeof(uint_fix16_t) * all_models[m_id]->faces_count);

                // Get screen coordinates
                for (unsigned v_id=0; v_id<all_models[m_id]->vertex_count; v_id++){
                    fix16_vec2 screen_vec2;
                    screen_vec2 = getScreenCoordinate(
                        FOV, all_models[m_id]->vertices[v_id],
                        all_models[m_id]->getPosition_ref(), all_models[m_id]->getRotation_ref(),
                        all_models[m_id]->getScale_ref(),
                        camera_pos, camera_rot,
                        &vert_z_depths[v_id]
                    );
                    int16_t x = (int16_t)screen_vec2.x;
                    int16_t y = (int16_t)screen_vec2.y;
                    screen_coords[v_id] = {x, y};
                }

                // Init the face_draw_order
                // -- CRUDE ORDERING (Work In Progress)
                for (unsigned f_id=0; f_id<all_models[m_id]->faces_count; f_id++)
                {
                    // CRUDELY only choosing first vertex
                    unsigned int f_v0_id = all_models[m_id]->faces[f_id].First;
                    unsigned int f_v1_id = all_models[m_id]->faces[f_id].Second;
                    unsigned int f_v2_id = all_models[m_id]->faces[f_id].Third;
                    // Get face z-depth
                    Fix16 f_z_depth  = vert_z_depths[f_v0_id]/3.0f;
                    f_z_depth       += vert_z_depths[f_v1_id]/3.0f;
                    f_z_depth       += vert_z_depths[f_v2_id]/3.0f;

                    // Init index = f_id
                    face_draw_order[f_id].uint = f_id;
                    face_draw_order[f_id].fix16 = f_z_depth;
                }
                // Sorting
                bubble_sort(face_draw_order, all_models[m_id]->faces_count);

                // Draw face edges
                for (unsigned int ordered_id=0; ordered_id<all_models[m_id]->faces_count; ordered_id++)
                {
                    auto f_id = face_draw_order[ordered_id].uint;
                    const auto v0 = screen_coords[all_models[m_id]->faces[f_id].First];
                    const auto v1 = screen_coords[all_models[m_id]->faces[f_id].Second];
                    const auto v2 = screen_coords[all_models[m_id]->faces[f_id].Third];
                    const int16_t fix16_cast_int_min = (0xffff & (fix16_minimum>>16)) - 1;
                    if( v0.x == fix16_cast_int_min ||
                        v1.x == fix16_cast_int_min ||
                        v2.x == fix16_cast_int_min
                    ){
                        continue;
                    }
                    uint32_t colorr =
                        0xff << (ordered_id*(24)/all_models[m_id]->faces_count);
                    triangle(
                        v0.x,v0.y,v1.x,v1.y,v2.x,v2.y,
                        color((colorr>>16)&0xcf, (colorr>>8)&0xcf, (colorr>>0)&0xcf),
                        color(0,0,0)
                    );
                }
                free(face_draw_order);
                free(vert_z_depths);
                free(screen_coords);
            }
        }  // else if (RENDER_MODE == 3)

        else if (RENDER_MODE == 4){
            for (unsigned m_id=0; m_id<all_model_count; m_id++)
            {
                // For each model..
                // Allocate memory first:
                //   1. Screen coordinates
                int16_t_vec2* screen_coords = (int16_t_vec2*) malloc(sizeof(int16_t_vec2) * all_models[m_id]->vertex_count);
                //   2. Draw order list
                Fix16 fix16_sink;
                // Get screen coordinates
                for (unsigned v_id=0; v_id<all_models[m_id]->vertex_count; v_id++){
                    fix16_vec2 screen_vec2;
                    screen_vec2 = getScreenCoordinate(
                        FOV, all_models[m_id]->vertices[v_id],
                        all_models[m_id]->getPosition_ref(), all_models[m_id]->getRotation_ref(),
                        all_models[m_id]->getScale_ref(),
                        camera_pos, camera_rot,
                        &fix16_sink
                    );
                    int16_t x = (int16_t)screen_vec2.x;
                    int16_t y = (int16_t)screen_vec2.y;
                    screen_coords[v_id] = {x, y};
                }

                for (unsigned int f_id=0; f_id<all_models[m_id]->faces_count; f_id++)
                {
                    const auto v0 = screen_coords[all_models[m_id]->faces[f_id].First];
                    const auto v1 = screen_coords[all_models[m_id]->faces[f_id].Second];
                    const auto v2 = screen_coords[all_models[m_id]->faces[f_id].Third];
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
            }
        }  // else if (RENDER_MODE == 4)

        else if (RENDER_MODE == 5){
            for (unsigned m_id=0; m_id<all_model_count; m_id++)
            {
                // For each model..
                // Allocate memory first:
                //   1. Screen coordinates
                int16_t_vec2* screen_coords = (int16_t_vec2*) malloc(sizeof(int16_t_vec2) * all_models[m_id]->vertex_count);
                //   2. Draw order list
                Fix16 fix16_sink;
                // Get screen coordinates
                for (unsigned v_id=0; v_id<all_models[m_id]->vertex_count; v_id++){
                    fix16_vec2 screen_vec2;
                    screen_vec2 = getScreenCoordinate(
                        FOV, all_models[m_id]->vertices[v_id],
                        all_models[m_id]->getPosition_ref(), all_models[m_id]->getRotation_ref(),
                        all_models[m_id]->getScale_ref(),
                        camera_pos, camera_rot,
                        &fix16_sink
                    );
                    int16_t x = (int16_t)screen_vec2.x;
                    int16_t y = (int16_t)screen_vec2.y;
                    screen_coords[v_id] = {x, y};
                }

                for (unsigned int f_id=0; f_id<all_models[m_id]->faces_count; f_id++)
                {
                    const auto v0 = screen_coords[all_models[m_id]->faces[f_id].First];
                    const auto v1 = screen_coords[all_models[m_id]->faces[f_id].Second];
                    const auto v2 = screen_coords[all_models[m_id]->faces[f_id].Third];
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
            }
        } // else if (RENDER_MODE == 5)

        else if (RENDER_MODE == 6){
            for (unsigned m_id=0; m_id<all_model_count; m_id++)
            {
                Fix16 fix16_sink;
                // Get screen coordinates
                for (unsigned v_id=0; v_id<all_models[m_id]->vertex_count; v_id++){
                    fix16_vec2 screen_vec2;
                    screen_vec2 = getScreenCoordinate(
                        FOV, all_models[m_id]->vertices[v_id],
                        all_models[m_id]->getPosition_ref(), all_models[m_id]->getRotation_ref(),
                        all_models[m_id]->getScale_ref(),
                        camera_pos, camera_rot,
                        &fix16_sink
                    );
                    int16_t x = (int16_t)screen_vec2.x;
                    int16_t y = (int16_t)screen_vec2.y;
                    const int16_t fix16_cast_int_min = (0xffff & (fix16_minimum>>16)) - 1;
                    if( x == fix16_cast_int_min  ){
                        continue;
                    }
                    draw_center_square(x,y,5,5, color(0,0,0));
                }

            }
        } // else if (RENDER_MODE == 6)

        // Draw rotation visualizer in corner
        draw_RotationVisualizer(camera_rot);

// --------------------------------------------------

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~ Display FPS ~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#ifndef PC
        // Note that this is directly yanked from <insert_someones_git_and_name>.
        // The whole "fps_functions.hpp" is taken.
        // I have not written FPS calculation functionality.
        fps_formatted_update();
        fps_display();
        last_dt = Fix16(1.0f) / (Fix16(((int16_t) fps10)) / 10.0f);
#else
        // SDL_GetTicks() seems not to be super accurate, so accumulating
        // frames and updating the frame counter every 250 ms
        const Uint32 FPS_UPDATE_FREQ_MS = 150;
        Uint32 currentTime = SDL_GetTicks();
        frames++;
        if (currentTime - startTime >= FPS_UPDATE_FREQ_MS) {
            float fps = frames / ((currentTime - startTime) / 1000.0f);
            last_fps = (uint32_t) fps;
            startTime = currentTime;
            frames = 0;
        }
        sdl_debug_uint32_t(last_fps, 10, 10);

        last_dt = Fix16(1.0f) / (Fix16(((int16_t) last_fps)));
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~ Refresh screen ~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#ifndef PC
        LCD_Refresh();
#else
        SDL_UpdateTexture(texture, NULL, screenPixels, SCREEN_X * sizeof(Uint32));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
#endif
    } // while(!done)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~ End Program ~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#ifndef PC
    calcEnd(); //restore screen and do stuff
#else
    // End program without leaking memory
    delete[] screenPixels;
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
#endif
}
