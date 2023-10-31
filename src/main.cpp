
#include "libfixmath/fix16.hpp"

#include "RenderFP3D.hpp"

#include "constants.hpp"

#include "Model.hpp"

#include "StringUtils.hpp"

#ifndef PC
    #include "app_description.hpp"
    #include <sdk/calc/calc.hpp>
    #include <sdk/os/lcd.hpp>
    #include <sdk/os/debug.hpp>
    #include <sdk/os/mem.hpp>
    #include <sdk/os/file.hpp>
#else
#   include <SDL2/SDL.h>
#   include <iostream>
#   include <unistd.h>  // File open & close
#   include <fcntl.h>   // File open & close
#endif

#ifdef PC
Uint32 * screenPixels;
void setPixel(int x, int y, uint16_t color)
{
    screenPixels[y * SCREEN_X + x] = color;
}
//void LCD_ClearScreen()
void LCD_ClearScreen()
{
    memset(screenPixels, 255, SCREEN_X * SCREEN_Y * sizeof(Uint32));
}
inline uint16_t color(uint8_t R, uint8_t G, uint8_t B){
    return    (((R<<8) & 0b1111100000000000) |
         ((G<<3) & 0b0000011111100000) |
         ((B>>3) & 0b0000000000011111));
}
void fillScreen(uint16_t color)
{
    memset(screenPixels, (Uint32)color, SCREEN_X * SCREEN_Y * sizeof(Uint32));
}
//Draw a line (bresanham line algorithm)
void line(int x1, int y1, int x2, int y2, uint16_t color){
	int8_t ix, iy;

	int dx = (x2>x1 ? (ix=1, x2-x1) : (ix=-1, x1-x2) );
	int dy = (y2>y1 ? (iy=1, y2-y1) : (iy=-1, y1-y2) );

	setPixel(x1,y1,color);
	if(dx>=dy){ //the derivative is less than 1 (not so steep)
		//y1 is the whole number of the y value
		//error is the fractional part (times dx to make it a whole number)
		// y = y1 + (error/dx)
		//if error/dx is greater than 0.5 (error is greater than dx/2) we add 1 to y1 and subtract dx from error (so error/dx is now around -0.5)
		int error = 0;
		while (x1!=x2) {
			x1 += ix; //go one step in x direction
			error += dy;//add dy/dx to the y value.
			if (error>=(dx>>1)){ //If error is greater than dx/2 (error/dx is >=0.5)
				y1+=iy;
				error-=dx;
			}
			setPixel(x1,y1,color);
		}
	}else{ //the derivative is greater than 1 (very steep)
		int error = 0;
		while (y1!=y2) { //The same thing, just go up y and look at x
			y1 += iy; //go one step in y direction
			error += dx;//add dx/dy to the x value.
			if (error>=(dy>>1)){ //If error is greater than dx/2 (error/dx is >=0.5)
				x1+=ix;
				error-=dy;
			}
			setPixel(x1,y1,color);
		}
	}
}
#endif

void draw_center_square(int16_t cx, int16_t cy, int16_t sx, int16_t sy, uint16_t color)
{
    for(int16_t i=-sx/2; i<sx/2; i++)
    {
        for(int16_t j=-sy/2; j<sy/2; j++)
        {
            setPixel(cx+i, cy+j, color);
        }
    }
}


#ifndef PC
extern "C" void main()
{
    calcInit(); //backup screen and init some variables
#else
int main(int argc, const char * argv[])
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        return -1;
    }

    // Create a window
    SDL_Window *window = SDL_CreateWindow("Classpad II PC demo",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SCREEN_X,
                                          SCREEN_Y,
                                          SDL_WINDOW_OPENGL);
    if (window == nullptr)
    {
        SDL_Log("Could not create a window: %s", SDL_GetError());
        return -1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr)
    {
        SDL_Log("Could not create a renderer: %s", SDL_GetError());
        return -1;
    }

    SDL_Texture * texture = SDL_CreateTexture(renderer,
    SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, SCREEN_X, SCREEN_Y);
    screenPixels = new Uint32[SCREEN_X * SCREEN_Y];

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

