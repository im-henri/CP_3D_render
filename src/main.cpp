
#include "libfixmath/fix16.hpp"

#include "RenderFP3D.hpp"

#include "constants.hpp"

#include "Model.hpp"

#include "StringUtils.hpp"

//#include "gen_uv_tex.hpp"

#ifndef PC
    #include "app_description.hpp"
    #include <sdk/calc/calc.hpp>
    #include <sdk/os/lcd.hpp>
    #include <sdk/os/debug.hpp>
    #include <sdk/os/mem.hpp>
    #include <sdk/os/file.hpp>
    #include "fps_functions.hpp"
#else
#   include <SDL2/SDL.h>
#   include <iostream>
#   include <unistd.h>  // File open & close
#   include <fcntl.h>   // File open & close
#endif

#define FILL_SCREEN_COLOR color(190,190,190)

//
// ----------------------
//

// Since endianess differ on PC and CP using ifdef
// to define PC that turns manually bytes into value.
inline uint32_t ByteArrToUint32_t(char* bytes)
{
#ifdef PC
    return   ((uint32_t) ((unsigned char*) bytes)[3]) << 0*8 |
             ((uint32_t) ((unsigned char*) bytes)[2]) << 1*8 |
             ((uint32_t) ((unsigned char*) bytes)[1]) << 2*8 |
             ((uint32_t) ((unsigned char*) bytes)[0]) << 3*8;
#else
    return *((uint32_t*)(bytes));
#endif
}

//
// ----------------------
//

#ifdef PC
Uint32 * screenPixels;
#endif

#ifdef PC
void setPixel(int x, int y, uint32_t color)
{
    if(x>=0 && x < SCREEN_X && y>=0 && y < SCREEN_Y)
        screenPixels[y * SCREEN_X + x] = color;
}
//void LCD_ClearScreen()
void LCD_ClearScreen()
{
    memset(screenPixels, 255, SCREEN_X * SCREEN_Y * sizeof(Uint32));
}
inline uint32_t color(uint8_t R, uint8_t G, uint8_t B){
    return ((R<<8*2) | (G<<8*1) | (B<<8*0));
}

void fillScreen(uint32_t color)
{
    for (int x=0; x<SCREEN_X; x++){
        for (int y=0; y<SCREEN_Y; y++){
            setPixel(x,y,color);
        }
    }
}

