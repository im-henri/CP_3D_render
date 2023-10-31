#include "StringUtils.hpp"

#ifndef PC
#   include <sdk/os/file.hpp>
#   include <sdk/os/mem.hpp>
#   include <sdk/os/lcd.hpp>
#   include <sdk/os/debug.hpp>
#else
#   include <SDL2/SDL.h>
#   include <iostream>
#   include <unistd.h>  // File open & close
#   include <fcntl.h>   // File open & close
#endif

// Does not require NULL termination
int custom_atoi(char* str)
{
    int value = 0;
    int sign = 1;
    if(*str == '-')
    {
        sign = -1;
        str++;
    }
    // Check numeric value
    while (*str >= '0' && *str <= '9')
    {
        value *= 10;
        value += (int) (*str-'0');
        str++;
    }
    return (value * sign);
}

// Returns: true if could find target
//          false could not find (Also seeks back to original location)
bool seek_next_char(int fd, char target)
{
    bool found = false;
    const uint16_t BUF_SIZE = 64;
    char buf[BUF_SIZE];
    int total_seek = 0;
    while(!found){
        int rd_bytes = read(fd, buf, BUF_SIZE);
        // Check if end of file was reached
        if (rd_bytes <= 0)
            break;
        total_seek += rd_bytes;
        // Go through buf to check if target was there
        // And seek to the target when found
        uint16_t idx = 0;
        while(buf[idx] != '\0'){
            if (buf[idx] == target){
                lseek(fd, -rd_bytes+idx, SEEK_CUR);
                found = true;
                break;
            }
            idx++;
        }
    }
    if(!found)
        lseek(fd, -total_seek, SEEK_CUR);
    return found;
}

// Reads string into buff from the current position
// till the target character. The target character is
// also included. File position is then at the next
// character after target. It will modify the buff
// reagardless of successful read or not.
//
// Returns:  true when successful
//          false when not succesful (also seeks back to original location)
bool read_until(int fd, char* buf, int buf_size, char target, bool include_target)
{
    // Memset should not be needed in theory but for some reason when printing it does not stop at
    // first NULL and does some more magic. So nullifying string first.
    memset(buf, 0, buf_size);

    char *orig_buf = buf;
    bool found = false;
    // Made it support larger chunks than 1 character but realized it is worse as when
    // buffer string cant fit any more characters we got to stop early..
    // And now this function looks kinda garbage. . .
    const uint16_t READ_CHUNK_SIZE = 1;
    int total_seek = 0;
    int target_length = 0;
    int buf_size_left = buf_size;
    while(!found){
        if (buf_size_left <= 0){
            break;
        }
        int rd_bytes = read(fd, buf, READ_CHUNK_SIZE);
#ifndef PC
        Debug_Printf(1,8, false, 0, "rd_byte: %d", rd_bytes);
        LCD_Refresh();
#endif
        // Check if end of file was reached
        if (rd_bytes <= 0)
            break;
        buf_size_left -= rd_bytes;
        // Go through buf to check if target was there
        // And seek to the target when found
        uint16_t idx = 0;
        while(buf[idx] != '\0'){
            if (buf[idx] == target){
                lseek(fd, -rd_bytes+idx+1, SEEK_CUR);
                found = true;
                break;
            }
            idx++;
            target_length++;
        }
        buf += rd_bytes;
        total_seek += rd_bytes;
    }
    if(found){
        if(include_target){
            target_length += 1;
        }
        orig_buf[target_length] = '\0';
    }
    else{
        lseek(fd, -total_seek, SEEK_CUR);
    }
    return found;
}

// Assumes Linefeed '\n' (LF) as the end of line character
// Use "dos2unix" on your file to turn them into LF when using files from windows.
// Windows default is Carriage + Linefeed \r\n (CRLF) which is 2 characters.
// TODO: Add another read_until that has target
//       string instead of single character.
bool read_line(int fd, char* buf, int buf_size)
{
    // Reads line till new line
    if(read_until(fd, buf, buf_size, '\n', false))
        return true;
    // Reached end (no more new lines). Just read the rest of the line normally
    int last_read = read(fd, buf, buf_size);
    #ifndef PC
        Debug_Printf(1,7, false, 0, "last_read: %d", last_read);
        LCD_Refresh();
    #endif
    if (last_read > 0){
        return true;
    }
    return false;
}