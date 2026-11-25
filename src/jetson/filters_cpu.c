#include "filters.h"
#include "common/image.h"
#include "common/errors.h"

#include <string.h>

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