/*
 * image.h - in-memory image and PPM (P6, color) / PGM (P5, gray) I/O.
 *
 * Pixels are stored row by row, top to bottom, with `channels` bytes per
 * pixel (3 = R,G,B for P6, 1 = gray for P5). Only 8-bit images (maxval 255).
 *
 * Ownership: every function that fills an image_t allocates img->pixels;
 * the caller releases it with image_free().
 */
#ifndef IMAGE_H
#define IMAGE_H

#include <stddef.h>
#include <stdint.h>

#define IMAGE_MAX_SIDE 8192


typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t channels;   /* 1 (gray) or 3 (RGB) */
    uint8_t *pixels;     /* width * height * channels bytes, malloc'ed */
} image_t;

/* Number of pixel bytes: width * height * channels. */
size_t image_size(const image_t *img);

/*
 * Img must not own pixels; call image_free first
 * Allocate an empty image (pixels not initialized).
 * Returns ERR_OK, ERR_ARGS (bad size or channels) or ERR_NOMEM.
 */
int image_alloc(image_t *img, uint32_t width, uint32_t height, uint32_t channels);

/* Free the pixel buffer and reset the struct to zero. Safe to call twice. */
void image_free(image_t *img);

/*
 * Parse a P5/P6 image that is already in memory (e.g. from file_read_all).
 * On success img is filled and owns a copy of the pixels.
 * Returns ERR_OK, ERR_ARGS, ERR_FORMAT, ERR_UNSUPPORTED or ERR_NOMEM.
 */
int image_parse_ppm(const uint8_t *data, size_t size, image_t *img);

/*
 * Read the file at path and parse it (file_read_all + image_parse_ppm).
 * Returns the same codes as image_parse_ppm, plus the file read errors.
 */
int image_load_ppm(const char *path, image_t *img);

/*
 * Save as P6 (channels == 3) or P5 (channels == 1).
 * Returns ERR_OK, ERR_ARGS, ERR_NOMEM or the file write errors.
 */
int image_save_ppm(const char *path, const image_t *img);

#endif /* IMAGE_H */