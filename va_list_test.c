#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

int ctfs_fcntl(int fd, int cmd, ...)
{
    va_list ap;
    va_start(ap, cmd); 
    printf("Normal Paras: %d %d\n", fd, cmd);
    printf("Received: %d\n", va_arg(ap, int));  // retuens a parameter for each call
    printf("Received: %d\n", va_arg(ap, int));
    printf("Received: %d\n", va_arg(ap, int));
    printf("Received: %d\n", va_arg(ap, int));
    va_end(ap);
    return 0;
}

int main(int argc, char **argv)
{
    ctfs_fcntl(10, 1, 2, 3, 4);
    return 0;
}