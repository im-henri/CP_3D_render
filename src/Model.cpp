#include "Model.hpp"

#include "StringUtils.hpp"

#include "constants.hpp"

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
    u_triple*   faces,
    unsigned    faces_count
    // u_pair* edges,       // TODO MUST ADD THESE ASWELL!
    // unsigned edge_count  // TODO MUST ADD THESE ASWELL!
) : loaded_from_file(false),
    position({0.0f, 0.0f, 0.0f}), rotation({0.0f, 0.0f}), scale({1.0f,1.0f,1.0f}),
    vertices(vertices), vertex_count(vertex_count),
    faces(faces), faces_count(faces_count)
{ }

Model::~Model()
{
    if(loaded_from_file)
    {
        free(vertices);
        free(faces);
    }
}

Model::Model(
    char* fname
) : loaded_from_file(false),
    position({0.0f, 0.0f, 0.0f}), rotation({0.0f, 0.0f}), scale({1.0f,1.0f,1.0f}),
    faces(nullptr), faces_count(0),
    vertices(nullptr), vertex_count(0)
{
    //loaded_from_file = this->load_from_raw_obj_file(fname);
    loaded_from_file = this->load_from_binary_obj_file(fname);
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

bool Model::load_from_binary_obj_file(char* fname)
{
    int fd = open(fname, UNIVERSIAL_FILE_READ );
    char buff[16] = {0};
    int rd_bytes;

    rd_bytes = read(fd, buff, 8+1);
    uint32_t vert_count = *((uint32_t*)(buff+0));
    uint32_t face_count = *((uint32_t*)(buff+4));
    //
    this->vertex_count = vert_count;
    this->faces_count = face_count;
    //
    unsigned lseek_vert_start = 8;
    unsigned lseek_face_start = lseek_vert_start + vert_count * 3 * 4;

    // Read binary to vertices
    lseek(fd, lseek_vert_start, SEEK_SET);
    this->vertices = (fix16_vec3*) malloc(sizeof(fix16_vec3) * this->vertex_count);
    rd_bytes = read(fd, this->vertices, vert_count*3*4); // vert_count(?x) * x,y,z(3x) * 32b Fix16 (4bytes)
    // Read binary to faces
    lseek(fd, lseek_face_start, SEEK_SET);
    this->faces    = (u_triple*)   malloc(sizeof(u_triple)   * this->faces_count);
    rd_bytes = read(fd, this->faces, face_count*3*4);   // face_count(?x) * v0 v1 v2 (3x) * 32b unsigned (4bytes)

    return true;
}
bool Model::load_from_raw_obj_file(char* fname)
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
    Debug_Printf(1,5, false, 0, "row  cnt: %d", row_count);
    LCD_Refresh();
    row_count++;
#endif
        if     (line_buff[0] == 'v') this->vertex_count += 1;
        else if(line_buff[0] == 'f') this->faces_count  += 1;
    }
#ifdef PC
    std::cout << "this->vertex_count: " << this->vertex_count << std::endl;
    std::cout << "this->faces_count:  " << this->faces_count  << std::endl;
#endif
#ifndef PC
    Debug_SetCursorPosition(1,1);
    Debug_PrintString("Creating data", false);
    LCD_Refresh();
#endif
    // -- Allocating now memories for the model
    this->vertices = (fix16_vec3*) malloc(sizeof(fix16_vec3) * this->vertex_count);
    this->faces    = (u_triple*)   malloc(sizeof(u_triple)   * this->faces_count);

    // -- Second pass actually reading data
    // Restart from file begin
    lseek(f_read, 0, SEEK_SET);
    int v_idx = 0;
    int face_idx = 0;
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
            faces[face_idx].First  = face_verts[0];
            faces[face_idx].Second = face_verts[1];
            faces[face_idx].Third  = face_verts[2];
            face_idx += 1;
        }
    }

    close(f_read);

    return success;
}