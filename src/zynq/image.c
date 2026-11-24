#include "image.h"
#include "fileio.h"
#include "common/errors.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* True for the whitespace characters allowed in a PPM header. */
static int is_space(uint8_t c)
{
    return c == ' '  || c == '\t' || c == '\n' ||
           c == '\r' || c == '\v' || c == '\f';
}

static int is_digit(uint8_t c) {
    return c >= '0' && c <= '9';
} 
static void skip_space(const uint8_t *data, size_t size, size_t *pos) {
    while(*pos < size) {
        if(is_space(data[*pos])) {
            *pos += 1;
        } else if(data[*pos] == '#') {
            while(*pos < size && data[*pos] != '\n') {
                *pos += 1;
            }
        } else {
            break;
        }
    }
}

static int read_number(const uint8_t *data, size_t size, size_t *pos, uint32_t *value) {
    skip_space(data, size, pos);
    uint32_t rez = 0;
    int digits = 0;
    while(*pos < size && is_digit(data[*pos])) {
        if(rez > PPM_NUMBER_LIMIT) {
            return ERR_FORMAT;
        }
        rez = rez * 10 + (uint32_t)(data[*pos] - '0');
        *pos += 1;
        digits += 1;
    }

    if(digits == 0) {
        return ERR_FORMAT;
    }

    *value = rez;
    return ERR_OK;
}

size_t image_size(const image_t *img) {
    if(img == NULL) return 0;

    return (size_t)img->height * img->width * img->channels;
}

int image_alloc(image_t *img, uint32_t width, uint32_t height, uint32_t channels) {
    if(img == NULL) return ERR_ARGS;
    
    *img = (image_t){0};
    if(width == 0 || width > IMAGE_MAX_SIDE || height == 0 || height > IMAGE_MAX_SIDE || (channels != 1 && channels != 3)) {
        return ERR_ARGS;
    }

    img->width    = width;
    img->height   = height;
    img->channels = channels;

    size_t size = image_size(img);
    img->pixels = malloc(size);

    if(img->pixels == NULL) {
        *img = (image_t){0};
        return ERR_NOMEM;
    }

    return ERR_OK;
}

void image_free(image_t *img) {
    if(img == NULL) 
        return;

    free(img->pixels);
    *img = (image_t){0};
}


int image_parse_ppm(const uint8_t *data, size_t size, image_t *img) {
    if(data == NULL || img == NULL) {
        return ERR_ARGS;
    }

    *img = (image_t){0};
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t channels = 0;
    uint32_t maxval = 0;
    size_t pos = 0;
    int rc = 0;

    if(size < 2) {
        return ERR_FORMAT;
    }

    if(data[0] != 'P') {
        return ERR_FORMAT;
    }

    switch (data[1]) {
    case '6':
        channels = 3;
        break;
    case '5':
        channels = 1;
        break;
    case '1':
    case '2':
    case '3':
    case '4':
        return ERR_UNSUPPORTED;
    default:
        return ERR_FORMAT;
    }

    pos = 2;

    if(pos >= size || !is_space(data[pos])) {
        return ERR_FORMAT;
    }

    if ((rc = read_number(data, size, &pos, &width))  != ERR_OK ||
        (rc = read_number(data, size, &pos, &height)) != ERR_OK ||
        (rc = read_number(data, size, &pos, &maxval)) != ERR_OK)
            return rc;

    if(maxval != 255) {
        return ERR_UNSUPPORTED;
    }

    /* exactly one whitespace byte, then the pixels start */
    if(pos >= size || !is_space(data[pos])) {
        return ERR_FORMAT;
    }
    pos += 1;

    /* image_alloc checks the dimensions before computing the size */
    rc = image_alloc(img, width, height, channels);
    if(rc == ERR_ARGS) {
        return ERR_FORMAT;          /* bad dimensions = bad file */
    }
    if(rc != ERR_OK) {
        return rc;                  /* ERR_NOMEM */
    }

    /* the file must contain all the pixel bytes */
    if(size - pos < image_size(img)) {
        image_free(img);
        return ERR_FORMAT;
    }

    memcpy(img->pixels, data + pos, image_size(img));
    return ERR_OK;
}

int image_load_ppm(const char *path, image_t *img)
{
    uint8_t *data = NULL;
    size_t size = 0;
    int rc;

    if (path == NULL || img == NULL)
        return ERR_ARGS;

    rc = file_read_all(path, &data, &size);
    if (rc != ERR_OK)
        return rc;

    rc = image_parse_ppm(data, size, img);
    free(data);                   
    return rc;
}


int image_save_ppm(const char *path, const image_t *img)
{
    char header[64];
    uint8_t *buf = NULL;
    size_t total = 0;
    int header_len;
    int rc;

    if (path == NULL || img == NULL || img->pixels == NULL ||
        (img->channels != 1 && img->channels != 3))
        return ERR_ARGS;

    /* P6 = color (3 channels), P5 = gray (1 channel) */
    header_len = snprintf(header, sizeof(header), "P%c\n%u %u\n255\n",
                          img->channels == 3 ? '6' : '5',
                          (unsigned)img->width, (unsigned)img->height);
    if (header_len <= 0 || (size_t)header_len >= sizeof(header))
        return ERR_ARGS;

    /* header + pixels in one buffer, written with a single call */
    total = (size_t)header_len + image_size(img);
    buf = (uint8_t *)malloc(total);
    if (buf == NULL)
        return ERR_NOMEM;

    memcpy(buf, header, (size_t)header_len);
    memcpy(buf + header_len, img->pixels, image_size(img));

    rc = file_write_all(path, buf, total);
    free(buf);
    return rc;
}