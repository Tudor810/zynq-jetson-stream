#include "errors.h"
#include "netio.h"

#include <sys/socket.h>
#include <errno.h>
#include <stdlib.h>
#include <arpa/inet.h>

int sendn(int fd, const void *buf, size_t n) {
    size_t sent      = 0 ;
    const uint8_t* p = (const uint8_t*)buf;

    while(sent < n) {
        ssize_t s = send(fd, p + sent, n - sent, MSG_NOSIGNAL);
        if(s < 0) {
            if(errno == EINTR) 
                continue;
            return ERR_SEND;
        }

        sent += (size_t)s;
    }

    return ERR_OK;
}


int recvn(int fd, void *buf, size_t n) {
    size_t received = 0;
    uint8_t *p      = (uint8_t*)buf;
    
    while(received < n) {
        ssize_t r = recv(fd, p + received, n - received, 0);
        if(r < 0) {
            if(errno == EINTR) 
                continue;
            return ERR_RECV;
        }

        if(r == 0) return ERR_CLOSED;
        received += (size_t)r;
    }

    return ERR_OK;
}


