#include "common/errors.h"
#include "common/netio.h"
#include "fileio.h"

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
    struct sockaddr_in addr = {};
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
    uint8_t *text = NULL, *image = NULL;
    size_t text_size = 0, image_size = 0;
    void* reply;
    uint32_t reply_len = 0;
    int server_fd = -1;
    int rc, exit_code = EXIT_FAILURE;

    if (argc != 5) {
        fprintf(stderr, "usage: %s <image.ppm> <text.txt> <server ip> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    struct in_addr server_ip;
    uint16_t server_port;
    
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
    rc = file_read_all(argv[2], &text, &text_size);
    if (rc != ERR_OK) { err_print(argv[2], rc); goto cleanup; }

    rc = file_read_all(argv[1], &image, &image_size);
    if (rc != ERR_OK) { err_print(argv[1], rc); goto cleanup; }

    printf("[client] text: %zu bytes, image file: %zu bytes\n", text_size, image_size);
    
    if(text_size == 0 || text_size > NETIO_MAX_PAYLOAD) {
        fprintf(stderr, "[client] text file must be 1 byte to %u bytes\n", NETIO_MAX_PAYLOAD);
        goto cleanup;
    }

    /* TODO: parse the PPM header (before connecting). */

    /* ---- 3. connect ---- */
    printf("[client] connecting to %s:%u...\n", argv[3], (unsigned)server_port);
    errno = 0;
    rc = connect_to_server(server_ip, server_port, &server_fd);
    if(rc != ERR_OK) { 
        err_print("connect", rc); 
        goto cleanup; 
    }

    /* ---- 4. send the text ---- */

    rc = send_message(server_fd, text, (uint32_t)text_size);
    if(rc != ERR_OK) {
        err_print("send_message", rc);
        goto cleanup;
    }

    /* ---- 5. receive the reply ---- */
    errno = 0;
    rc = recv_message(server_fd, &reply, &reply_len);
    if(rc != ERR_OK) {
        err_print("recv_message", rc);
        goto cleanup;
    }

    printf("[client] received %u bytes:\n", reply_len);
    printf("---------- reply ----------\n");
    printf("%.*s\n", (int)reply_len, (char *)reply);
    printf("---------------------------\n");


    /* TODO: send the image, receive the processed image, display it. */


    exit_code = EXIT_SUCCESS;

cleanup:

    if(server_fd >= 0) {
        close(server_fd);
        printf("[client] connection closed\n");
    }
    free(reply);
    free(text);
    free(image);
    return exit_code;
}
