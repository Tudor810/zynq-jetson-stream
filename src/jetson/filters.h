/*
 * filters.h - image operations, in two implementations with identical
 * signatures: cpu_* (plain C, the reference) and gpu_* (CUDA kernels).
 *
 * Common rules for every function:
 *   - reads `in`, creates `out` with image_alloc (out must not own pixels)
 *   - the caller frees `out` with image_free
 *   - returns ERR_OK, ERR_ARGS or ERR_NOMEM
 *   - integer arithmetic only, so CPU and GPU results are byte-identical
 *   - pixels outside the image (needed by blur and Sobel at the borders)
 *     take the value of the nearest pixel inside the image ("clamp")
 */
#ifndef FILTERS_H
#define FILTERS_H

#include <stdint.h>

#include "common/image.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * RGB (3 channels) -> gray (1 channel):
 *     gray = (299 R + 587 G + 114 B + 500) / 1000
 * A 1-channel input is copied unchanged.
 */
int cpu_grayscale(const image_t *in, image_t *out);

/*
 * 3x3 Gaussian blur of a gray image (1 channel in, 1 channel out):
 *
 *     1 2 1
 *     2 4 2   / 16     out = (weighted sum + 8) / 16
 *     1 2 1
 */
int cpu_gaussian_blur(const image_t *in, image_t *out);

/*
 * Sobel edge detection of a gray image (1 channel in, 1 channel out):
 *
 *     Gx = -1 0 1     Gy = -1 -2 -1
 *          -2 0 2           0  0  0
 *          -1 0 1           1  2  1
 *
 *     out = min(|Gx| + |Gy|, 255)
 */
int cpu_sobel(const image_t *in, image_t *out);

/*
 * Binarization of a gray image (1 channel in, 1 channel out):
 *     out = (in >= threshold) ? 255 : 0
 */
int cpu_threshold(const image_t *in, image_t *out, uint8_t threshold);

#ifdef USE_CUDA
int gpu_grayscale(const image_t *in, image_t *out);
int gpu_gaussian_blur(const image_t *in, image_t *out);
int gpu_sobel(const image_t *in, image_t *out);
int gpu_threshold(const image_t *in, image_t *out, uint8_t threshold);
#endif

#ifdef __cplusplus
}
#endif

#endif /* FILTERS_H */