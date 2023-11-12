
out_calculator = "./a_processed_calc.texture"
out_computer   = "./a_processed_comp.texture"

def init_table1(size, initUpper=True, initLower=True):
    table = [[0 for i in range(size)] for j in range(size)]
    y = 0
    if initLower:
        for circle in range(size):
            for x in range(circle+1):
                table[y][x] = y
            y += 1
    if initUpper:
        x = 0
        for circle in range(size):
            for y in range(circle+1):
                table[y][x] = x
            x += 1
    return table

def init_table2(size, q0, q1, q2, q3):
    table = [["0x00000000" for i in range(size)] for j in range(size)]
    def initQuarter(value, xmin, xmax, ymin, ymax):
        for x in range(xmin, xmax):
            for y in range(ymin, ymax):
                table[x][y] = value
    # Upper left
    initQuarter(q0,
        int(size*0/2),
        int(size*1/2),
        int(size*0/2),
        int(size*1/2))
    # Upper right
    initQuarter(q1,
        int(size*0/2),
        int(size*1/2),
        int(size*1/2),
        int(size*2/2))
    # Lower left
    initQuarter(q2,
        int(size*1/2),
        int(size*2/2),
        int(size*0/2),
        int(size*1/2))
    # Lower right
    initQuarter(q3,
        int(size*1/2),
        int(size*2/2),
        int(size*1/2),
        int(size*2/2))
    return table

def png_to_hextable():
    from PIL import Image

    im = Image.open('pika_clown3_1024.png') # Can be many different formats.
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
        #tmp.reverse()
        png_array.append(tmp)
    #png_array.reverse()
    return png_array, im.size[0], im.size[1]

def main():
    texture, size_x, size_y = png_to_hextable()
    if (size_x != size_y):
        print(f"Error: Texture must be square for now! size x = {size_x} size y = {size_y} ")
    else:
        print(f"Generating \"gen_uv_tex.hpp\" for texture sized({size_x}, {size_y})")
    textureSize = size_x

    #textureSize = 30
    #texture = init_table2(textureSize, "0xff0000", "0x000000", "0x0000ff", "0x00ff00")

    # Computer (my processor wants big-endian format)
    fPC = open(out_calculator, "wb")
    # Classpad wants little-endian
    fCP = open(out_computer, "wb")
    def write_out_32b(value):
        fPC.write(value.to_bytes(4, 'big'))
        fCP.write(value.to_bytes(4, 'little'))
    # Write info about length of our data
    write_out_32b(size_x)
    write_out_32b(size_y)
    # Write vertices in uint32 format
    for row in texture:
        for pix in row:
            write_out_32b( int(pix, base=16) )
    fPC.close()
    fCP.close()

    with open("src/gen_uv_tex.hpp", "w") as f:
        f.write(f"#pragma once\n\n")
        f.write(f"const int gen_textureWidth  = {textureSize};\n")
        f.write(f"const int gen_textureHeight = {textureSize};\n")
        f.write("uint32_t gen_uv_tex[gen_textureWidth*gen_textureHeight] = {\n")
        for row in texture:
            f.write("    ")
            for item in row:
                f.write(f"{item}, ")
            f.write("\n")
        f.write("};")

if __name__ == "__main__":
    main()