//Draw a line (bresanham line algorithm)
void line(int x1, int y1, int x2, int y2, uint32_t color){
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
void vline(int x, int y1, int y2, uint32_t color){ //vertical line needed for triangle()
    if (y1>y2) { int z=y2; y2=y1; y1=z;}
    for (int y=y1; y<=y2; y++)
        setPixel(x,y,color);
}

//Draw a filled triangle.
void triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t colorFill, uint32_t colorLine){
//Filled triangles are a lot of vertical lines.
/*                                                               -
                        a   ___________----------P3              -
       P0 _________---------              ____---                -
          ---____               _____-----                       -
               b ----___  _-----   c                             -
                        P2                                       -
The triangle has three points P0, P1 and P2 and three lines a, b and c. We go from left to right, calculating the point on a and the point on b or c and then we draw a vertical line connecting these two.
*/

    //Sort the points by x coordinate
    {
        int z;
        if(x0>x2){ z=x2; x2=x0; x0=z; z=y2; y2=y0; y0=z; }
        if(x1>x2){ z=x2; x2=x1; x1=z; z=y2; y2=y1; y1=z; }
        if(x0>x1){ z=x1; x1=x0; x0=z; z=y1; y1=y0; y0=z; }
    }

    int x = x0; //x is the variable that counts from left to right

    //Values for line a
    int ay = y0; //The point y for the current x on the line a
    int aiy; //The direction of line a
    int adx = (x2>x0 ? (       x2-x0) : (        x0-x2) );
    int ady = (y2>y0 ? (aiy=1, y2-y0) : (aiy=-1, y0-y2) );
    int aerr = 0; //The y value of a (fractional part). y is actually ay+(aerr/adx)

    //Values for line b
    int by = y0; //The point y for the current x on the line b
    int biy; //The direction of line b
    int bdx = (x1>x0 ? (       x1-x0) : (        x0-x1) );
    int bdy = (y1>y0 ? (biy=1, y1-y0) : (biy=-1, y0-y1) );
    int berr = 0;

    //Values for line c
    int cy = y1; //The point y for the current x on the line y (starting at P1)
    int ciy; //The direction of line c
    int cdx = (x2>x1 ? (       x2-x1) : (        x1-x2) );
    int cdy = (y2>y1 ? (ciy=1, y2-y1) : (ciy=-1, y1-y2) );
    int cerr = 0;

    //First draw area between a and b
    while (x<x1){
        x++;
        aerr+=ady;
        while(aerr>=adx >> 2){ //if aerr/adx >= 0.5
            aerr-=adx;
            ay+=aiy;
        }
        berr+=bdy;
        while(berr>=bdx >> 2){ //if berr/bdx >= 0.5
            berr-=bdx;
            by+=biy;
        }
        vline(x,ay,by,colorFill);
    }

    //Then draw area between a and c
    while (x<x2-1){ //we don't need x=x2, bacause x should already have the right vaue...
        x++;
        aerr+=ady;
        while(aerr>=adx >> 2){ //if aerr/adx >= 0.5
            aerr-=adx;
            ay+=aiy;
        }
        cerr+=cdy;
        while(cerr>=cdx >> 2){ //if berr/bdx >= 0.5
            cerr-=cdx;
            cy+=ciy;
        }
        vline(x,ay,cy,colorFill);
    }

    line(x0,y0,x1,y1,colorLine);
    line(x1,y1,x2,y2,colorLine);
    line(x2,y2,x0,y0,colorLine);
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

template <class T>
inline void swap(T& a, T& b) {
    T tmp = b;
    b = a;
    a = tmp;
}

void bubble_sort(uint_fix16_t a[], int n) {
    for (int j = n; j > 1; --j)
        for (int i = 1; i < j; ++i)
            if (a[i - 1].fix16 < a[i].fix16)
                swap(a[i - 1], a[i]);
}

void drawHorizontalLine(int x0, int x1, int y, int u0, int u1, int v0, int v1, uint32_t *texture, int textureWidth, int textureHeight) {
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
            auto c = color(
                    (0xff & (texel>>16)),
                    (0xff & (texel>>8)),
                    (0xff & texel)
            );
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
            auto c = color(
                    (0xff & (texel>>16)),
                    (0xff & (texel>>8)),
                    (0xff & texel)
            );
            setPixel(x, y, c);
        }
    }
}

