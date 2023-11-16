
#include "libfixmath/fix16.hpp"

#include "RenderFP3D.hpp"

#include "constants.hpp"

#include "Model.hpp"

#include "StringUtils.hpp"

#include "Utils.hpp"

#include "Renderer.hpp"

#include "DynamicArray.hpp"

#ifndef PC
#   include "app_description.hpp"
#   include <sdk/calc/calc.hpp>
#   include <sdk/os/input.hpp>
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

// Keymappings, both ClassPad and SDL2
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
#   define KEY_QUIT            testKey(k1,k2,KEY_CLEAR)
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
#   define KEY_QUIT            key_ESCAPE
#endif

#include "DynamicArray.hpp"  // Include the source file

bool DEBUG_TEST()
{
#ifdef PC
    //std::cout << "DEBUG_TEST is stopping everything!!!!" << std::endl;
#endif
    return false;
}

#ifndef PC
// fps10 from "fps_functions.hpp" in calculator case
extern int fps10;
#endif

#ifdef PC
    extern uint32_t * screenPixels;
#endif

#define MOVEMENT_SPEED   15.0f
#define CAMERA_SPEED      1.15f
#define FOV_UPDATE_SPEED 50.0f

#define FILL_SCREEN_COLOR color(190,190,190)

#ifndef PC
int custom_init()
{
    calcInit(); //backup screen and init some variables
    return 0;
}
#else
int custom_init(SDL_Window **window, SDL_Renderer **sdl_renderer, SDL_Texture ** texture)
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
        return -1;

    *window = SDL_CreateWindow("Classpad II PC demo",
                                SDL_WINDOWPOS_UNDEFINED,
                                SDL_WINDOWPOS_UNDEFINED,
                                (int) ((float) SCREEN_X * (float) WINDOW_SIZE_MULTIPLIER),
                                (int) ((float) SCREEN_Y * (float) WINDOW_SIZE_MULTIPLIER),
                                SDL_WINDOW_OPENGL);
    if (*window == nullptr) {
        SDL_Log("Could not create a window: %s", SDL_GetError());
        return -1;
    }

    *sdl_renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    if (sdl_renderer == nullptr) {
        SDL_Log("Could not create a sdl_renderer: %s", SDL_GetError());
        return -1;
    }

    *texture = SDL_CreateTexture(*sdl_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, SCREEN_X, SCREEN_Y);

    return 0;
}
#endif

