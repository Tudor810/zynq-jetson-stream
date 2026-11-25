#include "pipeline.h"
#include "filters.h"
#include "common/errors.h"
#include "common/timing.h"

#include <stddef.h>

int run_pipeline(const image_t *in, image_t *out,
                 const pipeline_config_t *cfg, pipeline_times_t *times)
{
    pipeline_config_t defaults = PIPELINE_CONFIG_DEFAULT;
    pipeline_times_t t = {0};
    uint64_t start, t0;
    int rc;
    image_t itm = {0};

    if (in == NULL || out == NULL)
        return ERR_ARGS;
    *out = (image_t){0};

    if (cfg == NULL)
        cfg = &defaults;

    if (cfg->backend != PIPELINE_CPU)
        return ERR_UNSUPPORTED;         

    start = time_now_us();

    /* ---- stage 1: grayscale ---- */
    t0 = time_now_us();
    rc = cpu_grayscale(in, &itm);
    t.grayscale_us = (uint32_t)(time_now_us() - t0);
    if (rc != ERR_OK)
        return rc;

    t0 = time_now_us();
    rc = cpu_gaussian_blur(&itm, out);
    image_free(&itm);
    if(rc != ERR_OK)
        return rc;

    t.blur_us = (uint32_t)(time_now_us() - t0);
    /* TODO: blur, Sobel and threshold, each into a temporary image,
       with the last stage writing into `out` */

    t.total_us = (uint32_t)(time_now_us() - start);
    if (times != NULL)
        *times = t;

    return ERR_OK;
}