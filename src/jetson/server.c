#include "common/errors.h"
#include "common/netio.h"

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
int main(void) {
    int server_fd = -1;
    int client_fd = -1;
    int err = ERR_OK;
    const char *where = "server";   /* which step failed, for the error message */
    uint32_t len = 0;
    void *buf = NULL;

    printf("[server] starting...\n");

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0) {
        err = ERR_SOCKET;
        where = "socket";
        goto cleanup;
    }
    printf("[server] socket created\n");

    int opt = 1;
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        err = ERR_SOCKET;
        where = "setsockopt";
        goto cleanup;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family         = AF_INET;
    addr.sin_addr.s_addr    = htonl(INADDR_ANY);
    addr.sin_port           = htons(DEFAULT_PORT);

    if(bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        err = ERR_SOCKET;
        where = "bind";
        goto cleanup;
    }
    printf("[server] bound to port %d\n", DEFAULT_PORT);

    if(listen(server_fd, 1) < 0) {
        err = ERR_SOCKET;
        where = "listen";
        goto cleanup;
    }
    printf("[server] listening, waiting for a client...\n");

    struct sockaddr_in client_addr = {0};
    socklen_t client_len = sizeof(client_addr);
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if(client_fd < 0) {
        err = ERR_SOCKET;
        where = "accept";
        goto cleanup;
    }

    char client_ip[INET_ADDRSTRLEN] = "?";
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
    printf("[server] client connected from %s:%d\n",
           client_ip, ntohs(client_addr.sin_port));

    printf("[server] waiting for a message...\n");
    err = recv_message(client_fd, &buf, &len);
    if(err != ERR_OK) {
        where = "recv_message";
        goto cleanup;
    }

    uppercase_text(buf, len);

    printf("[server] received %u bytes:\n", len);
    printf("---------- text ----------\n");
    printf("%.*s\n", (int)len, (char *)buf);
    printf("--------------------------\n");

    err = send_message(client_fd, buf, len);
    if(err != ERR_OK) {
        where = "send_message";
        goto cleanup;
    }

cleanup:
    if(err != ERR_OK) err_print(where, err);   /* print before close() changes errno */

    if(client_fd >= 0) {
        close(client_fd);
        printf("[server] client connection closed\n");
    }
    if(server_fd >= 0) close(server_fd);
    free(buf);

    printf("[server] stopped (%s)\n", err == ERR_OK ? "ok" : "error");
    return err == ERR_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}