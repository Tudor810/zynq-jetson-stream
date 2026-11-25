/*
 * pipeline.h - the full image processing chain run by the server:
 *
 *     grayscale -> Gaussian blur -> Sobel -> threshold
 *
 * Each stage feeds the next; intermediate images are freed inside.
 * The same chain runs on the CPU (cpu_* filters) or the GPU (gpu_* filters).
 */
#ifndef PIPELINE_H
#define PIPELINE_H

#include <stdint.h>

#include "common/image.h"

#define PIPELINE_DEFAULT_THRESHOLD 100

typedef enum {
    PIPELINE_CPU = 0,
    PIPELINE_GPU = 1        /* only available when built with USE_CUDA */
} pipeline_backend_t;

typedef struct {
    pipeline_backend_t backend;
    uint8_t threshold;      /* binarization: pixel >= threshold -> 255, else 0 */
} pipeline_config_t;

/* Initializer with the default settings:
 *     pipeline_config_t cfg = PIPELINE_CONFIG_DEFAULT;          */
#define PIPELINE_CONFIG_DEFAULT \
    { .backend = PIPELINE_CPU, .threshold = PIPELINE_DEFAULT_THRESHOLD }

/* Time spent in each stage, in microseconds. */
typedef struct {
    uint32_t grayscale_us;
    uint32_t blur_us;
    uint32_t sobel_us;
    uint32_t threshold_us;
    uint32_t total_us;
} pipeline_times_t;

/*
 * Run the whole chain on `in` (1 or 3 channels) and create `out`
 * (1 channel, values 0 or 255). The caller frees `out` with image_free.
 *
 * cfg:   settings; NULL means PIPELINE_CONFIG_DEFAULT.
 * times: filled with the stage timings; may be NULL.
 *
 * Returns ERR_OK, ERR_ARGS, ERR_NOMEM, or ERR_UNSUPPORTED if the GPU is
 * requested in a build without CUDA. On error, `out` is left empty and
 * every intermediate image is already freed.
 */
int run_pipeline(const image_t *in, image_t *out,
                 const pipeline_config_t *cfg, pipeline_times_t *times);

#endif /* PIPELINE_H */