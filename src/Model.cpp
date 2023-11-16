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

Model::~Model()
{
    if(loaded_from_file)
    {
        free(vertices);
        free(faces);
        free(uv_faces);
        free(uv_coords);
        if(has_texture){
            free(gen_uv_tex);
        }
    }
}

Model::Model(
    char* fname,
    char* ftexture,
    bool centerVertices
) : loaded_from_file(false),
    position({0.0f, 0.0f, 0.0f}), rotation({0.0f, 0.0f}), scale({1.0f,1.0f,1.0f}),
    vertices(nullptr), vertex_count(0),
    faces(nullptr), faces_count(0),
    has_texture(false),
    gen_textureWidth(0), gen_textureHeight(0),
    render_mode(0)
{
    loaded_from_file = this->load_from_binary_obj_file(fname, ftexture, centerVertices);
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

// Transform original model vertices to the geometric center
void Model::_centerModel()
{
    // Find the current center of the model
    fix16_vec3 center = {0.0f, 0.0f, 0.0f};
    for (unsigned int i = 0; i < vertex_count; ++i) {
        center.x += (vertices[i].x) / ((int16_t) vertex_count);
        center.y += (vertices[i].y) / ((int16_t) vertex_count);
        center.z += (vertices[i].z) / ((int16_t) vertex_count);
    }
    // Translate all vertices by the negative of the center
    for (unsigned int  i = 0; i < vertex_count; ++i) {
        vertices[i].x -= center.x;
        vertices[i].y -= center.y;
        vertices[i].z -= center.z;
    }
}

// Transform raw model vertices to the geometric center
void Model::_scaleModel(Fix16 factor)
{
    // Translate all vertices by the negative of the center
    for (unsigned int  i = 0; i < vertex_count; ++i) {
        vertices[i].x *= factor;
        vertices[i].y *= factor;
        vertices[i].z *= factor;
    }
}

// Scales model such that max distance between to furthest
void Model::_scaleModelTo(Fix16 maxWidth)
{
    // Find two vertices that are furthest apart
    fix16_vec3 min_vert = {vertices[0].x, vertices[0].y ,vertices[0].z};
    Fix16 last_min_sum = vertices[0].x + vertices[0].y + vertices[0].z;
    fix16_vec3 max_vert = {vertices[0].x, vertices[0].y ,vertices[0].z};
    Fix16 last_max_sum = vertices[0].x + vertices[0].y + vertices[0].z;
    //
    // Simply add together all coordinates and one with highest sum
    // is "furthest" and one with lowest sum is "closest"
    for (unsigned int i = 1; i < vertex_count; ++i) {
        Fix16 sum = vertices[i].x + vertices[i].y + vertices[i].z;
        if (sum < last_min_sum)
        {
            last_min_sum = sum;
            min_vert = {vertices[i].x, vertices[i].y ,vertices[i].z};
        }
        else if (sum > last_max_sum)
        {
            last_max_sum = sum;
            max_vert = {vertices[i].x, vertices[i].y ,vertices[i].z};
        }
    }
    Fix16 dx = max_vert.x - min_vert.x;
    Fix16 dy = max_vert.y - min_vert.y;
    Fix16 dz = max_vert.z - min_vert.z;
    Fix16 currentMaxWidth = (dx * dx + dy * dy + dz * dz).sqrt();
    Fix16 scaleFactor = maxWidth/currentMaxWidth;

    _scaleModel(scaleFactor);
}

// Scale raw model vertices
bool Model::load_from_binary_obj_file(char* fname, char* ftexture, bool center)
{
    // ~~~~~~~~~~~~~~~~~~~~~ Object ~~~~~~~~~~~~~~~~~~~~~

    int fd = open(fname, UNIVERSIAL_FILE_READ );
    char buff[32] = {0};

    read(fd, buff, 31);
    uint32_t vert_count = *((uint32_t*)(buff+0));
    uint32_t face_count = *((uint32_t*)(buff+4));
    uint32_t uvface_count  = *((uint32_t*)(buff+8));
    uint32_t uvcoord_count = *((uint32_t*)(buff+12));

    //
    this->vertex_count = vert_count;
    this->faces_count = face_count;
    this->uv_face_count = uvface_count;
    this->uv_coord_count = uvcoord_count;
    //uvface_count
    unsigned lseek_vert_start    = 16;
    unsigned lseek_vert_end      = lseek_vert_start + vert_count * 3 * 4;

    unsigned lseek_face_start    = lseek_vert_end;
    unsigned lseek_face_end      = lseek_face_start + faces_count * 3 * 4;

    unsigned lseek_uvface_start  = lseek_face_end;
    unsigned lseek_uvface_end    = lseek_uvface_start + uv_face_count * 3 * 4;

    unsigned lseek_uvcoord_start = lseek_uvface_end;

    // Read binary to vertices
    lseek(fd, lseek_vert_start, SEEK_SET);
    this->vertices = (fix16_vec3*) malloc(sizeof(fix16_vec3) * this->vertex_count);
    read(fd, this->vertices, vert_count*3*4);   // vert_count(?x) * x,y,z(3x) * 32b Fix16 (4bytes)

    // Read binary to faces
    lseek(fd, lseek_face_start, SEEK_SET);
    this->faces    = (u_triple*)   malloc(sizeof(u_triple)   * this->faces_count);
    read(fd, this->faces, face_count*3*4);      // face_count(?x) * v0 v1 v2 (3x) * 32b unsigned (4bytes)

    // Read binary to uv faces
    lseek(fd, lseek_uvface_start, SEEK_SET);
    this->uv_faces = (u_triple*)   malloc(sizeof(u_triple)   * this->uv_face_count);
    read(fd, this->uv_faces, uvface_count*3*4); // uv_face_count(?x) * v0 v1 v2 (3x) * 32b unsigned (4bytes)

    // Read binary to uv coords
    lseek(fd, lseek_uvcoord_start, SEEK_SET);
    this->uv_coords = (fix16_vec2*) malloc(sizeof(fix16_vec2) * this->uv_coord_count);
    read(fd, this->uv_coords, uv_coord_count*2*4);   // uv_coord_count(?x) * u,v(2x) * 32b Fix16 (4bytes)

    // File has been completely read
    close(fd);

    // Center model
    if(center)
        _centerModel();

    // ~~~~~~~~~~~~~~~~~~~~~ Texture ~~~~~~~~~~~~~~~~~~~~~

    if (ftexture[0] == '\0'){
#ifdef PC
        std::cout << "No texture was provided" << std::endl;
#endif
        this->has_texture = false;
        return true;
    }

    if (uv_face_count == 0){
#ifdef PC
        std::cout << "Model has no texture (No UV coordinates). Not loading texture." << std::endl;
#endif
        this->has_texture = false;
        return true;
    }

    // Now load
    fd = open(ftexture, UNIVERSIAL_FILE_READ);
    memset(buff, 0, 32);
    read(fd, buff, 31);
    uint32_t tex_size_x = *((uint32_t*)(buff+0));
    uint32_t tex_size_y = *((uint32_t*)(buff+4));

    unsigned lseek_texture_start    = 8;

    this->gen_textureWidth  = tex_size_x;
    this->gen_textureHeight = tex_size_y;
    this->has_texture = true;

#ifdef PC
        std::cout
                 << "tex_size_x = " << tex_size_x
                 << " tex_size_y = " << tex_size_y
                 << std::endl;
#endif
    // Read binary to textuer
    lseek(fd, lseek_texture_start, SEEK_SET);
    this->gen_uv_tex = (uint32_t*) malloc(sizeof(uint32_t) * tex_size_x*tex_size_y);
    read(fd, this->gen_uv_tex, tex_size_x*tex_size_y*4);   // tex_size_x*tex_size_y(?x) * 32b Fix16 (4bytes)

    close(fd);

    return true;
}
