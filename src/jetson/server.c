#include "common/errors.h"
#include "common/protocol.h"
#include "common/image.h"
#include "common/timing.h"
#include "filters.h"

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
            buf[i] = (uint8_t)(buf[i] - 'a' + 'A');
        }
    }
}

static int send_error(int client_fd, const char *context, int code) {
    msg_header_t info = {.type = MSG_ERROR};

    char err[128];
    snprintf(err, sizeof(err), "%s: %s", context, err_str(code));

    errno = 0;
    int rc = proto_send(client_fd, &info, err, (uint32_t)strlen(err));
    if(rc != ERR_OK) {
        err_print("proto_send (ERROR)", rc);
        return rc;
    }

    printf("[server] sent ERROR: %s\n", err);

    return ERR_OK;
}

static int handle_text(int client_fd, const msg_header_t *hdr, uint8_t *buf) {
    printf("[server] received TEXT: %u bytes\n", hdr->payload_size);

    
    uint64_t t0 = time_now_us();
    uppercase_text(buf, hdr->payload_size);
    uint32_t elapsed_us = (uint32_t)(time_now_us() - t0);

    msg_header_t info = {
        .type           = MSG_TEXT_RESULT, 
        .server_time_us = elapsed_us, 
    };

    errno = 0;
    int rc = proto_send(client_fd, &info, buf, hdr->payload_size);
    if(rc != ERR_OK) {
        err_print("proto_send (TEXT_RESULT)", rc);
        return rc;
    }

    printf("[server] sent TEXT_RESULT: %u bytes (processing %u us)\n",
       hdr->payload_size, elapsed_us);

    return ERR_OK;
}

static int handle_image(int client_fd, const msg_header_t *hdr, const uint8_t *buf) {
    printf("[server] received IMAGE: %ux%u, %u channel(s), %u bytes\n",
       hdr->width, hdr->height, hdr->channels, hdr->payload_size);
    

    image_t image = {
        .height   = hdr->height, 
        .width    = hdr->width, 
        .channels = hdr->channels, 
        .pixels   = (uint8_t *)buf
    };
    image_t result = {0};
    uint64_t t0 = time_now_us();
    errno = 0;
    int rc = cpu_grayscale(&image, &result);
    uint32_t elapsed_us = (uint32_t)(time_now_us() - t0);

    if (rc != ERR_OK) {
        err_print("cpu_grayscale", rc);
        return send_error(client_fd, "grayscale failed", rc);
    }

    msg_header_t info = {
        .type           = MSG_IMAGE_RESULT,
        .width          = result.width,
        .height         = result.height,
        .channels       = result.channels,
        .server_time_us = elapsed_us 
    };
    
    errno = 0;
    rc = proto_send(client_fd, &info, result.pixels, (uint32_t)image_size(&result));
    image_free(&result);
    if(rc != ERR_OK) {
        err_print("proto_send (IMAGE_RESULT)", rc);
        return rc;
    }

    printf("[server] sent IMAGE_RESULT: %ux%u, %u channel(s) (processing %u us)\n",
       info.width, info.height, info.channels, elapsed_us);

    return ERR_OK;
}

static void handle_client(int client_fd) {
    uint8_t *buf = NULL;
    msg_header_t hdr = {0};
    int rc;

    while(1) {
        printf("[server] waiting for a message...\n");

        errno = 0;
        rc = proto_recv(client_fd, &hdr, &buf);
        if(rc == ERR_CLOSED) {
            printf("[server] client disconnected\n");
            break;
        }

        if(rc != ERR_OK) {
            err_print("proto_recv", rc);
            break;
        }

        switch(hdr.type) {
            case MSG_TEXT:
                rc = handle_text(client_fd, &hdr, buf);
                break;   
            case MSG_IMAGE: 
                rc = handle_image(client_fd, &hdr, buf);
                break;
            default:
                rc = send_error(client_fd, "unexpected message type", ERR_PROTO_TYPE);
                break;
        }

        free(buf);
        buf = NULL;

        if(rc != ERR_OK) 
            break;
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