#include "common/errors.h"
#include "fileio.h"

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>

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

int main(int argc, char **argv)
{
    uint8_t *text = NULL, *image = NULL;
    size_t text_size = 0, image_size = 0;
    int rc, exit_code = EXIT_FAILURE;

    if (argc != 5) {
        fprintf(stderr, "usage: %s <image.ppm> <text.txt> <server ip> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    struct in_addr server_ip;
    uint16_t server_port;

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

    rc = file_read_all(argv[2], &text, &text_size);
    if (rc != ERR_OK) { err_print(argv[2], rc); goto cleanup; }

    rc = file_read_all(argv[1], &image, &image_size);
    if (rc != ERR_OK) { err_print(argv[1], rc); goto cleanup; }

    printf("text: %zu bytes, image file: %zu bytes\n", text_size, image_size);

    printf("%s\n", text);
    /* TODO: parse the PPM header, connect to the Jetson, send, receive. */

    exit_code = EXIT_SUCCESS;

cleanup:
    free(text);
    free(image);
    return exit_code;
}
