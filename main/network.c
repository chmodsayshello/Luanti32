#include "network.h"

#ifndef IPV6
#define LWIP_IPV4 1
#else
#define LWIP_IPV6 1
#endif

#include <lwip/sockets.h>
#include <netdb.h>

bool n_connect(char* address, uint16_t port, conn_fd* fd) {
#ifndef IPV6
    *fd = socket(AF_INET, SOCK_DGRAM, 0);
    
    struct sockaddr_in i_address;
    struct hostent *host = gethostbyname(address);
    if (host == NULL) {
        return false;
    }

    i_address.sin_addr.s_addr = *(long *) host->h_addr_list[0];
    i_address.sin_port = htons(port);
    i_address.sin_family = AF_INET;

    return connect(*fd, (struct sockaddr *)&i_address, sizeof(i_address)) <= 0;
#else
    return false;
#endif
}

void n_disconnect(conn_fd fd) {
    close(fd);
}

void n_send(void* ptr, size_t size, conn_fd fd) {
#ifndef IPV6
    send(fd, ptr, size, 0); 
#endif
}

size_t n_getsize(conn_fd fd) {
    struct pollfd fds;
    fds.fd = fd;
    
    ssize_t bytes = poll(&fds, 1, 0);

    if (bytes < 0) {
        return 0;
    }
    return (size_t) bytes;
}

size_t n_read(void* dest, size_t max, conn_fd fd) {
    return recvfrom(fd, dest, max, 0, NULL, NULL);
}

size_t n_read_peek(void* dest, size_t max, conn_fd fd) {
    return recvfrom(fd, dest, max, MSG_PEEK, NULL, NULL);
}