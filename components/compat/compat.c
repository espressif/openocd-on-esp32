#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pwd.h>

int chdir(const char *path)
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

pid_t tcgetpgrp(int fd)
{
    errno = ENOTTY;
    return -1;
}

sighandler_t signal(int signum, sighandler_t handler)
{
    if (handler == SIG_IGN) {
        return SIG_IGN;
    }
    errno = ENOSYS;
    return SIG_ERR;
}

int dup(int oldfd)
{
    errno = ENOSYS;
    return -1;
}

uid_t getuid(void)
{
    return 0;
}

struct passwd *getpwuid(uid_t uid)
{
    return NULL;
}

int getnameinfo(const struct sockaddr *addr, socklen_t addrlen,
                char *host, socklen_t hostlen,
                char *serv, socklen_t servlen, int flags)
{
    return EAI_FAIL;
}
