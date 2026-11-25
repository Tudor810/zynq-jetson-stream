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

#define OUTPUT_PATH_IMAGE "results/output.ppm"   /* placeholder until HDMI works */
#define OUTPUT_PATH_TEXT  "results/output.txt" 

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

/* Create a TCP socket and connect it to ip:port. On success *fd is the socket. */
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

/*
 * Receive one reply and check that it has the expected type.
 * On success *payload is a malloc'ed buffer (caller frees).
 * A MSG_ERROR reply is printed and returned as ERR_REMOTE.
 */
static int recv_reply(int fd, msg_type_t expected, msg_header_t *hdr, uint8_t **payload)
{
    errno = 0;
    int rc = proto_recv(fd, hdr, payload);
    if (rc != ERR_OK) {
        err_print("proto_recv", rc);
        return rc;
    }

    if (hdr->type == MSG_ERROR) {
        fprintf(stderr, "[client] server error: %.*s\n",
                (int)hdr->payload_size, (char *)*payload);
        rc = ERR_REMOTE;
    } else if (hdr->type != expected) {
        fprintf(stderr, "[client] expected %s, got %s\n",
                proto_type_name((uint16_t)expected), proto_type_name(hdr->type));
        rc = ERR_PROTO_TYPE;
    }

    if (rc != ERR_OK) {
        free(*payload);
        *payload = NULL;
    }
    return rc;
}

/* Send the text, receive the uppercase version and print it. */
static int exchange_text(int fd, const uint8_t *text, size_t size)
{
    msg_header_t info = { .type = MSG_TEXT };
    msg_header_t hdr = {0};
    uint8_t *reply = NULL;
    int rc;

    errno = 0;
    rc = proto_send(fd, &info, text, (uint32_t)size);
    if (rc != ERR_OK) {
        err_print("proto_send (text)", rc);
        return rc;
    }
    printf("[client] sent text: %zu bytes\n", size);

    rc = recv_reply(fd, MSG_TEXT_RESULT, &hdr, &reply);
    if (rc != ERR_OK)
        return rc;

    printf("[client] received %u bytes (server time %u us):\n",
           hdr.payload_size, hdr.server_time_us);
    // printf("---------- reply ----------\n");
    // printf("%.*s\n", (int)hdr.payload_size, (char *)reply);
    // printf("---------------------------\n");

    rc = file_write_all(OUTPUT_PATH_TEXT, reply, (size_t)hdr.payload_size);
    if (rc != ERR_OK) { 
        err_print(OUTPUT_PATH_TEXT, rc); 
        free(reply);
        return rc;
    }

    printf("[client] result saved to %s\n", OUTPUT_PATH_TEXT);
    free(reply);
    return ERR_OK;
}

/*
 * Send the image and receive the processed one.
 * On success *result owns the received pixels (free with image_free).
 */
static int exchange_image(int fd, const image_t *image, image_t *result)
{
    msg_header_t info = {
        .type     = MSG_IMAGE,
        .width    = image->width,
        .height   = image->height,
        .channels = image->channels
    };
    msg_header_t hdr = {0};
    uint8_t *reply = NULL;
    int rc;

    errno = 0;
    rc = proto_send(fd, &info, image->pixels, (uint32_t)image_size(image));
    if (rc != ERR_OK) {
        err_print("proto_send (image)", rc);
        return rc;
    }
    printf("[client] sent image: %ux%u, %u channel(s)\n",
           image->width, image->height, image->channels);

    rc = recv_reply(fd, MSG_IMAGE_RESULT, &hdr, &reply);
    if (rc != ERR_OK)
        return rc;

    printf("[client] received %s: %ux%u, %u channel(s), %u bytes (server time %u us)\n",
           proto_type_name(hdr.type), hdr.width, hdr.height, hdr.channels,
           hdr.payload_size, hdr.server_time_us);

    /* the received buffer becomes the result's pixels (no copy) */
    result->width    = hdr.width;
    result->height   = hdr.height;
    result->channels = hdr.channels;
    result->pixels   = reply;
    return ERR_OK;
}

int main(int argc, char **argv)
{
    uint8_t *text = NULL;
    size_t text_size = 0;
    image_t image = {0}, result = {0};
    struct in_addr server_ip;
    uint16_t server_port = 0;
    int server_fd = -1;
    int rc, exit_code = EXIT_FAILURE;

    if (argc != 5) {
        fprintf(stderr, "usage: %s <image.ppm> <text.txt> <server ip> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* ---- 1. check the arguments ---- */
    if (parse_ip(argv[3], &server_ip) != ERR_OK) {
        fprintf(stderr, "invalid server IP: '%s'\n", argv[3]);
        goto cleanup;
    }
    if (parse_port(argv[4], &server_port) != ERR_OK) {
        fprintf(stderr, "invalid port: '%s' (must be 1-65535)\n", argv[4]);
        goto cleanup;
    }

    /* ---- 2. load the inputs ---- */
    errno = 0;
    rc = file_read_all(argv[2], &text, &text_size);
    if (rc != ERR_OK) { err_print(argv[2], rc); goto cleanup; }

    errno = 0;
    rc = image_load_ppm(argv[1], &image);
    if (rc != ERR_OK) { err_print(argv[1], rc); goto cleanup; }

    printf("[client] text: %zu bytes, image: %ux%u, %u channel(s)\n",
           text_size, image.width, image.height, image.channels);

    if (text_size == 0 || text_size > PROTO_MAX_PAYLOAD) {
        fprintf(stderr, "[client] text file must be 1 byte to %u bytes\n", PROTO_MAX_PAYLOAD);
        goto cleanup;
    }

    /* ---- 3. connect ---- */
    printf("[client] connecting to %s:%u...\n", argv[3], (unsigned)server_port);
    errno = 0;
    rc = connect_to_server(server_ip, server_port, &server_fd);
    if (rc != ERR_OK) { err_print("connect", rc); goto cleanup; }
    printf("[client] connected\n");

    /* ---- 4. text round trip ---- */
    if (exchange_text(server_fd, text, text_size) != ERR_OK)
        goto cleanup;

    /* ---- 5. image round trip ---- */
    if (exchange_image(server_fd, &image, &result) != ERR_OK)
        goto cleanup;

    /* ---- 6. show the result (saved to a file until HDMI works) ---- */
    errno = 0;
    rc = image_save_ppm(OUTPUT_PATH_IMAGE, &result);
    if (rc != ERR_OK) { err_print(OUTPUT_PATH_IMAGE, rc); goto cleanup; }
    printf("[client] result saved to %s\n", OUTPUT_PATH_IMAGE);

    exit_code = EXIT_SUCCESS;

cleanup:
    if (server_fd >= 0) {
        close(server_fd);
        printf("[client] connection closed\n");
    }
    free(text);
    image_free(&image);
    image_free(&result);
    return exit_code;
}