#pragma once

#ifdef PC
#   define SCREEN_X 320*1
#   define SCREEN_Y 528*1
#else
#   define SCREEN_X 320
#   define SCREEN_Y 528
#endif

#ifdef PC
#   define UNIVERSIAL_FILE_READ O_RDONLY
#else
#   define UNIVERSIAL_FILE_READ OPEN_READ
#endif