
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

#define FILL_SCREEN_COLOR color(130,130,130)



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

bool DEBUG_TEST()
{
    return false;
    // True  = EXIT
    // False = CONTINUE

    int fd = open(
#ifdef PC
        "./processed_comp.PCObj",
#else
        "\\fls0\\processed_calc.PCObj",
#endif
        UNIVERSIAL_FILE_READ
    );

    char buff[1024] = {0};
    int rd_bytes;

    rd_bytes = read(fd, buff, 8+1);
    uint32_t vert_count = *((uint32_t*)(buff+0));
    uint32_t face_count = *((uint32_t*)(buff+4));
    //
    unsigned lseek_vert_start = 8;
    unsigned lseek_face_start = lseek_vert_start + vert_count * 3 * 4;

    lseek(fd, lseek_vert_start, SEEK_SET);
    fix16_vec3* verts = (fix16_vec3*) malloc(sizeof(fix16_vec3) * vert_count);
    rd_bytes = read(fd, verts, vert_count*3*4);

    lseek(fd, lseek_face_start, SEEK_SET);
    u_triple* faces    = (u_triple*) malloc(sizeof(u_triple) * face_count);
    rd_bytes = read(fd, faces, face_count*3*4);   // face_count(?x) * v0 v1 v2 (3x) * 32b unsigned (4bytes)


#ifdef PC
    std::cout << "rd_bytes:   0x" << std::hex << rd_bytes << std::endl;
    std::cout << "vert_count: 0x" << std::hex << vert_count  << std::endl;
    std::cout << "face_count: 0x" << std::hex << face_count  << std::endl;
    for(int i=0; i<vert_count; i++){
        std::cout << "v("<<i<<")  "
            << (float) verts[i].x << " "
            << (float) verts[i].y << " "
            << (float) verts[i].z << std::endl;
    }
    std::cout << std::endl;
    for(int i=0; i<face_count; i++){
        std::cout << "v("<<i<<")  "
            << faces[i].First << " "
            << faces[i].Second << " "
            << faces[i].Third << std::endl;
    }
#else
    fillScreen(FILL_SCREEN_COLOR);
    Debug_Printf(1,1, false, 0, "rd_bytes:   0x%x", rd_bytes);
    Debug_Printf(1,2, false, 0, "vert_count: 0x%x", vert_count );
    Debug_Printf(1,3, false, 0, "face_count: 0x%x", face_count );
    Debug_Printf(1,4, false, 0, "Reading 0:  0x%x", verts[0].x.value );
    Debug_Printf(1,5, false, 0, "Reading 1:  0x%x", verts[0].y.value );
    LCD_Refresh();
#endif

    close(fd);
    free(verts);

    return true;
    // True  = EXIT
    // False = CONTINUE
}


//
// ----------------------
//


