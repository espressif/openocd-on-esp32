#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

       
int chdir(const char* path)
{
    errno = ENOSYS;
    return -1;
}

char *getcwd(char *buf, size_t size)
{
    snprintf(buf, size, "/");
    buf[size - 1] = 0;
    return buf; 
}

mode_t umask(mode_t mask)
{
    static mode_t old_mask = S_IWGRP | S_IWOTH;
    mode_t ret = old_mask;
    old_mask = mask;
    return ret;
}
