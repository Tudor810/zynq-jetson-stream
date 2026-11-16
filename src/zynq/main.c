/*
 * main.c - Zynq side, starting point.
 *
 * Step 1 (now): read the text file and the image from the SD card.
 * Later: connect to the Jetson, send both, receive results, display.
 *
 * Usage: zynq_client <image.ppm> <text.txt>
 */
#include <stdio.h>
#include <stdlib.h>

#include "../common/errors.h"
#include "fileio.h"

int main(int argc, char **argv)
{
    uint8_t *text = NULL, *image = NULL;
    size_t text_size = 0, image_size = 0;
    int rc, exit_code = EXIT_FAILURE;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <image.ppm> <text.txt>\n", argv[0]);
        return EXIT_FAILURE;
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
