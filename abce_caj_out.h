#ifndef _ABCE_CAJ_OUT_H_
#define _ABCE_CAJ_OUT_H_

#include <stddef.h>
#include <stdint.h>

struct abce_caj_out_ctx {
	int (*datasink)(struct abce_caj_out_ctx *ctx, const char *data, size_t sz);
	void *userdata;
	const char *commanlindentchars;
	size_t indentcharsz;
	size_t indentamount;
	size_t curindentlevel;
	unsigned first:1;
	unsigned veryfirst:1;
};

int abce_caj_out_put2_start_dict(struct abce_caj_out_ctx *ctx, const char *key, size_t keysz);
int abce_caj_out_put_start_dict(struct abce_caj_out_ctx *ctx, const char *key);
int abce_caj_out_put2_start_array(struct abce_caj_out_ctx *ctx, const char *key, size_t keysz);
int abce_caj_out_put_start_array(struct abce_caj_out_ctx *ctx, const char *key);
int abce_caj_out_add_start_dict(struct abce_caj_out_ctx *ctx);
int abce_caj_out_add_start_array(struct abce_caj_out_ctx *ctx);
int abce_caj_out_end_dict(struct abce_caj_out_ctx *ctx);
int abce_caj_out_end_array(struct abce_caj_out_ctx *ctx);
int abce_caj_out_put22_string(struct abce_caj_out_ctx *ctx, const char *key, size_t keysz, const char *val, size_t valsz);
int abce_caj_out_put_string(struct abce_caj_out_ctx *ctx, const char *key, const char *val);
int abce_caj_out_add2_string(struct abce_caj_out_ctx *ctx, const char *val, size_t valsz);
int abce_caj_out_add_string(struct abce_caj_out_ctx *ctx, const char *val);
int abce_caj_out_put2_number(struct abce_caj_out_ctx *ctx, const char *key, size_t keysz, double d);
int abce_caj_out_put_number(struct abce_caj_out_ctx *ctx, const char *key, double d);
int abce_caj_out_put2_number_ex(struct abce_caj_out_ctx *ctx, const char *key, size_t keysz, double d);
int abce_caj_out_put_number_ex(struct abce_caj_out_ctx *ctx, const char *key, double d);
int abce_caj_out_add_number(struct abce_caj_out_ctx *ctx, double d);
int abce_caj_out_add_number_ex(struct abce_caj_out_ctx *ctx, double d);
int abce_caj_out_put2_flop(struct abce_caj_out_ctx *ctx, const char *key, size_t keysz, double d);
int abce_caj_out_put_flop(struct abce_caj_out_ctx *ctx, const char *key, double d);
int abce_caj_out_put2_flop_ex(struct abce_caj_out_ctx *ctx, const char *key, size_t keysz, double d);
int abce_caj_out_put_flop_ex(struct abce_caj_out_ctx *ctx, const char *key, double d);
int abce_caj_out_add_flop(struct abce_caj_out_ctx *ctx, double d);
int abce_caj_out_add_flop_ex(struct abce_caj_out_ctx *ctx, double d);
int abce_caj_out_put2_i64(struct abce_caj_out_ctx *ctx, const char *key, size_t keysz, int64_t i);
int abce_caj_out_put_i64(struct abce_caj_out_ctx *ctx, const char *key, int64_t i);
int abce_caj_out_add_i64(struct abce_caj_out_ctx *ctx, int64_t i);
int abce_caj_out_put2_boolean(struct abce_caj_out_ctx *ctx, const char *key, size_t keysz, int b);
int abce_caj_out_put_boolean(struct abce_caj_out_ctx *ctx, const char *key, int b);
int abce_caj_out_add_boolean(struct abce_caj_out_ctx *ctx, int b);
int abce_caj_out_put2_null(struct abce_caj_out_ctx *ctx, const char *key, size_t keysz);
int abce_caj_out_put_null(struct abce_caj_out_ctx *ctx, const char *key);
int abce_caj_out_add_null(struct abce_caj_out_ctx *ctx);

void abce_caj_out_init(struct abce_caj_out_ctx *ctx, int tabs, size_t indentamount,
                  int (*datasink)(struct abce_caj_out_ctx *ctx, const char *data, size_t sz),
		  void *userdata);

#endif