#ifndef PC
extern "C" void main()
{
    int init_status = custom_init();
    if (init_status != 0) return;
#else // ifdef PC
int main(int argc, const char * argv[])
{
    SDL_Window *window;
    SDL_Renderer *sdl_renderer;
    SDL_Texture * texture;
    screenPixels = new Uint32[SCREEN_X * SCREEN_Y];

    int init_status = custom_init(&window, &sdl_renderer, &texture);
    if (init_status != 0) return init_status;

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
    bool key_ESCAPE = false;
#endif // PC
    bool KEY_RENDER_MODE_prev = false; // De-bouncing the button

    char model1_path[] =
#ifdef PC
        "./3D_Converted_Models/little_endian_pika.pkObj";
#else
        "\\fls0\\big_endian_pika.pkObj";
#endif

    char model1_texture_path[] =
#ifdef PC
        "./3D_Converted_Models/little_endian_pika.texture";
#else
        "\\fls0\\big_endian_pika.texture";
#endif

    char model2_path[] =
#ifdef PC
        "./3D_Converted_Models/little_endian_cube.pkObj";
#else
        "\\fls0\\big_endian_cube.pkObj";
#endif

    fillScreen(FILL_SCREEN_COLOR);
#ifndef PC
    // Let user know that program has not crashed and we are loading model
    Debug_SetCursorPosition(1,1);
    Debug_PrintString("Load obj", false);
    LCD_Refresh();
#endif

    // Create renderer
    Renderer renderer;

    // Add model to renderer and modify its initial rotation
    auto model = renderer.addModel(model1_path, model1_texture_path);
    model->getRotation_ref().y = Fix16(3.145f/2.0f);

    uint16_t rend_mod = 0;

    Fix16 place_in_circle = 0.0f;
    const int16_t place_count = 4;
    const Fix16 radius = 13.0f;
    Model* autoplaced_models[place_count];
    for(int16_t i=0; i<place_count; i++){
        place_in_circle = ((Fix16(fix16_pi)) * 2.0f * Fix16(i) / place_count);

        auto m = renderer.addModel(model2_path, NO_TEXTURE);
        autoplaced_models[i] = m;

        // Position
        m->getPosition_ref().x = place_in_circle.sin() * radius;
        m->getPosition_ref().y = +5.0f;
        m->getPosition_ref().z = place_in_circle.cos() * radius;

        // Rotation
        m->getRotation_ref().y = Fix16(3.145f/2.0f);
        m->render_mode = (rend_mod++)%RENDER_MODE_COUNT;

        // Scale model such that the max width
        // between furthest vertices is 7.0f
        m->_scaleModelTo(7.0f);
    }
#ifdef PC
    uint32_t time_t0 = SDL_GetTicks();
    int accumulative_frames  = 0;
    uint32_t last_fps = 0;
#endif

    Fix16 lightRotation = 0.0f;

    // Delta-time
    Fix16 last_dt = Fix16((int16_t) 0.0016f);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~ Main Loop ~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    bool done = false;

    done = DEBUG_TEST();

    while(!done)
    {

#ifndef PC
        // FPS & delta-time count timer
        fps_update();
#endif

        lightRotation += last_dt * 1.2f;

        renderer.get_lightPos().x = lightRotation.sin() * -8.0f;
        renderer.get_lightPos().y = -10.0f;
        renderer.get_lightPos().z = lightRotation.cos() * -8.0f;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~ Key Presses ~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#ifndef PC
        // ClassPad keypresses are stored in k1 and k2 in bits
        uint32_t k1,k2; getKey(&k1,&k2);
#else // !PC -> PC
        // Get the next event
        SDL_Event event;
        if (SDL_PollEvent(&event))
        {
            switch( event.type ){
                /* Keyboard event */
                /* Pass the event data onto PrintKeyInfo() */
                case SDL_KEYDOWN:
                    switch( event.key.keysym.sym ){
                        case SDLK_LEFT:   key_left  = true; break;
                        case SDLK_RIGHT:  key_right = true; break;
                        case SDLK_UP:     key_up    = true; break;
                        case SDLK_DOWN:   key_down  = true; break;
                        case SDLK_r:      key_r     = true; break;
                        case SDLK_f:      key_f     = true; break;
                        case SDLK_a:      key_a     = true; break;
                        case SDLK_d:      key_d     = true; break;
                        case SDLK_w:      key_w     = true; break;
                        case SDLK_s:      key_s     = true; break;
                        case SDLK_1:      key_1     = true; break;
                        case SDLK_2:      key_2     = true; break;
                        case SDLK_e:      key_e     = true; break;
                        case SDLK_ESCAPE: key_ESCAPE = true; break;
                        default:                            break;
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
                        case SDLK_ESCAPE: key_ESCAPE = false; break;
                        default:                            break;
                    }
                    break;
                case SDL_QUIT: // Closing window i.e. pressing window "X" button
                    done = true;
                    break;
                default:
                    break;
            }
        }
#endif // PC

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~ Key Presses ~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#ifndef PC
    // Only poll Calculator when any key was pressed
    if (Input_IsAnyKeyDown())
    {
#endif
        if (KEY_QUIT)
            done = true;

        if(KEY_MOVE_LEFT)  {
            renderer.get_camera_pos().z += renderer.get_camera_rot().x.sin() * last_dt * MOVEMENT_SPEED;
            renderer.get_camera_pos().x -= renderer.get_camera_rot().x.cos() * last_dt * MOVEMENT_SPEED;
            renderer.camera_move_dirty = true;
        }
        if(KEY_MOVE_RIGHT) {
            renderer.get_camera_pos().z -= renderer.get_camera_rot().x.sin() * last_dt * MOVEMENT_SPEED;
            renderer.get_camera_pos().x += renderer.get_camera_rot().x.cos() * last_dt * MOVEMENT_SPEED;
            renderer.camera_move_dirty = true;
        }
        if(KEY_MOVE_FORWARD)    {
            renderer.get_camera_pos().x += renderer.get_camera_rot().x.sin() * last_dt * MOVEMENT_SPEED;
            renderer.get_camera_pos().z += renderer.get_camera_rot().x.cos() * last_dt * MOVEMENT_SPEED;
            renderer.camera_move_dirty = true;
        }
        if(KEY_MOVE_BACKWARD)  {
            renderer.get_camera_pos().x -= renderer.get_camera_rot().x.sin() * last_dt * MOVEMENT_SPEED;
            renderer.get_camera_pos().z -= renderer.get_camera_rot().x.cos() * last_dt * MOVEMENT_SPEED;
            renderer.camera_move_dirty = true;
        }
        if(KEY_MOVE_UP) {
            renderer.get_camera_pos().y -= last_dt * MOVEMENT_SPEED;
            renderer.camera_move_dirty = true;
        }
        if(KEY_MOVE_DOWN){
            renderer.get_camera_pos().y += last_dt * MOVEMENT_SPEED;
            renderer.camera_move_dirty = true;
        }

        if(KEY_MOVE_FOV_ADD)
            renderer.get_FOV() += last_dt * FOV_UPDATE_SPEED;
        if(KEY_MOVE_FOV_SUB)
            renderer.get_FOV() -= last_dt * FOV_UPDATE_SPEED;

        if(KEY_MOVE_REND_MODE){
            if(KEY_RENDER_MODE_prev == false){
                if(model->render_mode > 0)
                    model->render_mode = model->render_mode - 1;
                else
                    model->render_mode = RENDER_MODE_COUNT - 1;
            }
            KEY_RENDER_MODE_prev = true;
        }
        else{
            KEY_RENDER_MODE_prev = false;
        }

        if(KEY_ROTATE_LEFT)
            renderer.get_camera_rot().x -= last_dt * CAMERA_SPEED;
        if(KEY_ROTATE_RIGHT)
            renderer.get_camera_rot().x += last_dt * CAMERA_SPEED;
        if(KEY_ROTATE_UP)
            renderer.get_camera_rot().y -= last_dt * CAMERA_SPEED;
        if(KEY_ROTATE_DOWN)
            renderer.get_camera_rot().y += last_dt * CAMERA_SPEED;

#ifndef PC
    } // Input_IsAnyKeyDown()
    else {
        KEY_RENDER_MODE_prev = false;
    }
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~ Main Loop ~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

        // Only doing simple rotation in main loop
        model->getRotation_ref().x += last_dt * 0.5f;


        auto roty = autoplaced_models[0]->getRotation_ref().y + last_dt * 1.0f;
        auto rotx = autoplaced_models[0]->getRotation_ref().x + last_dt * 1.0f;
        for (int i=0; i<place_count; i++){
            autoplaced_models[i]->getRotation_ref().y = roty;
            autoplaced_models[i]->getRotation_ref().x = rotx;
        }

        // Return clear BoundingBox (Box created by these 2 points only must be cleared)
        // This should drastically improve speed if models take only small part of screen.
        int16_t_vec2 bbox_max = {0, 0};
        int16_t_vec2 bbox_min = {SCREEN_X, SCREEN_Y};
        // Draws objects to ram. Later refersh lcd.
        renderer.update(&bbox_max, &bbox_min);

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
        // SDL_GetTicks() seems not to be super accurate, so adding frames to
        // accumulative_frames and updating the frame counter with some period
        const Uint32 FPS_UPDATE_FREQ_MS = 300;
        Uint32 time_t1 = SDL_GetTicks();
        accumulative_frames++;
        if (time_t1 - time_t0 >= FPS_UPDATE_FREQ_MS) {
            float fps = accumulative_frames / ((time_t1 - time_t0) / 1000.0f);
            last_fps = (uint32_t) fps;
            time_t0 = time_t1;
            accumulative_frames = 0;
            // Also update dt
            last_dt = Fix16(1.0f) / (Fix16(((int16_t) last_fps)));
        }
        // Printing "text" (only numbers for possible for now) from
        // manually created bitmaps
        sdl_debug_uint32_t(last_fps, 10, 10);

#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~ Refresh screen ~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#ifndef PC
        LCD_Refresh();
#else
        SDL_UpdateTexture(texture, NULL, screenPixels, SCREEN_X * sizeof(Uint32));
        SDL_RenderClear(sdl_renderer);
        SDL_RenderCopy(sdl_renderer, texture, NULL, NULL);
        SDL_RenderPresent(sdl_renderer);
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~  Clear VRAM for new frame ~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// #ifdef PC
//         std::cout << "bbox_min.x: " << bbox_min.x << " | bbox_min.y: " << bbox_min.y << std::endl;
//         std::cout << "bbox_max.x: " << bbox_max.x << " | bbox_max.y: " << bbox_max.y << std::endl;
// #endif
        // Clear models
        for(int x=bbox_min.x; x<bbox_max.x; x++){
            for(int y=bbox_min.y; y<bbox_max.y; y++){
                // UNSAFE Pixel setting (not checking if the pixel is inside)
#ifdef PC
                setPixel_Unsafe(x,y, FILL_SCREEN_COLOR);
#else
                vram[width*y + x] = FILL_SCREEN_COLOR;
#endif
            }
        }

        // Clear rotation visualizer
        for(int x=SCREEN_X-ROTATION_VISUALIZER_LINE_WIDTH*2-ROTATION_VISALIZER_EDGE_OFFSET; x<SCREEN_X; x++){
            for(int y=0; y<=ROTATION_VISUALIZER_LINE_WIDTH*2+ROTATION_VISALIZER_EDGE_OFFSET; y++){
#ifdef PC
                setPixel_Unsafe(x,y, FILL_SCREEN_COLOR);
#else
                vram[width*y + x] = FILL_SCREEN_COLOR;
#endif
            }
        }

        // Clear FPS Text (only for pc as pc draws on screen differently)
#ifdef PC
        for(int x=10; x < 50; x++){
            for(int y=10; y < 6*4; y++){
                setPixel_Unsafe(x,y, FILL_SCREEN_COLOR);
            }
        }
#endif
        // Clear light box
        renderer.clear_LightLocation(FILL_SCREEN_COLOR);

        // fillScreen(FILL_SCREEN_COLOR);

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
    SDL_DestroyRenderer(sdl_renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
#endif
}
