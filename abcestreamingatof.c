#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "abcestreamingatof.h"

static inline void abce_streaming_atof_ctx_mark_negative(struct abce_streaming_atof_ctx *ctx)
{
	if (ctx->bufsiz != 0)
	{
		abort();
	}
	ctx->buf[ctx->bufsiz++] = '-';
}
static inline void abce_streaming_atof_ctx_emit_digit(struct abce_streaming_atof_ctx *ctx, char digit)
{
	if (!isdigit((unsigned char)digit))
	{
		abort();
	}
	if (digit == '0' && (ctx->bufsiz == 0 || (ctx->bufsiz == 1 && ctx->buf[0] == '-')))
	{
		if (ctx->exponent_offset_set && ctx->exponent_offset < 0)
		{
			ctx->exponent_offset--;
		}
		return;
	}
	if (ctx->bufsiz >= ((int)sizeof(ctx->buf)) - 6)
	{
		// "e-999\0"
		if (!ctx->exponent_offset_set)
		{
			if (ctx->skip_offset == INT64_MAX)
			{
				// Uh-oh. Let's hope this never happens.
				// It's a huge honking number unless it has
				// a very negative exponent.
				return;
			}
			ctx->skip_offset++;
		}
		return;
	}
	ctx->buf[ctx->bufsiz++] = digit;
	if (ctx->bufsiz == 1 || (ctx->bufsiz == 2 && ctx->buf[0] == '-'))
	{
		ctx->buf[ctx->bufsiz++] = '.';
	}
}
static inline void abce_streaming_atof_ctx_store_period(struct abce_streaming_atof_ctx *ctx)
{
	int periodbufsiz;
	int curbufsiz;
	if (ctx->bufsiz == 0 || (ctx->bufsiz == 1 && ctx->buf[0] == '-'))
	{
		ctx->exponent_offset = -1;
		ctx->exponent_offset_set = 1;
		return;
		// FIXME implement
	}
	if (ctx->buf[0] == '-')
	{
		periodbufsiz = 3;
	}
	else
	{
		periodbufsiz = 2;
	}
	curbufsiz = ctx->bufsiz;
	ctx->exponent_offset = curbufsiz - periodbufsiz + ctx->skip_offset;
	ctx->exponent_offset_set = 1;
}
static inline void abce_streaming_atof_ctx_set_exponent(struct abce_streaming_atof_ctx *ctx, int64_t exponent)
{
	ctx->exponent_offset += exponent;
}
static inline double abce_streaming_atof_ctx_get_number(struct abce_streaming_atof_ctx *ctx)
{
	int nch;
	int remain;
	if (ctx->mode == STREAMING_ATOF_MODE_ERROR)
	{
		abort();
	}
	if (!ctx->exponent_offset_set)
	{
		abce_streaming_atof_ctx_store_period(ctx);
		abce_streaming_atof_ctx_emit_digit(ctx, '0');
	}
	if (ctx->bufsiz == -1)
	{
		return atof(ctx->buf);
	}

	if (ctx->bufsiz == 0 || (ctx->bufsiz == 1 && ctx->buf[0] == '-'))
	{
		ctx->buf[ctx->bufsiz++] = '0';
		ctx->buf[ctx->bufsiz++] = '.';
		ctx->buf[ctx->bufsiz++] = '0';
	}

	ctx->buf[ctx->bufsiz++] = 'e';
	if (ctx->exponent_offset > 999)
	{
		ctx->exponent_offset = 999;
	}
	else if (ctx->exponent_offset < -999)
	{
		ctx->exponent_offset = -999;
	}
	remain = (int)sizeof(ctx->buf);
	remain -= ctx->bufsiz;
	if (remain < 0)
	{
		abort();
	}
	nch = snprintf(ctx->buf + ctx->bufsiz, (size_t)remain, "%.3d", (int)ctx->exponent_offset);
	if (ctx->bufsiz + nch >= ((int)sizeof(ctx->buf)))
	{
		abort();
	}
	ctx->bufsiz += nch;
	ctx->buf[ctx->bufsiz] = '\0';
	ctx->bufsiz = -1; // special marker
	return atof(ctx->buf);
}

