#ifndef _ABCESTREAMINGATOF_H_
#define _ABCESTREAMINGATOF_H_ 1

#include <stdio.h>
#include <stdint.h>

enum abce_streaming_atof_mode {
	STREAMING_ATOF_MODE_PERIOD_OR_EXPONENT_CHAR,
	STREAMING_ATOF_MODE_MANTISSA_SIGN,
	STREAMING_ATOF_MODE_MANTISSA_FIRST,
	STREAMING_ATOF_MODE_MANTISSA,
	STREAMING_ATOF_MODE_MANTISSA_FRAC_FIRST,
	STREAMING_ATOF_MODE_MANTISSA_FRAC,
	STREAMING_ATOF_MODE_EXPONENT_CHAR,
	STREAMING_ATOF_MODE_EXPONENT_SIGN,
	STREAMING_ATOF_MODE_EXPONENT_FIRST,
	STREAMING_ATOF_MODE_EXPONENT,
	STREAMING_ATOF_MODE_DONE,
	STREAMING_ATOF_MODE_ERROR,
};

struct abce_streaming_atof_ctx {
	char buf[64];
	int bufsiz;
	enum abce_streaming_atof_mode mode;
	int64_t exponent_offset;
	int64_t skip_offset;
	int64_t exponent;
	unsigned exponent_offset_set:1;
	unsigned expnegative:1;
	unsigned strict_json:1;
};

static inline void abce_streaming_atof_init_ex(struct abce_streaming_atof_ctx *ctx, int strict_json)
{
	ctx->bufsiz = 0;
	ctx->mode = STREAMING_ATOF_MODE_MANTISSA_SIGN;
	ctx->exponent_offset = 0;
	ctx->exponent_offset_set = 0;
	ctx->skip_offset = 0;
	ctx->exponent = 0;
	ctx->expnegative = 0;
	ctx->strict_json = !!strict_json;
}
static inline void abce_streaming_atof_init(struct abce_streaming_atof_ctx *ctx)
{
	abce_streaming_atof_init_ex(ctx, 0);
}
static inline void abce_streaming_atof_init_strict_json(struct abce_streaming_atof_ctx *ctx)
{
	abce_streaming_atof_init_ex(ctx, 1);
}

static inline int abce_streaming_atof_is_error(struct abce_streaming_atof_ctx *ctx)
{
	return ctx->mode == STREAMING_ATOF_MODE_ERROR;
}

double abce_streaming_atof_end(struct abce_streaming_atof_ctx *ctx);

ssize_t abce_streaming_atof_feed(struct abce_streaming_atof_ctx *ctx, const char *data, size_t len);

#endif
