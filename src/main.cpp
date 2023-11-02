
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
    fillScreen(color(255,255,255));
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
    memset(screenPixels, (Uint32)color, SCREEN_X * SCREEN_Y * sizeof(Uint32));
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
            /*
            */

            // Draw face edges
            for (unsigned f_id=0; f_id<all_models[m_id]->faces_count; f_id++)
            {
                triangle(
                    (int16_t)(screen_coords[all_models[m_id]->faces[f_id].First].x),
                    (int16_t)(screen_coords[all_models[m_id]->faces[f_id].First].y),
                    (int16_t)(screen_coords[all_models[m_id]->faces[f_id].Second].x),
                    (int16_t)(screen_coords[all_models[m_id]->faces[f_id].Second].y),
                    (int16_t)(screen_coords[all_models[m_id]->faces[f_id].Third].x),
                    (int16_t)(screen_coords[all_models[m_id]->faces[f_id].Third].y),
                    color(
                        155+(f_id*1)%100,
                        200,
                        97
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
