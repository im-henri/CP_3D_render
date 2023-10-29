
#include "libfixmath/fix16.hpp"

#include "RenderFP3D.hpp"

#define SCREEN_X 320
#define SCREEN_Y 528

#ifndef PC
    #include <appdef.hpp>
    /*
    * Fill this section in with some information about your app.
    * All fields are optional - so if you don't need one, take it out.
    */
    APP_NAME("aa")
    APP_DESCRIPTION("bb")
    APP_AUTHOR("cc")
    APP_VERSION("1.0.0")

    #include <sdk/calc/calc.hpp>
    #include <sdk/os/lcd.hpp>
    #include <sdk/os/debug.hpp>

#else
#   include <SDL2/SDL.h>
#   include <iostream>
#endif

// Defined in breakpoint_handler_stub.s
extern "C" void ASM_NOP();

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

#endif

    fillScreen(color(255,255,255));

    bool done = false;
    fix16_vec3 cube_vertices[8] = {
        // Lower 4 vertices
        {-1.0f, -1.0f, -1.0f}, {1.0f, -1.0f, -1.0f},
        {-1.0f,  1.0f, -1.0f}, {1.0f,  1.0f, -1.0f},

        // Higher 4 vertices
        {-1.0f, -1.0f,  1.0f}, {1.0f, -1.0f,  1.0f},
        {-1.0f,  1.0f,  1.0f}, {1.0f,  1.0f,  1.0f}
    };
    #define CUBE_LINE_ARR_SIZE 24
    uint16_t cube_lines[CUBE_LINE_ARR_SIZE] = {
        0,1,
        0,2,
        1,3,
        2,3,

        0+4,1+4,
        0+4,2+4,
        1+4,3+4,
        2+4,3+4,

        0,4,
        1,5,
        2,6,
        3,7
    };

    fix16_vec3 camera_pos = {-2.0f, -1.6f, -3.0f};
    fix16_vec2 camera_rot = {0.7f, 0.6f};

    Fix16 FOV = 200.0f;
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
                        case SDLK_LEFT:
                            //camera_pos.x -= 0.1f;
                            camera_pos.z += camera_rot.x.sin()*0.1f;
                            camera_pos.x -= camera_rot.x.cos()*0.1f;
                            break;
                        case SDLK_RIGHT:
                            //camera_pos.x += 0.1f;
                            camera_pos.z -= camera_rot.x.sin()*0.1f;
                            camera_pos.x += camera_rot.x.cos()*0.1f;
                            break;
                        case SDLK_UP:
                            //camera_pos.z += 0.1f;
                            camera_pos.x += camera_rot.x.sin()*0.1f;
                            camera_pos.z += camera_rot.x.cos()*0.1f;
                            break;
                        case SDLK_DOWN:
                            //camera_pos.z -= 0.1f;
                            camera_pos.x -= camera_rot.x.sin()*0.1f;
                            camera_pos.z -= camera_rot.x.cos()*0.1f;
                            break;
                        case SDLK_r:
                            camera_pos.y -= 0.1f;
                            break;
                        case SDLK_f:
                            camera_pos.y += 0.1f;
                            break;

                        case SDLK_a:
                            camera_rot.x -= 0.1f;
                            break;
                        case SDLK_d:
                            camera_rot.x += 0.1f;
                            break;
                        case SDLK_w:
                            camera_rot.y -= 0.1f;
                            break;
                        case SDLK_s:
                            camera_rot.y += 0.1f;
                            break;

                        case SDLK_1:
                            FOV += 1.0f;
                            break;
                        case SDLK_2:
                            FOV -= 1.0f;
                            break;
                        default:
                            break;
                    }
                case SDL_KEYUP:
                    //PrintKeyInfo( &event.key );
                    break;

                /* SDL_QUIT event (window close) */
                case SDL_QUIT:
                    done = true;
                    break;

                default:
                    break;
            }
        }
#endif

// --------------------------------------------------
        // Creating screen coordinates of the cube and drawing red boxes on vertices
        fix16_vec2 screen_cube[8];
        for (int i=0; i<8; i++){
            auto screen_vec2 = getScreenCoordinate(FOV, cube_vertices[i], camera_pos, camera_rot);
            int16_t x = (int16_t)screen_vec2.x;
            int16_t y = (int16_t)screen_vec2.y;
            screen_cube[i] = {x, y};
            draw_center_square(x, y, 4,4, color(255,0,0));
            //setPixel(x, y, color(0,0,0));
        }
        // Drawng edges of the cube
        int i=0;
        while(i<CUBE_LINE_ARR_SIZE)
        {

            line((int16_t)(screen_cube[cube_lines[i+0]].x), (int16_t)(screen_cube[cube_lines[i+0]].y),
                 (int16_t)(screen_cube[cube_lines[i+1]].x), (int16_t)(screen_cube[cube_lines[i+1]].y),
                color(0,0,0)
            );
            i+=2;
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








/*
// Demo of using libfixmath on Classpads SH4
// https://github.com/PetteriAimonen/libfixmath/tree/master/libfixmath
// Note that not all of the source is included. If you need other functionaly
// then put the libfixmath source under "src/Fixmath"
#include "libfixmath/fix16.hpp"
//
#include "app_description.hpp"
#include <sdk/calc/calc.hpp>
#include <sdk/os/lcd.hpp>
#include <sdk/os/debug.hpp>

#define SCREEN_X 320
#define SCREEN_Y 528

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

extern "C"
void main()
{
    calcInit(); //backup screen and init some variables

    // Constants such as PI or e are available
    Fix16 angle = -fix16_pi;
    Fix16 hue   = 0.0f;
    while(true){
        // ------ Clear screen ------
        fillScreen(color(255, 255, 255));

        // ----- Check for exit -----
        uint32_t k1,k2;
        getKey(&k1,&k2);
        if(testKey(k1,k2,KEY_CLEAR))
            break;

        // ------ Fix16 example ------
        // Supports math with float
        angle += 0.10f;
        hue   += 0.02f;
        // Trigonometric functions
        auto sin_angle = angle.sin();
        auto cos_angle = angle.cos();
        // Fix16 casted to int16_t
        int16_t x = (sin_angle * 40.0f);
        int16_t y = (cos_angle * 40.0f);
        // Coloring r
        int16_t hue_r = (int16_t) ((hue.sin() + 1.0f)/2.0f * 255.0f);
        int16_t hue_b = (int16_t) ((hue.cos() + 1.0f)/2.0f * 255.0f);
        uint8_t col_r = hue_r;
        uint8_t col_g = 0;
        uint8_t col_b = hue_b;
        draw_center_square(SCREEN_X/2 + x, SCREEN_Y/2 + y, 30, 30, color(col_r,col_g,col_b));
        // Debug printing
        char buff[14];
        fix16_to_str(sin_angle, buff, 3);
        Debug_Printf(2, 2, false, 0, "sin(angle)=%s", buff);
        fix16_to_str(cos_angle, buff, 3);
        Debug_Printf(2, 3, false, 0, "cos(angle)=%s", buff);

        // ----- Refresh screen -----
        LCD_Refresh();
    }

    calcEnd(); //restore screen and do stuff
}
*/