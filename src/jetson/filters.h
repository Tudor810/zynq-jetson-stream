/*
 * filters.h - image operations, in two implementations with identical
 * signatures: cpu_* (plain C, the reference) and gpu_* (CUDA kernels).
 *
 * Each function reads `in` and creates `out` with image_alloc;
 * the caller frees `out` with image_free.
 * Returns ERR_OK, ERR_ARGS or ERR_NOMEM.
 */
#ifndef FILTERS_H
#define FILTERS_H

#include "common/image.h"

#ifdef __cplusplus
extern "C" {
#endif

/* RGB -> gray: (299 R + 587 G + 114 B + 500) / 1000. A gray input is copied. */
int cpu_grayscale(const image_t *in, image_t *out);

#ifdef USE_CUDA
int gpu_grayscale(const image_t *in, image_t *out);
#endif

#ifdef __cplusplus
}
#endif

#endif /* FILTERS_H */