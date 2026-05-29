/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "feedback.h"

LOG_MODULE_REGISTER(feedback, LOG_LEVEL_INF);

/* High-Speed uses Q16.16 feedback format.
 * At 48 kHz with 8000 microframes/sec, nominal is 6 samples/microframe.
 */
#define FEEDBACK_K		16
#define SAMPLES_PER_MICROFRAME	6

struct feedback_ctx *feedback_init(void)
{
	return NULL;
}

void feedback_process(struct feedback_ctx *ctx)
{
	ARG_UNUSED(ctx);
}

void feedback_reset_ctx(struct feedback_ctx *ctx)
{
	ARG_UNUSED(ctx);
}

void feedback_start(struct feedback_ctx *ctx, int i2s_blocks_queued)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(i2s_blocks_queued);
}

uint32_t feedback_value(struct feedback_ctx *ctx)
{
	/* Always request nominal number of samples per microframe */
	return SAMPLES_PER_MICROFRAME << FEEDBACK_K;
}
