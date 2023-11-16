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

Compile for computer with SDL2 graphics library
```
make PC
```

To create new binary format models + textures edit and run python script
```
python/ObjTexConverter.py
```

Credits:
hollyhock2: https://github.com/SnailMath/hollyhock-2
Libfixmath: https://github.com/PetteriAimonen/libfixmath/tree/master