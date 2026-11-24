#include "common/errors.h"
#include "common/protocol.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>


static void uppercase_text(uint8_t* buf, uint32_t len) {
    for(uint32_t i = 0; i < len;i++) {
        if(buf[i] >= 'a' && buf[i] <= 'z') {
            buf[i] = buf[i] - 'a' + 'A';
        }
    }
}

static void handle_client(int client_fd) {
    void *buf = NULL;
    uint32_t len = 0;
    int rc;

    while(1) {
        printf("[server] waiting for a message...\n");

        errno = 0;
        rc = recv_message(client_fd, &buf, &len);
        if(rc == ERR_CLOSED) {
            printf("[server] client disconnected\n");
            break;
        }

        if(rc != ERR_OK) {
            err_print("recv_message", rc);
            break;
        }

        printf("[server] received %u bytes:\n", len);
        // printf("---------- text ----------\n");
        // printf("%.*s\n", (int)len, (char *)buf);
        // printf("--------------------------\n");

        uppercase_text(buf, len);

        rc = send_message(client_fd, buf, len);
        free(buf);
        buf = NULL;

        if(rc != ERR_OK) {
            err_print("send_message", rc);
            break;
        }
        printf("[server] sent %u bytes back\n", len);
    }
}

int main(void) {
    int server_fd = -1;
    int err = ERR_OK;
    const char *where = "server";   /* which setup step failed */

    printf("[server] starting...\n");

     /* ---- 1. setup ---- */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0) {
        err = ERR_SOCKET;
        where = "socket";
        goto cleanup;
    }

    int opt = 1;
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        err = ERR_SOCKET;
        where = "setsockopt";
        goto cleanup;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family         = AF_INET;
    addr.sin_addr.s_addr    = htonl(INADDR_ANY);
    addr.sin_port           = htons(PROTO_DEFAULT_PORT);

    if(bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        err = ERR_SOCKET;
        where = "bind";
        goto cleanup;
    }

    if(listen(server_fd, 1) < 0) {
        err = ERR_SOCKET;
        where = "listen";
        goto cleanup;
    }
    printf("[server] listening on port %d (Ctrl+C to stop)\n", PROTO_DEFAULT_PORT);

    /* ---- 2. accept clients ---- */

    while(1) {
        struct sockaddr_in client_addr = {0};
        socklen_t client_len = sizeof(client_addr);

        printf("[server] waiting for a client...\n");

        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if(client_fd < 0) {
            err_print("accept", ERR_SOCKET);
            continue;
        }

        char client_ip[INET_ADDRSTRLEN] = "?";
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        printf("[server] client connected from %s:%d\n",
            client_ip, ntohs(client_addr.sin_port));

        handle_client(client_fd);

        close(client_fd);
        printf("[server] connection with %s closed\n\n", client_ip);
    }
    
cleanup:
    if(err != ERR_OK) err_print(where, err);   /* print before close() changes errno */
    if(server_fd >= 0) close(server_fd);

    printf("[server] stopped (%s)\n", err == ERR_OK ? "ok" : "error");
    return err == ERR_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}