3D Renderer using fixed point math. For casio ClassPad ii fx-cp400

Features:
- .obj models (converted to binary format)
- Textures
- Lighting
- 6 different render modes

Compile for calculator run makefile:
```
make
```
Copy everything to the root of calculator:
```
./App_sw_3d.bin
./3D_Converted_Models/big_endian_pika.pkObj
./3D_Converted_Models/big_endian_pika.texture
./3D_Converted_Models/big_endian_cube.pkObj
```


```
Keys:
8 4 2 6 9 3 = Camera rotation
D-PAD       = Movement
+ -         = FOV +/-
0           = Render Mode Cycle
Clear       = Exit
```


Compile for computer with SDL2 graphics library
```
make PC
```
```
Keys:
W S A D R F = Camera rotation
Arrow Keys  = Movement
1 2         = FOV +/-
E           = Render Mode Cycle
Esc         = Exit
```



To create new binary format models + textures edit and run python script
```
python/ObjTexConverter.py
```

Credits:
hollyhock2: https://github.com/SnailMath/hollyhock-2
Libfixmath: https://github.com/PetteriAimonen/libfixmath/tree/master