#ifdef PC
Uint32 * screenPixels;
void setPixel(int x, int y, uint32_t color)
{
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

#ifndef PC
extern "C" void main()
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
#else
int main(int argc, const char * argv[])
{
    if (DEBUG_TEST()){
        return 0;
    }

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

/*
#ifdef PC
    //char model_path[] = "./hi.obj";
    //char model_path[] = "./test.obj";
    char model_path[] = "./suzanne.obj";
#else
    char model_path[] = "\\fls0\\suzanne.obj";
    //char model_path[] = "\\fls0\\hi.obj";
#endif
*/
    char model_path[] =
#ifdef PC
        "./processed_comp.PCObj";
#else
        "\\fls0\\processed_calc.PCObj";
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

    #define FLOOR_SIZE   2.5f
    #define FLOOR_HEIGHT 6.5f
    fix16_vec3 vertices[] = {
        {-FLOOR_SIZE,  FLOOR_HEIGHT, -FLOOR_SIZE},
        {-FLOOR_SIZE,  FLOOR_HEIGHT,  FLOOR_SIZE},
        { FLOOR_SIZE,  FLOOR_HEIGHT, -FLOOR_SIZE},
        { FLOOR_SIZE,  FLOOR_HEIGHT,  FLOOR_SIZE},
    };
    unsigned vertex_count = sizeof(vertices)/sizeof(vertices[0]);
    u_triple faces[] = {
        {0,1,2},
        {3,1,2},
    };
    unsigned faces_count  = sizeof(faces)/sizeof(faces[0]);
    Model model_floor = Model(
        vertices,
        vertex_count,
        faces,
        faces_count
    );

    Model model_test  = Model(model_path);
    model_test.getRotation_ref().y = Fix16(3.145f/2.0f);

    Model* all_models[] = {
        &model_floor,
        // &cube1, &testmodel, &model_test
        &model_test
    };
    const unsigned all_model_count = sizeof(all_models) / sizeof(all_models[0]);

    fix16_vec3 camera_pos = {-9.0f, -1.6f, -10.0f};
    fix16_vec2 camera_rot = {0.8f, 0.2f};

    Fix16 FOV = 300.0f; // Does not mean 300 degrees, some "arbitrary" meaning

    bool done = false;
    while(!done)
    {
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
            camera_pos.z += camera_rot.x.sin()*0.20f;
            camera_pos.x -= camera_rot.x.cos()*0.20f;
        }
        if(testKey(k1,k2,KEY_6)) {
            //camera_pos.x += 0.1f;
            camera_pos.z -= camera_rot.x.sin()*0.20f;
            camera_pos.x += camera_rot.x.cos()*0.20f;
        }
        if(testKey(k1,k2,KEY_8))    {
            //camera_pos.z += 0.1f;
            camera_pos.x += camera_rot.x.sin()*0.20f;
            camera_pos.z += camera_rot.x.cos()*0.20f;
        }
        if(testKey(k1,k2,KEY_2))  {
            //camera_pos.z -= 0.1f;
            camera_pos.x -= camera_rot.x.sin()*0.20f;
            camera_pos.z -= camera_rot.x.cos()*0.20f;
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
            camera_pos.z += camera_rot.x.sin()*0.009f;
            camera_pos.x -= camera_rot.x.cos()*0.009f;
        }
        if (key_right){
            camera_pos.z -= camera_rot.x.sin()*0.009f;
            camera_pos.x += camera_rot.x.cos()*0.009f;
        }
        if (key_up){
            camera_pos.x += camera_rot.x.sin()*0.009f;
            camera_pos.z += camera_rot.x.cos()*0.009f;
        }
        if (key_down){
            camera_pos.x -= camera_rot.x.sin()*0.009f;
            camera_pos.z -= camera_rot.x.cos()*0.009f;
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
#ifdef PC
        all_models[all_model_count-1]->getRotation_ref().x += 0.0025f;
        all_models[all_model_count-1]->getRotation_ref().y += 0.0003f;
#else
        all_models[all_model_count-1]->getRotation_ref().x += 0.0025f*35.0f;
        all_models[all_model_count-1]->getRotation_ref().y += 0.0003f*35.0f;
#endif
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
                auto screen_vec2 = getScreenCoordinate(
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
                auto shade = (ordered_id*(SHADE_MAX-SHADE_MIN))/all_models[m_id]->faces_count;

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
                /*
                line((int16_t)(screen_coords[all_models[m_id]->faces[f_id].First].x),
                     (int16_t)(screen_coords[all_models[m_id]->faces[f_id].First].y),
                     (int16_t)(screen_coords[all_models[m_id]->faces[f_id].Second].x),
                     (int16_t)(screen_coords[all_models[m_id]->faces[f_id].Second].y),
                    color(0,0,0)
                );
                line((int16_t)(screen_coords[all_models[m_id]->faces[f_id].Second].x),
                     (int16_t)(screen_coords[all_models[m_id]->faces[f_id].Second].y),
                     (int16_t)(screen_coords[all_models[m_id]->faces[f_id].Third].x),
                     (int16_t)(screen_coords[all_models[m_id]->faces[f_id].Third].y),
                    color(0,0,0)
                );
                line((int16_t)(screen_coords[all_models[m_id]->faces[f_id].Third].x),
                     (int16_t)(screen_coords[all_models[m_id]->faces[f_id].Third].y),
                     (int16_t)(screen_coords[all_models[m_id]->faces[f_id].First].x),
                     (int16_t)(screen_coords[all_models[m_id]->faces[f_id].First].y),
                    color(0,0,0)
                );
                */
            }
            free(face_draw_order);
            free(vert_z_depths);
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
