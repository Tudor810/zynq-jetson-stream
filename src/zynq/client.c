#include "common/errors.h"
#include "common/fileio.h"
#include "common/protocol.h"
#include "common/image.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>
#include <sys/socket.h>

/* Parse a port number (1-65535). Returns ERR_OK or ERR_ARGS. */
static int parse_port(const char *text, uint16_t *port)
{
    char *end = NULL;
    long value;

    errno = 0;
    value = strtol(text, &end, 10);

    if (errno != 0 || end == text || *end != '\0' || value < 1 || value > 65535)
        return ERR_ARGS;

    *port = (uint16_t)value;
    return ERR_OK;
}

/* Parse an IPv4 address like "192.168.10.2". Returns ERR_OK or ERR_ADDRESS. */
static int parse_ip(const char *text, struct in_addr *ip)
{
    if (inet_pton(AF_INET, text, ip) != 1)
        return ERR_ADDRESS;
    return ERR_OK;
}

static int connect_to_server(struct in_addr ip, uint16_t port, int *fd) {
    struct sockaddr_in addr = {0};
    int s = socket(AF_INET, SOCK_STREAM, 0);

    if(s < 0) {
        return ERR_SOCKET;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr   = ip;
    addr.sin_port   = htons(port);

    if(connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        int saved = errno;
        close(s);
        errno = saved;
        return ERR_CONNECT;
    }

    *fd = s;
    return ERR_OK;
}

int main(int argc, char **argv)
{   
    msg_header_t info = {0}, hdr = {0};
    uint8_t *text = NULL;
    image_t image = {0}, result = {0};
    size_t text_size = 0;
    uint8_t* reply = NULL;
    int server_fd = -1;
    int rc, exit_code = EXIT_FAILURE;
    struct in_addr server_ip;
    uint16_t server_port;

    if (argc != 5) {
        fprintf(stderr, "usage: %s <image.ppm> <text.txt> <server ip> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }


    
    /* ---- 1. check the arguments ---- */
    rc = parse_ip(argv[3], &server_ip);
    if (rc != ERR_OK) {
        fprintf(stderr, "invalid server IP: '%s'\n", argv[3]);
        goto cleanup;
    }

    rc = parse_port(argv[4], &server_port);
    if (rc != ERR_OK) {
        fprintf(stderr, "invalid port: '%s' (must be 1-65535)\n", argv[4]);
        goto cleanup;
    }

    /* ---- 2. read the files ---- */
    errno = 0;
    rc = file_read_all(argv[2], &text, &text_size);
    if (rc != ERR_OK) { err_print(argv[2], rc); goto cleanup; }

    errno = 0;
    rc = image_load_ppm(argv[1], &image);
    if (rc != ERR_OK) { err_print(argv[1], rc); goto cleanup; }

    printf("[client] text: %zu bytes, image: %ux%u, %u channel(s)\n", text_size, image.width, image.height, image.channels);
    
    if(text_size == 0 || text_size > PROTO_MAX_PAYLOAD) {
        fprintf(stderr, "[client] text file must be 1 byte to %u bytes\n", PROTO_MAX_PAYLOAD);
        goto cleanup;
    }


    /* ---- 3. connect ---- */
    printf("[client] connecting to %s:%u...\n", argv[3], (unsigned)server_port);
    errno = 0;
    rc = connect_to_server(server_ip, server_port, &server_fd);
    if(rc != ERR_OK) { 
        err_print("connect", rc); 
        goto cleanup; 
    }

    // /* ---- 4. send the text ---- */

    info = (msg_header_t){.type = MSG_TEXT};
    errno = 0;
    rc = proto_send(server_fd, &info, text, (uint32_t)text_size);
    if(rc != ERR_OK) {
        err_print("proto_send", rc);
        goto cleanup;
    }

    // /* ---- 5. receive the reply ---- */

    errno = 0;
    rc = proto_recv(server_fd, &hdr, &reply);
    if(rc != ERR_OK) {
        err_print("proto_recv", rc);
        goto cleanup;
    }

    if (hdr.type == MSG_ERROR) {
        fprintf(stderr, "[client] server error: %.*s\n",
                (int)hdr.payload_size, (char *)reply);
        rc = ERR_REMOTE;
        goto cleanup;
    }
    if (hdr.type != MSG_TEXT_RESULT) {
        fprintf(stderr, "[client] expected %s, got %s\n",
                proto_type_name(MSG_TEXT_RESULT), proto_type_name(hdr.type));
        rc = ERR_PROTO_TYPE;
        goto cleanup;
    }
    printf("[client] received %u bytes (server time %u us):\n", hdr.payload_size, hdr.server_time_us);
    printf("---------- reply ----------\n");
    printf("%.*s\n", (int)hdr.payload_size, (char *)reply);
    printf("---------------------------\n");

    free(reply);
    reply = NULL;

    /* ---- 6. send the image ---- */
    info = (msg_header_t){
        .type     = MSG_IMAGE, 
        .height   = image.height,
        .width    = image.width,
        .channels = image.channels
    };
    errno = 0;
    rc = proto_send(server_fd, &info, image.pixels, (uint32_t)image_size(&image));
    if(rc != ERR_OK) {
        err_print("proto_send_image", rc);
        goto cleanup;
    }
    
    /* ---- 7. receive the updated image ---- */
    errno = 0;
    rc = proto_recv(server_fd, &hdr, &reply);
    if(rc != ERR_OK) {
        err_print("proto_recv_image", rc);
        goto cleanup;
    }

    if (hdr.type == MSG_ERROR) {
        fprintf(stderr, "[client] server error: %.*s\n",
                (int)hdr.payload_size, (char *)reply);
        rc = ERR_REMOTE;
        goto cleanup;
    }
    if (hdr.type != MSG_IMAGE_RESULT) {
        fprintf(stderr, "[client] expected %s, got %s\n",
                proto_type_name(MSG_IMAGE_RESULT), proto_type_name(hdr.type));
        rc = ERR_PROTO_TYPE;
        goto cleanup;
    }

    printf("[client] received %s: %ux%u, %u channel(s), %u bytes (server time %u us)\n",
       proto_type_name(hdr.type), hdr.width, hdr.height, hdr.channels,
       hdr.payload_size, hdr.server_time_us);

    /* ---- 8. display the image (save it to a new file for now) ---- */
    result.width    = hdr.width;
    result.height   = hdr.height;
    result.channels = hdr.channels;
    result.pixels   = reply;
    reply = NULL;

    rc = image_save_ppm("results/output.ppm", &result);
    if(rc != ERR_OK) {
        err_print("image_save_ppm", rc);
        goto cleanup;
    }

    printf("[client] result saved to results/output.ppm\n");
    exit_code = EXIT_SUCCESS;

cleanup:

    if(server_fd >= 0) {
        close(server_fd);
        printf("[client] connection closed\n");
    }
    free(reply);
    free(text);
    image_free(&image);
    image_free(&result);
    return exit_code;
}
