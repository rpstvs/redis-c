#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#define TRUE = true;

static void do_something(int connfd);

static void die(const char *msg)
{
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

static void msg(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
}

void main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    int val = 1;

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    struct sockaddr_in addr = {};

    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(0);

    int rv = bind(fd, (struct sockaddr *)&addr, sizeof(addr));

    if (rv)
    {
        die("bind()");
    }

    rv = listen(fd, SOMAXCONN);

    if (rv)
    {
        die("listen()");
    }

    while (true)
    {
        struct sockaddr_in client_addr = {};
        socklen_t socklen = sizeof(client_addr);

        int connfd = accept(fd, (struct sockaddr *)&client_addr, socklen);

        if (connfd < 0)
        {
            continue;
        }

        do_something(connfd);
        close(connfd);
    }

    while (true)
    {
        int32_t err = one_request(connfd);

        if (err)
        {
            break;
        }
    }
    return 0;
}

static void do_something(int connfd)
{
    char rbuf[64] = {};

    ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);

    if (n < 0)
    {
        msg("read() error");
        return;
    }

    printf("client says: %s\n", rbuf);

    char wbuf[] = "world";
    write(connfd, wbuf, strlen(wbuf));
}

static int32_t read_full(int fd, char *buff, size_t n)
{
    while (n > 0)
    {
        ssize_t rv = read(fd, &buff, n);

        if (rv <= 0)
        {
            return -1;
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buff += rv;
    }
    return 0;
}

static int32_t write_all(int fd, char *buff, size_t n)
{
    while (n > 0)
    {
        int rv = write(fd, buff, n);

        if (rv <= 0)
        {
            return -1;
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buff += rv;
    }
    return 0;
}

const size_t k_max_msg = 4096;
const size_t header_size = 4;

static int32_t one_request(int connfd)
{
    char rbuf[header_size + k_max_msg + 1];

    errno = 0;

    int32_t err = read_full(connfd, rbuf, 4);

    if (err)
    {
        if (errno == 0)
        {
            msg("EOF");
        }
        else
        {
            msg("read() error");
        }
        return err;
    }

    uint32_t len = 0;

    memcpy(&len, rbuf, 4);
    if (len > k_max_msg)
    {
        msg("too long");
        return -1;
    }

    err = read_full(connfd, &rbuf[4], len);

    if (err)
    {
        msg("read() error");
        return err;
    }

    rbuf[4 + len] = '\0';

    printf("client says: %s\n", &rbuf[4]);

    const char reply[] = "world";

    char wbuf[4 + sizeof(reply)];

    len = (uint32_t)strlen(reply);

    memccpy(wbuf, &len, 4);
    memccpy(&wbuf[4], reply, len);
    return write_all(connfd, wbuf, 4 + len);
}