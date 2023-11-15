from ALL_PATHS import *
from PIL import Image

########################### INFO ###########################
## Converts *.obj and its texture to binary format
########################## USAGE ###########################
## Give .obj model path and texture path. Set other to None
## if only model or texture is needed. Then use big_endian
## version in classpad. (Computer uses most likely little.)
############################################################

model_path   = models_path / "test.obj"
texture_path = None
out_name     = "test"

#model_path   = models_path / "suzanne.obj"
#texture_path = None
#out_name     = "suzanne"

#model_path   = models_path / "pika_clown3.obj"
#texture_path = models_path / 'pika_clown3_512.png'
#out_name    = "pika"

########################## DETAILS #########################
## Writes *.obj out as custom binary format *.pkObj which
## speeds up processing *obj (suzanne.obj from 9 min to few ms)
##
## Format: [      1     ] 32b vertex count
##         [      1     ] 32b face count
##         [      1     ] 32b uv faces count
##         [      1     ] 32b uv coord count
##         [vertex count] (32b( x ) + 32b( y ) + 32b( z ) of type Fix16)
##         [ face count ] (32b( v0) + 32b( v1) + 32b( v2) of type uint32_t)
##         [uv faces cnt] (32b(uv0) + 32b(uv1) + 32b(uv2) of type uint32_t)
##         [uv coord cnt] (32b( u ) + 32b( v )            of type Fix16)
##
## Writes *.png texture out as custom binary format *.texture
## to ease and speed up reading the png on calculator.
##
## Format: [     1     ] 32b size x
##         [     1     ] 32b size y
##         [   x * y   ] 32b pixels of type uint32_t
##
## Saves file both in little and big endian
## (ClassPad - big endian) (Computer - most likely little endian)
############################################################

obj_out_big_endian    = project_root / f"big_endian_{out_name}.pkObj"
obj_out_little_endian = project_root / f"little_endian_{out_name}.pkObj"
tex_out_big_endian    = project_root / f"big_endian_{out_name}.texture"
tex_out_little_endian = project_root / f"little_endian_{out_name}.texture"

#############

def float_to_fix16(value: float):
    value = int(value * (1<<16))
    return value + (1<<32 if value < 0 else 0)

def write_out_32b(fBig, fLit, value):
    fBig.write(value.to_bytes(4, 'big'))
    fLit.write(value.to_bytes(4, 'little'))

def process_obj(path, out_big_endian, out_little_endian):
    obj_rows = ""
    with open(path) as f:
        obj_rows = f.read()

    vertices  = []
    faces     = []
    uv_face   = []
    uv_coords = []
    for row in obj_rows.split("\n"):
        row = row.strip()
        if len(row) == 0: continue
        if row[0:2] == "v ":
            coordinates = row.split()[1::]
            x = float(coordinates[0])
            y = float(coordinates[1])
            z = float(coordinates[2])
            vertices.append((x,y,z))
        elif row[0:2] == "vt":
            coordinates = row.split()[1::]
            u =  float(coordinates[0])
            v = 1.0 - float(coordinates[1])
            uv_coords.append((u,v))
        elif row[0:2] == "f ":
            face = row.split()[1::]
            v0_data = face[0].split("/")
            v1_data = face[1].split("/")
            v2_data = face[2].split("/")
            v0 = int(v0_data[0])
            v1 = int(v1_data[0])
            v2 = int(v2_data[0])
            if(len(v0_data) > 1):
                vt0 = int(v0_data[1])
                vt1 = int(v1_data[1])
                vt2 = int(v2_data[1])
                uv_face.append((vt0,vt1,vt2))
            faces.append((v0,v1,v2))

    print("Vertices:        ", len(vertices))
    print("faces_count:     ", len(faces))
    print("uv_face_count:   ", len(uv_face))
    print("uv_coord_count:  ", len(uv_coords))
    print("edges:           ", len(faces)*3)

    vert_count     = len(vertices)
    face_count     = len(faces)
    uv_faces_count = len(uv_face)
    uv_coord_count = len(uv_coords)
    #edge_count = face_count*3

    # Computer (my processor wants big-endian format)
    fBig = open(out_big_endian, "wb")
    # Classpad wants little-endian
    fLit = open(out_little_endian, "wb")

    # Write info about length of our data
    write_out_32b(fBig, fLit, vert_count)
    write_out_32b(fBig, fLit, face_count)
    write_out_32b(fBig, fLit, uv_faces_count)
    write_out_32b(fBig, fLit, uv_coord_count)

    # Write vertices in fix16 format
    for v in vertices:
        for i in range(3):
            write_out_32b(fBig, fLit, float_to_fix16(v[i]))
    # Write faces in integer format
    for f in faces:
        for i in range(3):
            write_out_32b(fBig, fLit, f[i]-1)
    # Write faces in integer format
    for uvf in uv_face:
        for i in range(3):
            write_out_32b(fBig, fLit, uvf[i]-1)
    # Write uv coordinates in fix16 format
    for v in uv_coords:
        for i in range(2):
            write_out_32b(fBig, fLit, float_to_fix16(v[i]))

    fBig.close()
    fLit.close()

def process_texture(texture_path, out_big_endian, out_little_endian):
    def png_to_hextable(texture_path):
        im = Image.open(texture_path) # Can be many different formats.
        png = im.load()
        png_array = []
        for y in range(im.size[1]):
            tmp = []
            for x in range(im.size[0]):
                rgba = png[x,y]
                rgb  = rgba[0:3]
                rgb_hex = int(rgb[0]) << 2*8 | int(rgb[1]) << 1*8 | int(rgb[2]);
                rgb_hex = hex(rgb_hex)
                tmp.append(rgb_hex)
            png_array.append(tmp)
        return png_array, im.size[0], im.size[1]

    texture, size_x, size_y = png_to_hextable(texture_path)
    if (size_x != size_y):
        print(f"Error: Texture must be square for now! size x = {size_x} size y = {size_y} ")
        raise TypeError("Non-square texture may not be supported. Too lazy to check if I added support...")
    else:
        print(f"Generating binary texture\nsize x {size_x}\nsize y {size_y}")

    fBig = open(out_big_endian, "wb")
    fLit = open(out_little_endian, "wb")

    # Write info about length of our data
    write_out_32b(fBig, fLit, size_x)
    write_out_32b(fBig, fLit, size_y)
    # Write vertices in uint32 format
    for row in texture:
        for pix in row:
            write_out_32b(fBig, fLit, int(pix, base=16))
    fBig.close()
    fLit.close()

def main():
    if model_path != None:
        process_obj(model_path, obj_out_big_endian, obj_out_little_endian)
    if texture_path != None:
        process_texture(texture_path, tex_out_big_endian, tex_out_little_endian)

if __name__ == "__main__":
    main()