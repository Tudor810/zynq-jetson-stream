#include "filters.h"
#include "common/image.h"
#include "common/errors.h"

#include <string.h>

/* Move a coordinate into 0 .. size-1 (nearest valid pixel). */
static int clamp_coord(int v, int size) {
    if (v < 0)
        return 0;
    if (v >= size)
        return size - 1;
    return v;
}

static const int gauss3[3][3] = {
    { 1, 2, 1 },
    { 2, 4, 2 },
    { 1, 2, 1 }
};

/* Weighted sum of the 3x3 block around (x, y), with clamped borders. */
static int convolve3x3(const image_t *in, int x, int y, const int k[3][3])
{
    int w = (int)in->width;
    int h = (int)in->height;
    int sum = 0;

    for (int dy = -1; dy <= 1; dy++) {
        int yy = clamp_coord(y + dy, h);
        for (int dx = -1; dx <= 1; dx++) {
            int xx = clamp_coord(x + dx, w);
            sum += k[dy + 1][dx + 1] * in->pixels[yy * w + xx];
        }
    }
    return sum;
}

int cpu_grayscale(const image_t *in, image_t *out) {
    if(in == NULL || out == NULL || in->pixels == NULL || 
        (in->channels != 1 && in->channels != 3)) 
        return ERR_ARGS;

    int rc = image_alloc(out, in->width, in->height, 1);
    if(rc != ERR_OK) 
        return rc;

    size_t npixels = image_size(out);

    if(in->channels == 1) {
        memcpy(out->pixels, in->pixels, npixels);
        return ERR_OK;
    } 

    for (size_t i = 0; i < npixels; i++) {
        uint8_t R = in->pixels[3 * i];
        uint8_t G = in->pixels[3 * i + 1];
        uint8_t B = in->pixels[3 * i + 2];

        out->pixels[i] = (uint8_t)((299 * R + 587 * G + 114 * B + 500) / 1000);
    }

    return ERR_OK;
}

int cpu_gaussian_blur(const image_t *in, image_t *out) {
    if(in == NULL || in->pixels == NULL || out == NULL || in->channels != 1) 
        return ERR_ARGS;

    int rc = image_alloc(out, in->width, in->height, 1);
    if(rc != ERR_OK) 
        return rc;

    int w = (int)in->width;
    int h = (int)in->height;

    for(int y = 0;y < h; y++) {
        for(int x = 0; x < w; x++) {
            out->pixels[y * w + x] = (uint8_t)((convolve3x3(in, x, y, gauss3) + 8) / 16);
        }
    }

    return ERR_OK;
}

// int cpu_sobel(const image_t *in, image_t *out) {

// }
