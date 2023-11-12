######################################
##
## Writes .obj out as binary format for faster reading
##
## Format: [      1     ] 32b vertex count
##         [      1     ] 32b face count
##         [vertex count] (32b(x) + 32b(y) + 32b(z) Fix16)
##         [ face count ] (32b(v0)+ 32b(v1)+ 32b(v3) Unsigned)
##
## Saves file both in little and big endian
## (Classpad -little) (Computer ?)
#########################################

#model_path = "./suzanne.obj"
model_path = "./pikatchu.obj"
model_path = "./pikatchu_simple.obj"
model_path = "./pika_clown3.obj"
#model_path = "./cube_test.obj"

out_calculator = "./a_processed_calc.PCObj"
out_computer   = "./a_processed_comp.PCObj"

def process_obj(path, out_calculator, out_computer):
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
    fPC = open(out_calculator, "wb")
    # Classpad wants little-endian
    fCP = open(out_computer, "wb")

    def write_out_32b(value):
        fPC.write(value.to_bytes(4, 'big'))
        fCP.write(value.to_bytes(4, 'little'))

    # Write info about length of our data
    write_out_32b(vert_count)
    write_out_32b(face_count)
    write_out_32b(uv_faces_count)
    write_out_32b(uv_coord_count)

    # Write vertices in fix16 format
    for v in vertices:
        for i in range(3):
            write_out_32b( float_to_fix16(v[i]) )
    # Write faces in integer format
    for f in faces:
        for i in range(3):
            write_out_32b(f[i]-1)
    # Write faces in integer format
    for uvf in uv_face:
        for i in range(3):
            write_out_32b(uvf[i]-1)
    # Write uv coordinates in fix16 format
    for v in uv_coords:
        for i in range(2):
            write_out_32b( float_to_fix16(v[i]) )

    fPC.close()
    fCP.close()

def float_to_fix16(value: float):
    value = int(value * (1<<16))
    return value + (1<<32 if value < 0 else 0)

def main():
    process_obj(model_path, out_calculator, out_computer)

if __name__ == "__main__":
    main()