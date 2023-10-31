#pragma once

// Does not require NULL termination
int custom_atoi(char* str);

// Returns false could not find (Also seeks back to original location)
// Returns true if could find target
bool seek_next_char(int fd, char target);

// Reads string into buff from the current position
// till the target character. The target character is
// also included. File position is then at the next
// character after target. It WILL modify the buff
// even if it was not successful.
// Returns: true when successful
//          false when not succesful (also seeks back to original location)
bool read_until(int fd, char* buf, int buf_size, char target, bool include_target);

// Assumes Linefeed '\n' (LF) as the end of line character
// Use "dos2unix" on your file to turn them into LF when using files from windows.
// Windows default is Carriage + Linefeed \r\n (CRLF) which is 2 characters.
// TODO: Add another read_until that has target
//       string instead of single character.
bool read_line(int fd, char* buf, int buf_size);
