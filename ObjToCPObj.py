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

out_calculator = "./processed_calc.PCObj"
out_computer   = "./processed_comp.PCObj"

def process_obj(path, out_calculator, out_computer):
    obj_rows = ""
    with open(path) as f:
        obj_rows = f.read()

    vertices = []
    faces    = []
    for row in obj_rows.split("\n"):
        row = row.strip()
        if len(row) == 0: continue

        if row[0] == "v":
            coordinates = row.split()[1::]
            x = float(coordinates[0])
            y = float(coordinates[1])
            z = float(coordinates[2])
            vertices.append((x,y,z))
        elif row[0] == "f":
            face = row.split()[1::]
            v0 = int(face[0])
            v1 = int(face[1])
            v2 = int(face[2])
            faces.append((v0,v1,v2))
    print("Vertices:", len(vertices))
    print("faces:   ", len(faces))
    print("edges:   ", len(faces)*3)

    vert_count = len(vertices)
    face_count = len(faces)
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
    # Write vertices in fix16 format
    for v in vertices:
        for i in range(3):
            write_out_32b( float_to_fix16(v[i]) )
    # Write faces in integer format
    for f in faces:
        for i in range(3):
            write_out_32b(f[i]-1)

    fPC.close()
    fCP.close()

def float_to_fix16(value: float):
    value = int(value * (1<<16))
    return value + (1<<32 if value < 0 else 0)

def main():
    process_obj(model_path, out_calculator, out_computer)

if __name__ == "__main__":
    main()