void drawTriangle(
    Point2d v0, Point2d v1, Point2d v2,
    uint32_t *texture, int textureWidth, int textureHeight
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

        drawHorizontalLine(x0, x1, y, u0, u1, v0_coord, v1_coord, texture, textureWidth, textureHeight);
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

        drawHorizontalLine(x0, x1, y, u0, u1, v0_coord, v1_coord, texture, textureWidth, textureHeight);
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

#ifndef PC
extern "C" void main()
{
    bool done = false;
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
#else
int drawCharacter(char character, int x, int y, Uint32* screenPixels) {
    const int BITMAP_SIZE = 6;
    const int SIZE_MULTIPLIER = 2;
    const char* bitmapNumbers6x6[] = {
        // 0
        "011110"
        "100001"
        "100001"
        "100001"
        "100001"
        "011110",

        // 1
        "001000"
        "011000"
        "001000"
        "001000"
        "001000"
        "111111",

        // 2
        "011110"
        "100001"
        "000010"
        "000100"
        "001000"
        "111111",

        // 3
        "011110"
        "100001"
        "000110"
        "000001"
        "100001"
        "011110",

        // 4
        "000100"
        "001100"
        "010100"
        "111111"
        "000100"
        "000100",

        // 5
        "111111"
        "100000"
        "111110"
        "000001"
        "100001"
        "011110",

        // 6
        "011110"
        "100001"
        "100000"
        "111110"
        "100001"
        "011110",

        // 7
        "111111"
        "000001"
        "000010"
        "000100"
        "001000"
        "010000",

        // 8
        "011110"
        "100001"
        "011110"
        "100001"
        "100001"
        "011110",

        // 9
        "011110"
        "100001"
        "100001"
        "011111"
        "000001"
        "011110"
    };
    // Calculate the index in the bitmapFont array based on the ASCII value of the character
    int index = static_cast<int>(character) - 48;
    // Loop through the character's bitmap and draw pixels onto screenPixels
    for (int i = 0; i < BITMAP_SIZE; i++) {
        for (int j = 0; j < BITMAP_SIZE; j++) {
            for (int k = 0; k < SIZE_MULTIPLIER; k++){
                for (int l = 0; l < SIZE_MULTIPLIER; l++){
                    // Set the corresponding pixel in screenPixels to a color value (e.g., white)
                    if (bitmapNumbers6x6[index][j * BITMAP_SIZE + i] == '1'){
                        setPixel(x+i*2+k, y+j*2+l, color(255,120,34));
                    }
                }
            }
        }
    }
    return BITMAP_SIZE*SIZE_MULTIPLIER;
}

void sdl_debug_uint32_t(uint32_t value, int x, int y) {
    const char* str = std::to_string(value).c_str();
    int currentX = x;
    // Loop through each character in the string
    for (int i = 0; str[i] != '\0'; ++i) {
        char character = str[i];
        // Draw the character at the current position
        currentX += drawCharacter(character, currentX, y, screenPixels) + 1;
        // Move to the next position
        // currentX += FONT_WIDTH + 1; // Add some space between characters
    }
}

int main(int argc, const char * argv[])
{
    bool done = false;
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        return -1;
    }

    // Create a window
    SDL_Window *window = SDL_CreateWindow("Classpad II PC demo",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SCREEN_X*2,
                                          SCREEN_Y*2,
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

    SDL_Texture * texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, SCREEN_X, SCREEN_Y);
    screenPixels = new Uint32[SCREEN_X * SCREEN_Y];

    if (DEBUG_TEST(texture, renderer)){
        done = true;
    }

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

    char model_path[] =
#ifdef PC
        "./a_processed_comp.PCObj";
#else
        "\\fls0\\a_processed_calc.PCObj";
#endif

    char model_texture_path[] =
#ifdef PC
        "./a_processed_comp.texture";
#else
        "\\fls0\\a_processed_calc.texture";
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

    // Floor model
    // #define FLOOR_SIZE   2.5f
    // #define FLOOR_HEIGHT 6.5f
    // fix16_vec3 vertices[] = {
    //     {-FLOOR_SIZE,  FLOOR_HEIGHT, -FLOOR_SIZE},
    //     {-FLOOR_SIZE,  FLOOR_HEIGHT,  FLOOR_SIZE},
    //     { FLOOR_SIZE,  FLOOR_HEIGHT, -FLOOR_SIZE},
    //     { FLOOR_SIZE,  FLOOR_HEIGHT,  FLOOR_SIZE},
    // };
    // unsigned vertex_count = sizeof(vertices)/sizeof(vertices[0]);
    // u_triple faces[] = {
    //     {0,1,2},
    //     {3,1,2},
    // };
    // unsigned faces_count  = sizeof(faces)/sizeof(faces[0]);
    // Model model_floor = Model(
    //     vertices,
    //     vertex_count,
    //     faces,
    //     faces_count
    // );

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
    uint32_t last_fps = 0;
#endif

#ifndef PC
    bool KEY_0_prev = false;
#endif
    const uint16_t RENDER_MODE_COUNT = 5;
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


        // ----- Check for exit -----
#ifndef PC
        uint32_t k1,k2; getKey(&k1,&k2);
        if(testKey(k1,k2,KEY_CLEAR)) {
            done = true;
        }

        if(testKey(k1,k2,KEY_4))  {
            //camera_pos.x -= 0.1f;
            camera_pos.z += camera_rot.x.sin()*0.60f;
            camera_pos.x -= camera_rot.x.cos()*0.60f;
        }
        if(testKey(k1,k2,KEY_6)) {
            //camera_pos.x += 0.1f;
            camera_pos.z -= camera_rot.x.sin()*0.60f;
            camera_pos.x += camera_rot.x.cos()*0.60f;
        }
        if(testKey(k1,k2,KEY_8))    {
            //camera_pos.z += 0.1f;
            camera_pos.x += camera_rot.x.sin()*0.60f;
            camera_pos.z += camera_rot.x.cos()*0.60f;
        }
        if(testKey(k1,k2,KEY_2))  {
            //camera_pos.z -= 0.1f;
            camera_pos.x -= camera_rot.x.sin()*0.60f;
            camera_pos.z -= camera_rot.x.cos()*0.60f;
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
        if(testKey(k1,k2,KEY_0)){
            if(KEY_0_prev == false){
                if(RENDER_MODE > 0)
                    RENDER_MODE = RENDER_MODE - 1;
                else
                    RENDER_MODE = RENDER_MODE_COUNT - 1;
            }
            KEY_0_prev = true;
        }else{
            KEY_0_prev = false;
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
                        case SDLK_e:
                            if(RENDER_MODE > 0)
                                RENDER_MODE = RENDER_MODE - 1;
                            else
                                RENDER_MODE = RENDER_MODE_COUNT - 1;;
                            break;
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
            camera_pos.z += camera_rot.x.sin()*0.025f;
            camera_pos.x -= camera_rot.x.cos()*0.025f;
        }
        if (key_right){
            camera_pos.z -= camera_rot.x.sin()*0.025f;
            camera_pos.x += camera_rot.x.cos()*0.025f;
        }
        if (key_up){
            camera_pos.x += camera_rot.x.sin()*0.025f;
            camera_pos.z += camera_rot.x.cos()*0.025f;
        }
        if (key_down){
            camera_pos.x -= camera_rot.x.sin()*0.025f;
            camera_pos.z -= camera_rot.x.cos()*0.025f;
        }
        if (key_r){
            camera_pos.y -= 0.006f;
        }
        if (key_f){
            camera_pos.y += 0.006f;
        }

        if (key_a){
            camera_rot.x -= 0.0035f;
        }
        if (key_d){
            camera_rot.x += 0.0035f;
        }
        if (key_w){
            camera_rot.y -= 0.0035f;
        }
        if (key_s){
            camera_rot.y += 0.0035f;
        }

        if (key_1){
            FOV += 0.1f;
        }
        if (key_2){
            FOV -= 0.1f;
        }
#endif

// --------------------------------------------------

#ifdef PC
        //all_models[all_model_count-1]->getRotation_ref().x += 0.0025f;
        //all_models[all_model_count-1]->getRotation_ref().y += 0.0003f;
#else
        //all_models[all_model_count-1]->getRotation_ref().x += 0.0025f*35.0f;
        //all_models[all_model_count-1]->getRotation_ref().y += 0.0003f*35.0f;
#endif

        if (RENDER_MODE == 0){
            for (unsigned m_id=0; m_id<all_model_count; m_id++)
            {
                // For each model..
                // Allocate memory first:
                //   1. Screen coordinates
                int16_t_vec2* screen_coords = (int16_t_vec2*) malloc(sizeof(int16_t_vec2) * all_models[m_id]->vertex_count);
                //   2. Draw order list
                Fix16 * vert_z_depths = (Fix16*) malloc(sizeof(Fix16) * all_models[m_id]->vertex_count);
                //   3. Face draw order (to be created)
                //unsigned int * face_draw_order = (unsigned int*) malloc(sizeof(unsigned int) * all_models[m_id]->faces_count);
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
                    //draw_center_square(x, y, 4,4, color(255,0,0));
                    //setPixel(x, y, color(0,0,0));
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
                #define SHADE_MAX 255
                #define SHADE_MIN 100
                //(for (unsigned f_id=0; f_id<all_models[m_id]->faces_count; f_id++)
                for (unsigned int ordered_id=0; ordered_id<all_models[m_id]->faces_count; ordered_id++)
                {
                    auto f_id = face_draw_order[ordered_id].uint;
                    auto v0_x = (screen_coords[all_models[m_id]->faces[f_id].First].x);
                    auto v0_y = (screen_coords[all_models[m_id]->faces[f_id].First].y);
                    auto v1_x = (screen_coords[all_models[m_id]->faces[f_id].Second].x);
                    auto v1_y = (screen_coords[all_models[m_id]->faces[f_id].Second].y);
                    auto v2_x = (screen_coords[all_models[m_id]->faces[f_id].Third].x);
                    auto v2_y = (screen_coords[all_models[m_id]->faces[f_id].Third].y);
                    const int16_t fix16_cast_int_min = (0xffff & (fix16_minimum>>16)) - 1;
                    if( v0_x == fix16_cast_int_min ||
                        v1_x == fix16_cast_int_min ||
                        v2_x == fix16_cast_int_min
                    ){
                        continue;
                    }

                    //auto shade = (ordered_id*(SHADE_MAX-SHADE_MIN))/all_models[m_id]->faces_count;

                    //uint32_t colorr =
                    //    0xff  << (ordered_id*(24)/all_models[m_id]->faces_count);

                    //Point2d v0 = {v0_x,v0_y, gen_textureWidth*0,gen_textureHeight*1};
                    //Point2d v1 = {v1_x,v1_y, gen_textureWidth*1,gen_textureHeight*1};
                    //Point2d v2 = {v2_x,v2_y, gen_textureWidth*1,gen_textureHeight*0};

                    auto uv0_fix16_norm = all_models[m_id]->uv_coords[all_models[m_id]->uv_faces[f_id].First];
                    auto uv1_fix16_norm = all_models[m_id]->uv_coords[all_models[m_id]->uv_faces[f_id].Second];
                    auto uv2_fix16_norm = all_models[m_id]->uv_coords[all_models[m_id]->uv_faces[f_id].Third];


                    auto v0_u = (int16_t) (uv0_fix16_norm.x * (Fix16((int16_t)all_models[m_id]->gen_textureWidth)));
                    auto v0_v = (int16_t) (uv0_fix16_norm.y * (Fix16((int16_t)all_models[m_id]->gen_textureHeight)));

                    auto v1_u = (int16_t) (uv1_fix16_norm.x * (Fix16((int16_t)all_models[m_id]->gen_textureWidth)));
                    auto v1_v = (int16_t) (uv1_fix16_norm.y * (Fix16((int16_t)all_models[m_id]->gen_textureHeight)));

                    auto v2_u = (int16_t) (uv2_fix16_norm.x * (Fix16((int16_t)all_models[m_id]->gen_textureWidth)));
                    auto v2_v = (int16_t) (uv2_fix16_norm.y * (Fix16((int16_t)all_models[m_id]->gen_textureHeight)));

                    Point2d v0 = {v0_x,v0_y, v0_u, v0_v};
                    Point2d v1 = {v1_x,v1_y, v1_u, v1_v};
                    Point2d v2 = {v2_x,v2_y, v2_u, v2_v};

                    drawTriangle(
                        v0, v1, v2,
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
        }
        else if (RENDER_MODE == 1){
            for (unsigned m_id=0; m_id<all_model_count; m_id++)
            {
                // For each model..
                // Allocate memory first:
                //   1. Screen coordinates
                int16_t_vec2* screen_coords = (int16_t_vec2*) malloc(sizeof(int16_t_vec2) * all_models[m_id]->vertex_count);
                //   2. Draw order list
                Fix16 * vert_z_depths = (Fix16*) malloc(sizeof(Fix16) * all_models[m_id]->vertex_count);
                //   3. Face draw order (to be created)
                //unsigned int * face_draw_order = (unsigned int*) malloc(sizeof(unsigned int) * all_models[m_id]->faces_count);
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
                    //draw_center_square(x, y, 4,4, color(255,0,0));
                    //setPixel(x, y, color(0,0,0));
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
                #define SHADE_MAX 255
                #define SHADE_MIN 100
                //(for (unsigned f_id=0; f_id<all_models[m_id]->faces_count; f_id++)
                for (unsigned int ordered_id=0; ordered_id<all_models[m_id]->faces_count; ordered_id++)
                {
                    auto f_id = face_draw_order[ordered_id].uint;
                    auto v0_x = (screen_coords[all_models[m_id]->faces[f_id].First].x);
                    auto v0_y = (screen_coords[all_models[m_id]->faces[f_id].First].y);
                    auto v1_x = (screen_coords[all_models[m_id]->faces[f_id].Second].x);
                    auto v1_y = (screen_coords[all_models[m_id]->faces[f_id].Second].y);
                    auto v2_x = (screen_coords[all_models[m_id]->faces[f_id].Third].x);
                    auto v2_y = (screen_coords[all_models[m_id]->faces[f_id].Third].y);
                    const int16_t fix16_cast_int_min = (0xffff & (fix16_minimum>>16)) - 1;
                    if( v0_x == fix16_cast_int_min ||
                        v1_x == fix16_cast_int_min ||
                        v2_x == fix16_cast_int_min
                    ){
                        continue;
                    }

                    uint32_t colorr =
                        0xff  << (ordered_id*(24)/all_models[m_id]->faces_count);

                    triangle(
                        v0_x,v0_y,
                        v1_x,v1_y,
                        v2_x,v2_y,
                        color(
                            //SHADE_MIN+shade,0,SHADE_MAX-shade
                            (colorr>>16)&0xcf, (colorr>>8)&0xcf, (colorr>>0)&0xcf
                        ),
                        color(0,0,0)
                    );
                }
                free(face_draw_order);
                free(vert_z_depths);
                free(screen_coords);
            }
        }
        else if (RENDER_MODE == 2){
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

                //(for (unsigned f_id=0; f_id<all_models[m_id]->faces_count; f_id++)
                for (unsigned int f_id=0; f_id<all_models[m_id]->faces_count; f_id++)
                {
                    auto v0_x = (screen_coords[all_models[m_id]->faces[f_id].First].x);
                    auto v0_y = (screen_coords[all_models[m_id]->faces[f_id].First].y);
                    auto v1_x = (screen_coords[all_models[m_id]->faces[f_id].Second].x);
                    auto v1_y = (screen_coords[all_models[m_id]->faces[f_id].Second].y);
                    auto v2_x = (screen_coords[all_models[m_id]->faces[f_id].Third].x);
                    auto v2_y = (screen_coords[all_models[m_id]->faces[f_id].Third].y);
                    const int16_t fix16_cast_int_min = (0xffff & (fix16_minimum>>16)) - 1;
                    if( v0_x == fix16_cast_int_min ||
                        v1_x == fix16_cast_int_min ||
                        v2_x == fix16_cast_int_min
                    ){
                        continue;
                    }

                    triangle(
                        v0_x,v0_y,
                        v1_x,v1_y,
                        v2_x,v2_y,
                        color(
                            //SHADE_MIN+shade,0,SHADE_MAX-shade
                            255,(f_id*8)%255,(f_id*16)%255
                        ),
                        color(0,0,0)
                    );
                }
                free(screen_coords);
            }
        }
        else if (RENDER_MODE == 3){
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
                    auto v0_x = (screen_coords[all_models[m_id]->faces[f_id].First].x);
                    auto v0_y = (screen_coords[all_models[m_id]->faces[f_id].First].y);
                    auto v1_x = (screen_coords[all_models[m_id]->faces[f_id].Second].x);
                    auto v1_y = (screen_coords[all_models[m_id]->faces[f_id].Second].y);
                    auto v2_x = (screen_coords[all_models[m_id]->faces[f_id].Third].x);
                    auto v2_y = (screen_coords[all_models[m_id]->faces[f_id].Third].y);
                    const int16_t fix16_cast_int_min = (0xffff & (fix16_minimum>>16)) - 1;
                    if( v0_x == fix16_cast_int_min ||
                        v1_x == fix16_cast_int_min ||
                        v2_x == fix16_cast_int_min
                    ){
                        continue;
                    }

                    line(v0_x,v0_y, v1_x, v1_y, color(0,0,0));
                    line(v1_x,v1_y, v2_x, v2_y, color(0,0,0));
                    line(v2_x,v2_y, v0_x, v0_y, color(0,0,0));

                }
                free(screen_coords);
            }
        }
        else if (RENDER_MODE == 4){
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
        }

        // Draw rotation visualizer in corner
        draw_RotationVisualizer(camera_rot);

// --------------------------------------------------

        // ----- Refresh screen -----
#ifndef PC
        #ifndef PC
        fps_formatted_update();
        fps_display();
        #endif
        LCD_Refresh();
#else
        // Draw FPS
        Uint32 currentTime = SDL_GetTicks();
        frames++;
        if (currentTime - startTime >= 250) {
            // If 1 second has passed, update the FPS string
            float fps = frames / ((currentTime - startTime) / 1000.0f);
            last_fps = (uint32_t) fps;
            // Reset the counters
            startTime = currentTime;
            frames = 0;
        }
        sdl_debug_uint32_t(last_fps, 10, 10);


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
