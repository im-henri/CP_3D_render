#pragma once

#ifdef PC
#   define _SCREEN_SIZE_MULTIPLIER 1
#else
#   define _SCREEN_SIZE_MULTIPLIER 1
#endif

#define SCREEN_X 320
#define SCREEN_Y 528


#ifdef PC
#   define UNIVERSIAL_FILE_READ O_RDONLY
#else
#   define UNIVERSIAL_FILE_READ OPEN_READ
#endif