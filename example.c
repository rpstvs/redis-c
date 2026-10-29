#include <stdio.h>
#include <sys/socket.h>

void main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    int val = 1;

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    struct sockaddr_in addr = {};
}