#endif

    fix16_vec3 cube_vertices[] = {
        // Lower 4 vertices
        {-1.0f, -1.0f, -1.0f}, {1.0f, -1.0f, -1.0f},
        {-1.0f,  1.0f, -1.0f}, {1.0f,  1.0f, -1.0f},

        // Higher 4 vertices
        {-1.0f, -1.0f,  1.0f}, {1.0f, -1.0f,  1.0f},
        {-1.0f,  1.0f,  1.0f}, {1.0f,  1.0f,  1.0f}
    };
    u_pair cube_edges[] = {
        {0,1}, {0,2}, {1,3}, {2,3},
        {0+4,1+4}, {0+4,2+4}, {1+4,3+4}, {2+4,3+4},
        {0,4}, {1,5}, {2,6}, {3,7}
    };
    Model cube1(
        &(cube_vertices[0]),(sizeof(cube_vertices) / sizeof(cube_vertices[0])),
        &(cube_edges[0]),   (sizeof(cube_edges) / sizeof(cube_edges[0]))
    );
    cube1.getPosition_ref().y -= 6.0f;


#ifdef PC
    //char model_path[] = "./hi.obj";
    //char model_path[] = "./test.obj";
    char model_path[] = "./suzanne.obj";
#else
    char model_path[] = "\\fls0\\suzanne.obj";
    //char model_path[] = "\\fls0\\hi.obj";