double abce_streaming_atof_end(struct abce_streaming_atof_ctx *ctx)
{
	abce_streaming_atof_ctx_set_exponent(ctx, ctx->expnegative ? (-ctx->exponent) : ctx->exponent);
	ctx->mode = STREAMING_ATOF_MODE_DONE;
	ctx->exponent = 0;
	return abce_streaming_atof_ctx_get_number(ctx);
}

ssize_t abce_streaming_atof_feed(struct abce_streaming_atof_ctx *ctx, const char *data, size_t len)
{
	size_t i;
	if (len > SSIZE_MAX)
	{
		abort();
	}
	if (ctx->mode == STREAMING_ATOF_MODE_DONE || ctx->mode == STREAMING_ATOF_MODE_ERROR)
	{
		return 0;
	}

	for (i = 0; i < len; i++)
	{
		//printf("char %c intermediate: %.20Lg\n", data[i], ctx->d);
		//printf("char %c intermediate: %.20g %.20g %d %d %d\n", data[i], ctx->d1, ctx->d2, ctx->add_exponent1, ctx->add_exponent2, ctx->sub_exponent);
		if (ctx->mode == STREAMING_ATOF_MODE_MANTISSA_SIGN && data[i] == '+' && !ctx->strict_json)
		{
			ctx->mode = STREAMING_ATOF_MODE_MANTISSA_FIRST;
			continue;
		}
		if (ctx->mode == STREAMING_ATOF_MODE_MANTISSA_SIGN && data[i] == '-')
		{
			abce_streaming_atof_ctx_mark_negative(ctx);
			ctx->mode = STREAMING_ATOF_MODE_MANTISSA_FIRST;
			continue;
		}
		if (ctx->mode == STREAMING_ATOF_MODE_MANTISSA_SIGN && data[i] == '.' && !ctx->strict_json)
		{
			ctx->mode = STREAMING_ATOF_MODE_MANTISSA_FIRST;
		}
		if (ctx->mode == STREAMING_ATOF_MODE_MANTISSA_SIGN && isdigit((unsigned char)data[i]))
		{
			ctx->mode = STREAMING_ATOF_MODE_MANTISSA_FIRST;
		}
		if (ctx->mode == STREAMING_ATOF_MODE_MANTISSA_FIRST && data[i] == '0')
		{
			abce_streaming_atof_ctx_emit_digit(ctx, data[i]);
			ctx->mode = STREAMING_ATOF_MODE_PERIOD_OR_EXPONENT_CHAR;
			continue;
		}
		if (ctx->mode == STREAMING_ATOF_MODE_PERIOD_OR_EXPONENT_CHAR && (data[i] == 'e' || data[i] == 'E'))
		{
			ctx->mode = STREAMING_ATOF_MODE_EXPONENT_CHAR;
		}
		else if (ctx->mode == STREAMING_ATOF_MODE_PERIOD_OR_EXPONENT_CHAR && data[i] == '.')
		{
			ctx->mode = STREAMING_ATOF_MODE_MANTISSA;
		}
		else if (ctx->mode == STREAMING_ATOF_MODE_PERIOD_OR_EXPONENT_CHAR && !isdigit((unsigned char)data[i]))
		{
			ctx->mode = STREAMING_ATOF_MODE_DONE;
			return (ssize_t)i;
		}
		if (ctx->mode == STREAMING_ATOF_MODE_EXPONENT_CHAR)
		{
			if (data[i] == 'e' || data[i] == 'E')
			{
				if (!ctx->exponent_offset_set)
				{
					abce_streaming_atof_ctx_store_period(ctx);
					abce_streaming_atof_ctx_emit_digit(ctx, '0');
				}
				ctx->mode = STREAMING_ATOF_MODE_EXPONENT_SIGN;
				continue;
			}
			ctx->mode = STREAMING_ATOF_MODE_ERROR;
			return (ssize_t)i;
		}
		if ((ctx->mode == STREAMING_ATOF_MODE_MANTISSA || ctx->mode == STREAMING_ATOF_MODE_MANTISSA_FIRST))
		{
			if (isdigit((unsigned char)data[i]))
			{
				abce_streaming_atof_ctx_emit_digit(ctx, data[i]);
				ctx->mode = STREAMING_ATOF_MODE_MANTISSA;
				continue;
			}
			if (data[i] == '.')
			{
				abce_streaming_atof_ctx_store_period(ctx);
				ctx->mode = STREAMING_ATOF_MODE_MANTISSA_FRAC_FIRST;
				continue;
			}
			if (ctx->mode == STREAMING_ATOF_MODE_MANTISSA_FIRST)
			{
				ctx->mode = STREAMING_ATOF_MODE_ERROR;
				return (ssize_t)i;
			}
			if (data[i] == 'e' || data[i] == 'E')
			{
				if (!ctx->exponent_offset_set)
				{
					abce_streaming_atof_ctx_store_period(ctx);
					abce_streaming_atof_ctx_emit_digit(ctx, '0');
				}
				ctx->mode = STREAMING_ATOF_MODE_EXPONENT_SIGN;
				continue;
			}
			ctx->mode = STREAMING_ATOF_MODE_DONE;
			return (ssize_t)i;
		}
		if (ctx->mode == STREAMING_ATOF_MODE_MANTISSA_FRAC || ctx->mode == STREAMING_ATOF_MODE_MANTISSA_FRAC_FIRST)
		{
			if (isdigit((unsigned char)data[i]))
			{
				abce_streaming_atof_ctx_emit_digit(ctx, data[i]);
				ctx->mode = STREAMING_ATOF_MODE_MANTISSA_FRAC;
				continue;
			}
			// Let's accept "1." and "1.e2" if not strict
			if (ctx->mode == STREAMING_ATOF_MODE_MANTISSA_FRAC_FIRST && ctx->strict_json)
			{
				ctx->mode = STREAMING_ATOF_MODE_ERROR;
				return (ssize_t)i;
			}
			if (data[i] == 'e' || data[i] == 'E')
			{
				if (!ctx->exponent_offset_set)
				{
					abce_streaming_atof_ctx_store_period(ctx);
					abce_streaming_atof_ctx_emit_digit(ctx, '0');
				}
				ctx->mode = STREAMING_ATOF_MODE_EXPONENT_SIGN;
				continue;
			}
			ctx->mode = STREAMING_ATOF_MODE_DONE;
			return (ssize_t)i;
		}
		if (ctx->mode == STREAMING_ATOF_MODE_EXPONENT_SIGN)
		{
			if (data[i] == '+')
			{
				ctx->expnegative = 0;
				ctx->mode = STREAMING_ATOF_MODE_EXPONENT_FIRST;
				continue;
			}
			if (data[i] == '-')
			{
				ctx->expnegative = 1;
				ctx->mode = STREAMING_ATOF_MODE_EXPONENT_FIRST;
				continue;
			}
			if (isdigit((unsigned char)data[i]))
			{
				if (ctx->exponent > (INT64_MAX - 9)/10)
				{
					// prevent overflow
					continue;
				}
				ctx->exponent *= 10;
				ctx->exponent += (data[i] - '0');
				ctx->mode = STREAMING_ATOF_MODE_EXPONENT;
				continue;
			}
			ctx->mode = STREAMING_ATOF_MODE_ERROR;
			return (ssize_t)i;
		}
		if (ctx->mode == STREAMING_ATOF_MODE_EXPONENT_FIRST || ctx->mode == STREAMING_ATOF_MODE_EXPONENT)
		{
			if (isdigit((unsigned char)data[i]))
			{
				if (ctx->exponent > (INT64_MAX - 9)/10)
				{
					// prevent overflow
					continue;
				}
				ctx->exponent *= 10;
				ctx->exponent += (data[i] - '0');
				ctx->mode = STREAMING_ATOF_MODE_EXPONENT;
				continue;
			}
			if (ctx->mode == STREAMING_ATOF_MODE_EXPONENT_FIRST)
			{
				ctx->mode = STREAMING_ATOF_MODE_ERROR;
				return (ssize_t)i;
			}
			ctx->mode = STREAMING_ATOF_MODE_DONE;
			return (ssize_t)i;
		}
		ctx->mode = STREAMING_ATOF_MODE_ERROR;
		return (ssize_t)i;
	}
	return (ssize_t)len;
}
