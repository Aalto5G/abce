#ifndef _CAJ_H_
#define _CAJ_H_

#include <stddef.h>
#include "abcestreamingatof.h"

struct abce_caj_handler;

struct abce_caj_handler_vtable {
	int (*start_dict)(struct abce_caj_handler *cajh, const char *key, size_t keysz);
	int (*end_dict)(struct abce_caj_handler *cajh, const char *key, size_t keysz);
	int (*start_array)(struct abce_caj_handler *cajh, const char *key, size_t keysz);
	int (*end_array)(struct abce_caj_handler *cajh, const char *key, size_t keysz);
	int (*handle_null)(struct abce_caj_handler *cajh, const char *key, size_t keysz);
	int (*handle_string)(struct abce_caj_handler *cajh, const char *key, size_t keysz, const char *val, size_t valsz);
	int (*handle_number)(struct abce_caj_handler *cajh, const char *key, size_t keysz, double d, int is_integer);
	int (*handle_boolean)(struct abce_caj_handler *cajh, const char *key, size_t keysz, int b);
};

struct abce_caj_handler {
	const struct abce_caj_handler_vtable *vtable;
	void *userdata;
};

enum abce_caj_mode {
	ABCE_CAJ_MODE_KEYSTRING,
	ABCE_CAJ_MODE_KEYSTRING_ESCAPE,
	ABCE_CAJ_MODE_KEYSTRING_UESCAPE, // sz tells how many of these we have read
	ABCE_CAJ_MODE_STRING,
	ABCE_CAJ_MODE_STRING_ESCAPE,
	ABCE_CAJ_MODE_STRING_UESCAPE, // sz tells how many of these we have read
	ABCE_CAJ_MODE_TRUE, // sz tells how many of these we have read
	ABCE_CAJ_MODE_FALSE, // sz tells how many of these we have read
	ABCE_CAJ_MODE_NULL, // sz tells how many of these we have read
	ABCE_CAJ_MODE_FIRSTKEY,
	ABCE_CAJ_MODE_KEY,
	ABCE_CAJ_MODE_FIRSTVAL,
	ABCE_CAJ_MODE_VAL,
	ABCE_CAJ_MODE_COLON,
	ABCE_CAJ_MODE_COMMA,
	ABCE_CAJ_MODE_NUMBER,
};

struct abce_caj_keystack_item {
	char *key;
	size_t keysz;
};

struct abce_caj_ctx {
	enum abce_caj_mode mode;
	unsigned char sz; // uescape or token
	char uescape[5];
	unsigned char keypresent:1;
	char *key;
	size_t keysz;
	size_t keycap;
	struct abce_caj_keystack_item *keystack;
	size_t keystacksz;
	size_t keystackcap;
	char *val;
	size_t valsz;
	size_t valcap;
	struct abce_caj_handler *handler;
	struct abce_streaming_atof_ctx streamingatof;
	int is_integer;
};

void abce_caj_init(struct abce_caj_ctx *caj, struct abce_caj_handler *handler);

void abce_caj_free(struct abce_caj_ctx *caj);

int abce_caj_feed(struct abce_caj_ctx *caj, const void *vdata, size_t usz, int eof);

struct abce_pullcaj_ctx {
	enum abce_caj_mode mode;
	unsigned char sz; // uescape or token
	char uescape[5];
	unsigned char keypresent:1;
	char *key;
	size_t keysz;
	size_t keycap;
	struct abce_caj_keystack_item *keystack;
	size_t keystacksz;
	size_t keystackcap;
	char *val;
	size_t valsz;
	size_t valcap;
	struct abce_streaming_atof_ctx streamingatof;
	int is_integer;

	const void *vdata;
	size_t usz;
	int eof;
	size_t i;

	int state;
};

enum abce_pullcaj_event {
	ABCE_CAJ_EV_START_DICT,
	ABCE_CAJ_EV_END_DICT,
	ABCE_CAJ_EV_START_ARRAY,
	ABCE_CAJ_EV_END_ARRAY,
	ABCE_CAJ_EV_NULL,
	ABCE_CAJ_EV_STR, // union field str
	ABCE_CAJ_EV_NUM, // union field num
	ABCE_CAJ_EV_BOOL, // union field b
};

struct abce_pullcaj_event_info {
	enum abce_pullcaj_event ev;
	const char *key;
	size_t keysz;
	union {
		struct {
			int b;
		} b;
		struct {
			double d;
			int is_integer;
		} num;
		struct {
			const char *val;
			size_t valsz;
		} str;
	} u;
};

void abce_pullcaj_init(struct abce_pullcaj_ctx *pc);

// buffer needs to be valid until next call to this function is made
int abce_pullcaj_set_buf(struct abce_pullcaj_ctx *pc, const void *vdata, size_t usz, int eof);

/*
 * 1: got event
 * 0: end of JSON
 * -EINPROGRESS: need to call again with new buffer
 * <0: another error code, means out of memory or invalid JSON
 */
int abce_pullcaj_get_event(struct abce_pullcaj_ctx *pc, struct abce_pullcaj_event_info *ev);

void abce_pullcaj_free(struct abce_pullcaj_ctx *pc);

#endif
