#include "Model.hpp"

#include "StringUtils.hpp"

#ifndef PC
#   include <sdk/os/file.hpp>
#   include <sdk/os/mem.hpp>
#   include <sdk/os/debug.hpp>
#   include <sdk/os/lcd.hpp>
#else
#   include <SDL2/SDL.h>
#   include <iostream>
#   include <unistd.h>  // File open & close
#   include <fcntl.h>   // File open & close
#endif

Model::Model(
    fix16_vec3* vertices,
    unsigned vertex_count,
    u_pair* edges,
    unsigned edge_count
) : loaded_from_file(false),
    position({0.0f, 0.0f, 0.0f}), rotation({0.0f, 0.0f}), scale({1.0f,1.0f,1.0f}),
    vertices(vertices), vertex_count(vertex_count), edges(edges), edge_count(edge_count)
{ }

Model::~Model()
{
    if(loaded_from_file)
    {
        free(vertices);
        free(edges);
    }
}

Model::Model(
    char* fname
) : loaded_from_file(false),
    position({0.0f, 0.0f, 0.0f}), rotation({0.0f, 0.0f}), scale({1.0f,1.0f,1.0f}),
    vertices(nullptr), vertex_count(0), edges(nullptr), edge_count(0)
{
#ifndef PC
    Debug_SetCursorPosition(1,1);
    Debug_PrintString("ctor", false);
    LCD_Refresh();
#endif
    loaded_from_file = this->load_from_file(fname);
}

fix16_vec3& Model::getPosition_ref()
{
    return this->position;
}

fix16_vec2& Model::getRotation_ref()
{
    return this->rotation;
}

fix16_vec3& Model::getScale_ref()
{
    return this->scale;
}
#ifdef PC
#   define UNIVERSIAL_FILE_READ O_RDONLY
#else
#   define UNIVERSIAL_FILE_READ OPEN_READ
#endif

bool Model::load_from_file(char* fname)
{
    const unsigned MAX_FLOAT_SIZE_IN_STRING = 32; // 32 is kind of overkill

    // Doing in 2 passes to save memory at the cost
    // of performance. But its fine imo.
    bool success = false;
    const int LINE_BUFF_SZ = 128;
    char line_buff[LINE_BUFF_SZ];
#ifndef PC
    Debug_SetCursorPosition(1,1);
    Debug_PrintString("file", false);
    LCD_Refresh();
#endif
    int f_read = open(fname, UNIVERSIAL_FILE_READ);
#ifndef PC
    Debug_Printf(1,1, false, 0, "Opened file status %d", f_read);
    LCD_Refresh();
#endif
    // -- First Pass (finding sizes)
    int row_count = 0;
    while(read_line(f_read, line_buff, LINE_BUFF_SZ))
    {
#ifndef PC
    Debug_Printf(1,1, false, 0, "linebuf: %s", line_buff);
    Debug_Printf(1,3, false, 0, "vert cnt: %d", vertex_count);
    Debug_Printf(1,4, false, 0, "edge cnt: %d", edge_count);
    Debug_Printf(1,5, false, 0, "row  cnt: %d", row_count);
    LCD_Refresh();
    row_count++;
#endif
        if     (line_buff[0] == 'v') this->vertex_count += 1;
        else if(line_buff[0] == 'f') this->edge_count   += 3;
    }
#ifdef PC
    std::cout << "this->vertex_count: " << this->vertex_count << std::endl;
    std::cout << "this->edge_count:   " << this->edge_count   << std::endl;
#endif
#ifndef PC
    Debug_SetCursorPosition(1,1);
    Debug_PrintString("Creating data", false);
    LCD_Refresh();
#endif
    // -- Allocating now memories for the model
    this->vertices = (fix16_vec3*) malloc(sizeof(fix16_vec3) * this->vertex_count);
    this->edges    = (u_pair*)     malloc(sizeof(u_pair)     * this->edge_count);

    // -- Second pass actually reading data
    // Restart from file begin
    lseek(f_read, 0, SEEK_SET);
    int v_idx = 0;
    int edge_idx = 0;
    while(read_line(f_read, line_buff, LINE_BUFF_SZ))
    {
#ifndef PC
    Debug_Printf(1,10, false, 0, "lines left: %d", row_count);
    LCD_Refresh();
    row_count--;
#endif
        char * tmp_col = line_buff;
        if (line_buff[0] == 'v')
        {
            // Move character pointer to first vertex
            // skip 'v'
            tmp_col += 1;
            // skip whitespace
            while(*tmp_col == ' ') tmp_col += 1;
            // Iterate over each floating point value in the vertex
            for (int c_i=0; c_i<3; c_i++){
                // Buffer for turning float string to Fix16
                char buf_c_i[MAX_FLOAT_SIZE_IN_STRING] = {0}; // Assuming that the value is not larger than this
                // Find start and end of decimal value
                char* c_i_start = tmp_col; while(!(*tmp_col == ' ' || (*tmp_col == '\0'))) tmp_col += 1;
                char* c_i_end   = tmp_col; while(  *tmp_col == ' ') tmp_col += 1;
                // Copy into string to be passed to fix16_from_str(..). It must have NULL terminator at
                // end which is why we need extra separate buffer "buf_c_i".
                memcpy(buf_c_i, c_i_start, (c_i_end-c_i_start));
                if      (c_i == 0) vertices[v_idx].x = fix16_from_str(buf_c_i);
                else if (c_i == 1) vertices[v_idx].y = fix16_from_str(buf_c_i);
                else               vertices[v_idx].z = fix16_from_str(buf_c_i);
            }
            // Done with this vertex
            v_idx += 1;
        }
        else if(line_buff[0] == 'f')
        {
            // skip 'f'
            tmp_col += 1;
            // skip whitespace
            while(*tmp_col == ' ') tmp_col += 1;
            // Get all face vertex indexed
            int face_verts[3];
            for (int fv_i=0; fv_i<3; fv_i++){
                char* fv_i_start = tmp_col; while(!(*tmp_col == ' ' || (*tmp_col == '\0'))) tmp_col += 1;
                                            while(  *tmp_col == ' ') tmp_col += 1;
                face_verts[fv_i] = custom_atoi(fv_i_start) - 1;
            }
            // Now we can make lines to form the face
            for (int edge_i=0; edge_i<3; edge_i++)
            {
                edges[edge_idx].First  = face_verts[ (edge_i)  % 3];
                edges[edge_idx].Second = face_verts[(edge_i+1) % 3];
                edge_idx += 1;
            }
        }
    }

    close(f_read);

    return success;
}