#endif

    fillScreen(color(255,255,255));
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
    Model model_test = Model(model_path);


    // Example: reading 256 bytes from a file called @c test.txt from the USB flash
    //int fd = open("\\\\fls0\\test.txt", OPEN_READ);
    //char buf[256] = "6.0\0";
    //int ret = read(fd, buf, sizeof(buf));
    //ret = close(fd);


    // ----
    fix16_vec3 test_vertices[] = {

        {0.0f,   0.0f,   0.0f}, // Padding since numbering starts from 1 (LATER REMOVE THIS!)

        {-2.160963f,   0.631728f,   -0.317909f},
        {-2.160963f,   4.239110f,   -0.317909f},
        {-2.160963f,   0.631728f,   0.317909f},
        {-2.160963f,   4.239110f,   0.317909f},
        {-0.022835f,   0.631728f,   -0.317909f},
        {-0.022835f,   0.631728f,   0.317909f},
        {-0.022835f,   4.239110f,   -0.317909f},
        {-0.022835f,   4.239110f,   0.317909f},
        {1.789928f,    4.239110f,   -0.317909f},
        {1.789928f,    4.239110f,   0.317909f},
        {1.789928f,    -4.239110f,  -0.317909f},
        {1.789928f,    -4.239110f,  0.317909f},
        {-0.022835f,   -4.239110f,  -0.317909f},
        {-0.022835f,   -4.239110f,  0.317909f},
        {-0.022835f,   -0.904996f,  -0.317909f},
        {-0.022835f,   -0.904996f,  0.317909f},
        {-2.160963f,   -0.904996f,  -0.317909f},
        {-2.160963f,   -0.904996f,  0.317909f},
        {-2.160963f,   -4.239110f,  -0.317909f},
        {-2.160963f,   -4.239110f,  0.317909f},
        {-3.973727f,   -4.239110f,  -0.317909f},
        {-3.973727f,   -4.239110f,  0.317909f},
        {-3.973727f,   4.239110f,   -0.317909f},
        {-3.973727f,   4.239110f,   0.317909f},
        {3.973727f, -4.239110f, -0.317909f},
        {3.973727f, 0.737936f, -0.317909f},
        {3.973727f, -4.239110f, 0.317909f},
        {3.973727f, 0.737936f, 0.317909f},
        {2.562321f, -4.239110f, -0.317909f},
        {2.562321f, -4.239110f, 0.317909f},
        {2.562321f, 0.737936f, -0.317909f},
        {2.562321f, 0.737936f, 0.317909f},
        {2.562321f, 2.352888f, -0.317909f},
        {2.562321f, 1.204561f, -0.317909f},
        {2.562321f, 2.352888f, 0.317909f},
        {2.562321f, 1.204561f, 0.317909f},
        {3.973727f, 2.352888f, -0.317909f},
        {3.973727f, 2.352888f, 0.317909f},
        {3.973727f, 1.204561f, -0.317909f},
        {3.973727f, 1.204561f, 0.317909f},
    };
    u_pair test_edges[] = {
{1, 2},{6, 1},{3, 16},{6, 16},
{6, 12},{10, 8},{8, 5},{7, 5},
{7, 9},{5, 9},{15, 5},{15, 11},
{15, 13},{18, 15},{17, 15},{20, 17},
{18, 22},{22, 19},{21, 19},{24, 21},
{3, 24},{4, 24},{4, 23},{2, 23},
{23, 21},{23, 1},{21, 1},{1, 5},
{21, 17},{3, 22},{19, 17},{16, 13},
{13, 11},{14, 11},{17, 5},{11, 9},
{9, 7},{10, 7},{12, 9},{16, 14},
{18, 16},{5, 1},{3, 2},{2, 1},
{2, 3},{1, 3},{16, 6},{16, 12},
{12, 10},{8, 6},{5, 6},{5, 8},
{9, 5},{9, 11},{5, 11},{11, 13},
{13, 16},{15, 16},{15, 18},{17, 18},
{22, 20},{19, 20},{19, 22},{21, 22},
{24, 22},{24, 3},{23, 24},{23, 4},
{21, 24},{1, 21},{1, 17},{5, 17},{17, 19},{22, 18},{17, 20},
{13, 14},{11, 14},{11, 12},{5, 15},{9, 12},{7, 10},{7, 8},
{9, 10},{14, 12},{16, 3},{1, 6},{2, 4},{1, 23}, {1, 3},
{6, 3},{3, 6},{6, 12},{6, 10},{10, 6},{8, 6},{7, 8},
{7, 5},{5, 11},{15, 11},{15, 13},{15, 16},{18, 16},{17, 18},
{20, 18},{18, 20},{22, 20},{21, 22},{24, 22},{3, 22},{4, 3},
{4, 24},{2, 4},{23, 24},{23, 21},{21, 17},{1, 17},{21, 19},
{3, 18},{19, 20},{16, 14},{13, 14},{14, 12},{17, 15},{11, 12},
{9, 10},{10, 8},{12, 10},{16, 12},{18, 3},{5, 6},{3, 4},{2, 23},
{25, 26},{30, 25},{27, 32},{32, 29},{31, 29},{28, 31},{26, 31},
{31, 26},{28, 32},{29, 25},{27, 26},{29, 26},{33, 34},{38, 33},
{35, 40},{40, 37},{39, 37},{36, 39},{34, 39},{39, 34},{36, 40},
{37, 33},{35, 34},{37, 34},{25, 27},{30, 27},{27, 30},{32, 30},
{31, 32},{28, 32},{26, 28},{31, 29},{28, 27},{29, 30},{27, 28},
{29, 25},{33, 35},{38, 35},{35, 38},{40, 38},{39, 40},{36, 40},
{34, 36},{39, 37},{36, 35},{37, 38},{35, 36},{37, 33},{26, 27},
{25, 27},{32, 30},{29, 30},{29, 32},{31, 32},{31, 28},{26, 29},
{32, 27},{25, 30},{26, 28},{26, 25},{34, 35},{33, 35},{40, 38},
{37, 38},{37, 40},{39, 40},{39, 36},{34, 37},{40, 35},{33, 38},
{34, 36},{34, 33}
    };
    Model testmodel(
        &(test_vertices[0]),(sizeof(test_vertices) / sizeof(test_vertices[0])),
        &(test_edges[0]),   (sizeof(test_edges) / sizeof(test_edges[0]))
    );

    Model* all_models[] = {
        // &cube1, &testmodel, &model_test
        &model_test
    };
    const unsigned all_model_count = sizeof(all_models) / sizeof(all_models[0]);

    fix16_vec3 camera_pos = {-9.0f, -1.6f, -10.0f};
    fix16_vec2 camera_rot = {0.8f, 0.2f};

    Fix16 FOV = 300.0f; // Does not mean 300 degrees, some "arbitrary" meaning

    Fix16 fix_val1 = 0.0f;

    bool done = false;
    while(!done)
    {
        fillScreen(color(255,255,255));
        // --------------------------


        // ----- Check for exit -----
#ifndef PC
        uint32_t k1,k2; getKey(&k1,&k2);
        if(testKey(k1,k2,KEY_CLEAR)) {
            done = true;
        }

        if(testKey(k1,k2,KEY_4))  {
            //camera_pos.x -= 0.1f;
            camera_pos.z += camera_rot.x.sin()*0.1f;
            camera_pos.x -= camera_rot.x.cos()*0.1f;
        }
        if(testKey(k1,k2,KEY_6)) {
            //camera_pos.x += 0.1f;
            camera_pos.z -= camera_rot.x.sin()*0.1f;
            camera_pos.x += camera_rot.x.cos()*0.1f;
        }
        if(testKey(k1,k2,KEY_8))    {
            //camera_pos.z += 0.1f;
            camera_pos.x += camera_rot.x.sin()*0.1f;
            camera_pos.z += camera_rot.x.cos()*0.1f;
        }
        if(testKey(k1,k2,KEY_2))  {
            //camera_pos.z -= 0.1f;
            camera_pos.x -= camera_rot.x.sin()*0.1f;
            camera_pos.z -= camera_rot.x.cos()*0.1f;
        }
        if(testKey(k1,k2,KEY_9))     {
            camera_pos.y -= 0.1f;
        }
        if(testKey(k1,k2,KEY_3))     {
            camera_pos.y += 0.1f;
        }

        if(testKey(k1,k2,KEY_ADD))  {
            FOV += 1.0f;
        }
        if(testKey(k1,k2,KEY_SUBTRACT)){
            FOV -= 1.0f;
        }

        if(testKey(k1,k2,KEY_LEFT))  camera_rot.x -= 0.1f;
        if(testKey(k1,k2,KEY_RIGHT)) camera_rot.x += 0.1f;
        if(testKey(k1,k2,KEY_UP))    camera_rot.y -= 0.1f;
        if(testKey(k1,k2,KEY_DOWN))  camera_rot.y += 0.1f;


#else
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

        if (key_left){
            camera_pos.z += camera_rot.x.sin()*0.006f;
            camera_pos.x -= camera_rot.x.cos()*0.006f;
        }
        if (key_right){
            camera_pos.z -= camera_rot.x.sin()*0.006f;
            camera_pos.x += camera_rot.x.cos()*0.006f;
        }
        if (key_up){
            camera_pos.x += camera_rot.x.sin()*0.006f;
            camera_pos.z += camera_rot.x.cos()*0.006f;
        }
        if (key_down){
            camera_pos.x -= camera_rot.x.sin()*0.006f;
            camera_pos.z -= camera_rot.x.cos()*0.006f;
        }
        if (key_r){
            camera_pos.y -= 0.006f;
        }
        if (key_f){
            camera_pos.y += 0.006f;
        }

        if (key_a){
            camera_rot.x -= 0.002f;
        }
        if (key_d){
            camera_rot.x += 0.002f;
        }
        if (key_w){
            camera_rot.y -= 0.002f;
        }
        if (key_s){
            camera_rot.y += 0.002f;
        }

        if (key_1){
            FOV += 0.1f;
        }
        if (key_2){
            FOV -= 0.1f;
        }
#endif

// --------------------------------------------------
        all_models[0]->getRotation_ref().x += 0.004f;
        all_models[0]->getRotation_ref().y += 0.002f;
        //all_models[0]->getScale_ref().x = fix_val1.sin() * 0.1f + 1.0f;

        //testmodel.getRotation_ref().y += 0.03f;

        fix_val1 += 0.03f;

        for (unsigned m_id=0; m_id<all_model_count; m_id++)
        {
            // For each model..
            // Get screen coordinates (And visualize vertices)
            fix16_vec2* screen_coords = (fix16_vec2*) malloc(sizeof(fix16_vec2) * all_models[m_id]->vertex_count);
            for (unsigned v_id=0; v_id<all_models[m_id]->vertex_count; v_id++){
                auto screen_vec2 = getScreenCoordinate(
                    FOV, all_models[m_id]->vertices[v_id],
                    all_models[m_id]->getPosition_ref(), all_models[m_id]->getRotation_ref(),
                    all_models[m_id]->getScale_ref(),
                    camera_pos, camera_rot
                );
                int16_t x = (int16_t)screen_vec2.x;
                int16_t y = (int16_t)screen_vec2.y;
                screen_coords[v_id] = {x, y};
                //draw_center_square(x, y, 4,4, color(255,0,0));
                //setPixel(x, y, color(0,0,0));
            }
            // Draw edges of the cube
            for(unsigned e_id=0; e_id<all_models[m_id]->edge_count; e_id++){
                line((int16_t)(screen_coords[all_models[m_id]->edges[e_id].First].x),
                     (int16_t)(screen_coords[all_models[m_id]->edges[e_id].First].y),
                     (int16_t)(screen_coords[all_models[m_id]->edges[e_id].Second].x),
                     (int16_t)(screen_coords[all_models[m_id]->edges[e_id].Second].y),
                    color(0,0,0)
                );
            }
            free(screen_coords);
        }

// --------------------------------------------------

        // ----- Refresh screen -----
#ifndef PC
        LCD_Refresh();
#else
        SDL_UpdateTexture(texture, NULL, screenPixels, SCREEN_X * sizeof(Uint32));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
#endif
    }

#ifndef PC
    calcEnd(); //restore screen and do stuff
#else
    // Tidy up
    delete[] screenPixels;
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
#endif
}
