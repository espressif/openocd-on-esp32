#pragma once

#include_next "sys/termios.h"

struct winsize {
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel;   /* unused */
    unsigned short ws_ypixel;   /* unused */
};

#define TIOCGWINSZ (('T' << 8) | 1)
#define TIOCSWINSZ (('T' << 8) | 2)
#define TIOCLINUX  (('T' << 8) | 3)
#define TIOCGPGRP  (('T' << 8) | 0xf)
#define TIOCSPGRP  (('T' << 8) | 0x10)
#define CRTSCTS  0x08000
