#include "abce_caj.h"
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

void abce_caj_init(struct abce_caj_ctx *caj, struct abce_caj_handler *handler)
{
	memset(caj, 0, sizeof(*caj));
	caj->mode = ABCE_CAJ_MODE_VAL;
	caj->sz = 0;
	memset(caj->uescape, 0, sizeof(caj->uescape));
	caj->keypresent = 0;
	caj->key = NULL;
	caj->keysz = 0;
	caj->keycap = 0;
	caj->keystack = NULL;
	caj->keystacksz = 0;
	caj->keystackcap = 0;
	caj->val = NULL;
	caj->valsz = 0;
	caj->valcap = 0;
	caj->handler = handler;
}

void abce_pullcaj_init(struct abce_pullcaj_ctx *caj)
{
	memset(caj, 0, sizeof(*caj));
	caj->mode = ABCE_CAJ_MODE_VAL;
	caj->sz = 0;
	memset(caj->uescape, 0, sizeof(caj->uescape));
	caj->keypresent = 0;
	caj->key = NULL;
	caj->keysz = 0;
	caj->keycap = 0;
	caj->keystack = NULL;
	caj->keystacksz = 0;
	caj->keystackcap = 0;
	caj->val = NULL;
	caj->valsz = 0;
	caj->valcap = 0;

	caj->vdata = NULL;
	caj->usz = 0;
	caj->eof = 0;
	caj->i = 0;

	caj->state = 0;
}

void abce_caj_free(struct abce_caj_ctx *caj)
{
	free(caj->key);
	caj->key = NULL;
	free(caj->keystack);
	caj->keystack = NULL;
	free(caj->val);
	caj->val = NULL;
	memset(caj, 0, sizeof(*caj));
	abce_caj_init(caj, NULL);
}

void abce_pullcaj_free(struct abce_pullcaj_ctx *caj)
{
	free(caj->key);
	caj->key = NULL;
	free(caj->keystack);
	caj->keystack = NULL;
	free(caj->val);
	caj->val = NULL;
	memset(caj, 0, sizeof(*caj));
	abce_pullcaj_init(caj);
}

static int abce_caj_put_key(struct abce_caj_ctx *caj, char ch)
{
	size_t newcap = 0;
	char *newbuf;
	if (caj->keysz >= caj->keycap)
	{
		newcap = caj->keysz * 2 + 16;
		newbuf = realloc(caj->key, newcap);
		if (newbuf == NULL)
		{
			return -ENOMEM;
		}
		caj->key = newbuf;
		caj->keycap = newcap;
	}
	caj->key[caj->keysz++] = ch;
	return 0;
}

static int abce_pullcaj_put_key(struct abce_pullcaj_ctx *caj, char ch)
{
	size_t newcap = 0;
	char *newbuf;
	if (caj->keysz >= caj->keycap)
	{
		newcap = caj->keysz * 2 + 16;
		newbuf = realloc(caj->key, newcap);
		if (newbuf == NULL)
		{
			return -ENOMEM;
		}
		caj->key = newbuf;
		caj->keycap = newcap;
	}
	caj->key[caj->keysz++] = ch;
	return 0;
}

static int abce_caj_put_val(struct abce_caj_ctx *caj, char ch)
{
	size_t newcap = 0;
	char *newbuf;
	if (caj->valsz >= caj->valcap)
	{
		newcap = caj->valsz * 2 + 16;
		newbuf = realloc(caj->val, newcap);
		if (newbuf == NULL)
		{
			return -ENOMEM;
		}
		caj->val = newbuf;
		caj->valcap = newcap;
	}
	caj->val[caj->valsz++] = ch;
	return 0;
}

static int abce_pullcaj_put_val(struct abce_pullcaj_ctx *caj, char ch)
{
	size_t newcap = 0;
	char *newbuf;
	if (caj->valsz >= caj->valcap)
	{
		newcap = caj->valsz * 2 + 16;
		newbuf = realloc(caj->val, newcap);
		if (newbuf == NULL)
		{
			return -ENOMEM;
		}
		caj->val = newbuf;
		caj->valcap = newcap;
	}
	caj->val[caj->valsz++] = ch;
	return 0;
}

static int abce_caj_get_keystack(struct abce_caj_ctx *caj)
{
	size_t newcap;
	char *newbuf;

	newcap = caj->keystack[caj->keystacksz-1].keysz + 1;
	if (newcap > caj->keycap)
	{
		newbuf = realloc(caj->key, newcap);
		if (newbuf == NULL)
		{
			return -ENOMEM;
		}
		caj->key = newbuf;
		caj->keycap = newcap;
	}

	if (caj->keystack[caj->keystacksz-1].key == NULL)
	{
		caj->keypresent = 0;
		caj->keystacksz--;
		return 0;
	}

	caj->keypresent = 1;

	caj->keysz = newcap-1;
	memcpy(caj->key, caj->keystack[caj->keystacksz-1].key, caj->keysz);
	caj->key[caj->keysz] = '\0';

	free(caj->keystack[caj->keystacksz-1].key);
	caj->keystack[caj->keystacksz-1].key = NULL;
	caj->keystack[caj->keystacksz-1].keysz = 0;
	caj->keystacksz--;

	return 0;
}

static int abce_pullcaj_get_keystack(struct abce_pullcaj_ctx *caj)
{
	size_t newcap;
	char *newbuf;

	newcap = caj->keystack[caj->keystacksz-1].keysz + 1;
	if (newcap > caj->keycap)
	{
		newbuf = realloc(caj->key, newcap);
		if (newbuf == NULL)
		{
			return -ENOMEM;
		}
		caj->key = newbuf;
		caj->keycap = newcap;
	}

	if (caj->keystack[caj->keystacksz-1].key == NULL)
	{
		caj->keypresent = 0;
		caj->keystacksz--;
		return 0;
	}

	caj->keypresent = 1;

	caj->keysz = newcap-1;
	memcpy(caj->key, caj->keystack[caj->keystacksz-1].key, caj->keysz);
	caj->key[caj->keysz] = '\0';

	free(caj->keystack[caj->keystacksz-1].key);
	caj->keystack[caj->keystacksz-1].key = NULL;
	caj->keystack[caj->keystacksz-1].keysz = 0;
	caj->keystacksz--;

	return 0;
}

static int abce_caj_put_keystack_1(struct abce_caj_ctx *caj)
{
	size_t newcap;
	char *newbuf;
	struct abce_caj_keystack_item *newstack;
	if (caj->keystacksz >= caj->keystackcap)
	{
		newcap = caj->keystacksz * 2 + 16;
		newstack = realloc(caj->keystack, newcap * sizeof(*newstack));
		if (newstack == NULL)
		{
			return -ENOMEM;
		}
		caj->keystack = newstack;
		caj->keystackcap = newcap;
	}

	if (caj->keypresent == 0)
	{
		caj->keystack[caj->keystacksz].key = NULL;
		caj->keystack[caj->keystacksz].keysz = 0;
		caj->keystacksz++;
		caj->keypresent = 0;
		return 0;
	}

	newcap = caj->keysz + 1;
	newbuf = malloc(newcap);
	if (newbuf == NULL)
	{
		return -ENOMEM;
	}
	memcpy(newbuf, caj->key, caj->keysz);
	newbuf[caj->keysz] = '\0';
	caj->keystack[caj->keystacksz].key = newbuf;
	caj->keystack[caj->keystacksz].keysz = caj->keysz;
	caj->keystacksz++;
	return 0;
}

static int abce_pullcaj_put_keystack_1(struct abce_pullcaj_ctx *caj)
{
	size_t newcap;
	char *newbuf;
	struct abce_caj_keystack_item *newstack;
	if (caj->keystacksz >= caj->keystackcap)
	{
		newcap = caj->keystacksz * 2 + 16;
		newstack = realloc(caj->keystack, newcap * sizeof(*newstack));
		if (newstack == NULL)
		{
			return -ENOMEM;
		}
		caj->keystack = newstack;
		caj->keystackcap = newcap;
	}

	if (caj->keypresent == 0)
	{
		caj->keystack[caj->keystacksz].key = NULL;
		caj->keystack[caj->keystacksz].keysz = 0;
		caj->keystacksz++;
		caj->keypresent = 0;
		return 0;
	}

	newcap = caj->keysz + 1;
	newbuf = malloc(newcap);
	if (newbuf == NULL)
	{
		return -ENOMEM;
	}
	memcpy(newbuf, caj->key, caj->keysz);
	newbuf[caj->keysz] = '\0';
	caj->keystack[caj->keystacksz].key = newbuf;
	caj->keystack[caj->keystacksz].keysz = caj->keysz;
	caj->keystacksz++;
	return 0;
}

static inline void abce_caj_put_keystack_2(struct abce_caj_ctx *caj)
{
	caj->keypresent = 0;
}

static inline void abce_pullcaj_put_keystack_2(struct abce_pullcaj_ctx *caj)
{
	caj->keypresent = 0;
}

static inline size_t abce_caj_get_keysz(struct abce_caj_ctx *caj)
{
	if (caj->keypresent == 0)
	{
		return 0;
	}
	return caj->keysz;
}
static inline char *abce_caj_get_key(struct abce_caj_ctx *caj)
{
	if (caj->keypresent == 0)
	{
		return NULL;
	}
	return caj->key;
}

static inline size_t abce_pullcaj_get_keysz(struct abce_pullcaj_ctx *caj)
{
	if (caj->keypresent == 0)
	{
		return 0;
	}
	return caj->keysz;
}
static inline char *abce_pullcaj_get_key(struct abce_pullcaj_ctx *caj)
{
	if (caj->keypresent == 0)
	{
		return NULL;
	}
	return caj->key;
}

int abce_caj_feed(struct abce_caj_ctx *caj, const void *vdata, size_t usz, int eof)
{
	const unsigned char *data = (const unsigned char*)vdata;
	const char *cdata = (const char*)vdata;
	ssize_t sz = (ssize_t)usz;
	ssize_t i;
	int ret;
	if (sz < 0 || (size_t)sz != usz)
	{
		return -EFAULT;
	}
	for (i = 0; i < sz; i++)
	{
		if (caj->mode == ABCE_CAJ_MODE_KEYSTRING)
		{
			if (data[i] == '\\')
			{
				caj->mode = ABCE_CAJ_MODE_KEYSTRING_ESCAPE;
			}
			else if (data[i] == '"')
			{
				if (abce_caj_put_key(caj, '\0') != 0)
				{
					return -ENOMEM;
				}
				caj->keysz--;
				caj->keypresent = 1;
				caj->mode = ABCE_CAJ_MODE_COLON;
			}
			else
			{
				if (abce_caj_put_key(caj, (char)data[i]) != 0)
				{
					return -ENOMEM;
				}
			}
			continue;
		}
		else if (caj->mode == ABCE_CAJ_MODE_STRING)
		{
			if (data[i] == '\\')
			{
				caj->mode = ABCE_CAJ_MODE_STRING_ESCAPE;
			}
			else if (data[i] == '"')
			{
				if (abce_caj_put_val(caj, '\0') != 0)
				{
					return -ENOMEM;
				}
				caj->valsz--;
				caj->mode = ABCE_CAJ_MODE_COMMA;
				if (caj->handler->vtable->handle_string == NULL)
				{
					if (caj->keystacksz <= 0)
					{
						i++;
						while (i < sz)
						{
							if ((data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
							     data[i] == '\t'))
							{
								i++;
								continue;
							}
							return -EOVERFLOW;
						}
						return 0;
					}
					continue;
				}
				ret = caj->handler->vtable->handle_string(
					caj->handler,
					abce_caj_get_key(caj), abce_caj_get_keysz(caj),
					caj->val, caj->valsz);
				if (ret != 0)
				{
					return ret;
				}
				if (caj->keystacksz <= 0)
				{
					i++;
					while (i < sz)
					{
						if ((data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
						     data[i] == '\t'))
						{
							i++;
							continue;
						}
						return -EOVERFLOW;
					}
					return 0;
				}
			}
			else
			{
				if (abce_caj_put_val(caj, (char)data[i]) != 0)
				{
					return -ENOMEM;
				}
			}
			continue;
		}
		else if (caj->mode == ABCE_CAJ_MODE_KEYSTRING_ESCAPE)
		{
			int res = 0;
			switch (data[i])
			{
				case 'b':
					res = abce_caj_put_key(caj, '\b');
					break;
				case 'f':
					res = abce_caj_put_key(caj, '\f');
					break;
				case 'n':
					res = abce_caj_put_key(caj, '\n');
					break;
				case 'r':
					res = abce_caj_put_key(caj, '\r');
					break;
				case 't':
					res = abce_caj_put_key(caj, '\t');
					break;
				case 'u':
					caj->mode = ABCE_CAJ_MODE_KEYSTRING_UESCAPE;
					caj->sz = 0;
					break;
				default:
					return -EILSEQ;
			}
			if (res != 0)
			{
				return res;
			}
			continue;
		}
		else if (caj->mode == ABCE_CAJ_MODE_KEYSTRING_UESCAPE)
		{
			unsigned long codepoint;
			if (!isxdigit((unsigned char)data[i]))
			{
				return -EILSEQ;
			}
			caj->uescape[caj->sz++] = (char)data[i];
			if (caj->sz < 4)
			{
				continue;
			}
			caj->uescape[caj->sz] = '\0';
			codepoint = strtoul(caj->uescape, NULL, 16);
			if (codepoint < 0x80)
			{
				if (abce_caj_put_key(caj, (char)codepoint) != 0)
				{
					return -ENOMEM;
				}
				caj->mode = ABCE_CAJ_MODE_KEYSTRING;
				continue;
			}
			if (codepoint < 0x800)
			{
				if (abce_caj_put_key(caj, (char)(0xC0 | (codepoint>>6))) != 0)
				{
					return -ENOMEM;
				}
				if (abce_caj_put_key(caj, (char)(0x80 | (codepoint & 0x3F))) != 0)
				{
					return -ENOMEM;
				}
				caj->mode = ABCE_CAJ_MODE_KEYSTRING;
				continue;
			}
			if (abce_caj_put_key(caj, (char)(0xE0 | (codepoint>>12))) != 0)
			{
				return -ENOMEM;
			}
			if (abce_caj_put_key(caj, (char)(0x80 | ((codepoint>>6)&0x3F))) != 0)
			{
				return -ENOMEM;
			}
			if (abce_caj_put_key(caj, (char)(0x80 | ((codepoint>>0)&0x3F))) != 0)
			{
				return -ENOMEM;
			}
			caj->mode = ABCE_CAJ_MODE_KEYSTRING;
			continue;
		}
		else if (caj->mode == ABCE_CAJ_MODE_STRING_UESCAPE)
		{
			unsigned long codepoint;
			if (!isxdigit((unsigned char)data[i]))
			{
				return -EILSEQ;
			}
			caj->uescape[caj->sz++] = (char)data[i];
			if (caj->sz < 4)
			{
				continue;
			}
			caj->uescape[caj->sz] = '\0';
			codepoint = strtoul(caj->uescape, NULL, 16);
			if (codepoint < 0x80)
			{
				if (abce_caj_put_val(caj, (char)codepoint) != 0)
				{
					return -ENOMEM;
				}
				caj->mode = ABCE_CAJ_MODE_STRING;
				continue;
			}
			if (codepoint < 0x800)
			{
				if (abce_caj_put_val(caj, (char)(0xC0 | (codepoint>>6))) != 0)
				{
					return -ENOMEM;
				}
				if (abce_caj_put_val(caj, (char)(0x80 | (codepoint & 0x3F))) != 0)
				{
					return -ENOMEM;
				}
				caj->mode = ABCE_CAJ_MODE_STRING;
				continue;
			}
			if (abce_caj_put_val(caj, (char)(0xE0 | (codepoint>>12))) != 0)
			{
				return -ENOMEM;
			}
			if (abce_caj_put_val(caj, (char)(0x80 | ((codepoint>>6)&0x3F))) != 0)
			{
				return -ENOMEM;
			}
			if (abce_caj_put_val(caj, (char)(0x80 | ((codepoint>>0)&0x3F))) != 0)
			{
				return -ENOMEM;
			}
			caj->mode = ABCE_CAJ_MODE_STRING;
			continue;
		}
		else if (caj->mode == ABCE_CAJ_MODE_STRING_ESCAPE)
		{
			int res = 0;
			switch (data[i])
			{
				case 'b':
					res = abce_caj_put_val(caj, '\b');
					break;
				case 'f':
					res = abce_caj_put_val(caj, '\f');
					break;
				case 'n':
					res = abce_caj_put_val(caj, '\n');
					break;
				case 'r':
					res = abce_caj_put_val(caj, '\r');
					break;
				case 't':
					res = abce_caj_put_val(caj, '\t');
					break;
				case 'u':
					caj->mode = ABCE_CAJ_MODE_STRING_UESCAPE;
					caj->sz = 0;
					break;
				default:
					return -EILSEQ;
			}
			if (res != 0)
			{
				return res;
			}
			continue;
		}

		if ((data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
		     data[i] == '\t') && (
		       caj->mode == ABCE_CAJ_MODE_COLON ||
		       caj->mode == ABCE_CAJ_MODE_COMMA ||
		       caj->mode == ABCE_CAJ_MODE_FIRSTKEY ||
		       caj->mode == ABCE_CAJ_MODE_FIRSTVAL ||
		       caj->mode == ABCE_CAJ_MODE_KEY ||
		       caj->mode == ABCE_CAJ_MODE_VAL))
		{
			continue;
		}

		if (caj->mode == ABCE_CAJ_MODE_COLON)
		{
			if (data[i] != ':')
			{
				return -EINVAL;
			}
			caj->mode = ABCE_CAJ_MODE_VAL;
			continue;
		}
		if (caj->mode == ABCE_CAJ_MODE_COMMA && data[i] == ',')
		{
			if (data[i] == ',')
			{
				if (caj->keypresent)
				{
					caj->mode = ABCE_CAJ_MODE_KEY;
					caj->keypresent = 0;
				}
				else
				{
					caj->mode = ABCE_CAJ_MODE_VAL;
				}
				continue;
			}
		}
		if ((caj->mode == ABCE_CAJ_MODE_COMMA || caj->mode == ABCE_CAJ_MODE_FIRSTKEY) && data[i] == '}')
		{
			if (data[i] == '}')
			{
				if (caj->mode == ABCE_CAJ_MODE_COMMA)
				{
					if (!caj->keypresent)
					{
						return -EINVAL;
					}
					// could be array or dict
				}
				caj->mode = ABCE_CAJ_MODE_COMMA;

				if (abce_caj_get_keystack(caj) != 0)
				{
					return -ENOMEM;
				}

				if (caj->handler->vtable->end_dict == NULL)
				{
					if (caj->keystacksz <= 0)
					{
						i++;
						while (i < sz)
						{
							if ((data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
							     data[i] == '\t'))
							{
								i++;
								continue;
							}
							return -EOVERFLOW;
						}
						return 0;
					}
					continue;
				}
				ret = caj->handler->vtable->end_dict(
					caj->handler,
					abce_caj_get_key(caj), abce_caj_get_keysz(caj));
				if (ret != 0)
				{
					return ret;
				}
				if (caj->keystacksz <= 0)
				{
					i++;
					while (i < sz)
					{
						if ((data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
						     data[i] == '\t'))
						{
							i++;
							continue;
						}
						return -EOVERFLOW;
					}
					return 0;
				}
				continue;
			}
		}
		if ((caj->mode == ABCE_CAJ_MODE_COMMA || caj->mode == ABCE_CAJ_MODE_FIRSTVAL) && data[i] == ']')
		{
			if (data[i] == ']')
			{
				if (caj->mode == ABCE_CAJ_MODE_COMMA)
				{
					if (caj->keypresent)
					{
						return -EINVAL;
					}
					// could be array or dict
				}
				caj->mode = ABCE_CAJ_MODE_COMMA;

				if (abce_caj_get_keystack(caj) != 0)
				{
					return -ENOMEM;
				}

				if (caj->handler->vtable->end_array == NULL)
				{
					if (caj->keystacksz <= 0)
					{
						i++;
						while (i < sz)
						{
							if ((data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
							     data[i] == '\t'))
							{
								i++;
								continue;
							}
							return -EOVERFLOW;
						}
						return 0;
					}
					continue;
				}
				ret = caj->handler->vtable->end_array(
					caj->handler,
					abce_caj_get_key(caj), abce_caj_get_keysz(caj));
				if (ret != 0)
				{
					return ret;
				}
				if (caj->keystacksz <= 0)
				{
					i++;
					while (i < sz)
					{
						if ((data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
						     data[i] == '\t'))
						{
							i++;
							continue;
						}
						return -EOVERFLOW;
					}
					return 0;
				}
				continue;
			}
		}
		if ((caj->mode == ABCE_CAJ_MODE_FIRSTVAL || caj->mode == ABCE_CAJ_MODE_VAL) && data[i] == '{')
		{
			if (abce_caj_put_keystack_1(caj) != 0)
			{
				return -ENOMEM;
			}
			caj->mode = ABCE_CAJ_MODE_FIRSTKEY;

			if (caj->handler->vtable->start_dict == NULL)
			{
				abce_caj_put_keystack_2(caj);
				continue;
			}
			ret = caj->handler->vtable->start_dict(
				caj->handler,
				abce_caj_get_key(caj), abce_caj_get_keysz(caj));
			abce_caj_put_keystack_2(caj);
			if (ret != 0)
			{
				return ret;
			}
			continue;
		}
		if ((caj->mode == ABCE_CAJ_MODE_FIRSTVAL || caj->mode == ABCE_CAJ_MODE_VAL) && data[i] == '[')
		{
			if (abce_caj_put_keystack_1(caj) != 0)
			{
				return -ENOMEM;
			}
			caj->mode = ABCE_CAJ_MODE_FIRSTVAL;

			if (caj->handler->vtable->start_array == NULL)
			{
				abce_caj_put_keystack_2(caj);
				continue;
			}
			ret = caj->handler->vtable->start_array(
				caj->handler,
				abce_caj_get_key(caj), abce_caj_get_keysz(caj));
			abce_caj_put_keystack_2(caj);
			if (ret != 0)
			{
				return ret;
			}
			continue;
		}

		if (caj->mode == ABCE_CAJ_MODE_TRUE)
		{
			if (data[i] != "true"[caj->sz++])
			{
				return -EINVAL;
			}
			if (caj->sz < 4)
			{
				continue;
			}
			caj->mode = ABCE_CAJ_MODE_COMMA;
			if (caj->handler->vtable->handle_boolean == NULL)
			{
				if (caj->keystacksz <= 0)
				{
					i++;
					while (i < sz)
					{
						if ((data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
						     data[i] == '\t'))
						{
							i++;
							continue;
						}
						return -EOVERFLOW;
					}
					return 0;
				}
				continue;
			}
			ret = caj->handler->vtable->handle_boolean(
				caj->handler,
				abce_caj_get_key(caj), abce_caj_get_keysz(caj),
				1);
			if (ret != 0)
			{
				return ret;
			}
			if (caj->keystacksz <= 0)
			{
				i++;
				while (i < sz)
				{
					if ((data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
					     data[i] == '\t'))
					{
						i++;
						continue;
					}
					return -EOVERFLOW;
				}
				return 0;
			}
			continue;
		}
		else if (caj->mode == ABCE_CAJ_MODE_FALSE)
		{
			if (data[i] != "false"[caj->sz++])
			{
				return -EINVAL;
			}
			if (caj->sz < 5)
			{
				continue;
			}
			caj->mode = ABCE_CAJ_MODE_COMMA;
			if (caj->handler->vtable->handle_boolean == NULL)
			{
				if (caj->keystacksz <= 0)
				{
					i++;
					while (i < sz)
					{
						if ((data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
						     data[i] == '\t'))
						{
							i++;
							continue;
						}
						return -EOVERFLOW;
					}
					return 0;
				}
				continue;
			}
			ret = caj->handler->vtable->handle_boolean(
				caj->handler,
				abce_caj_get_key(caj), abce_caj_get_keysz(caj),
				0);
			if (ret != 0)
			{
				return ret;
			}
			if (caj->keystacksz <= 0)
			{
				i++;
				while (i < sz)
				{
					if ((data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
					     data[i] == '\t'))
					{
						i++;
						continue;
					}
					return -EOVERFLOW;
				}
				return 0;
			}
			continue;
		}
		else if (caj->mode == ABCE_CAJ_MODE_NULL)
		{
			if (data[i] != "null"[caj->sz++])
			{
				return -EINVAL;
			}
			if (caj->sz < 4)
			{
				continue;
			}
			caj->mode = ABCE_CAJ_MODE_COMMA;
			if (caj->handler->vtable->handle_null == NULL)
			{
				if (caj->keystacksz <= 0)
				{
					i++;
					while (i < sz)
					{
						if ((data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
						     data[i] == '\t'))
						{
							i++;
							continue;
						}
						return -EOVERFLOW;
					}
					return 0;
				}
				continue;
			}
			ret = caj->handler->vtable->handle_null(
				caj->handler,
				abce_caj_get_key(caj), abce_caj_get_keysz(caj));
			if (ret != 0)
			{
				return ret;
			}
			if (caj->keystacksz <= 0)
			{
				i++;
				while (i < sz)
				{
					if ((data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
					     data[i] == '\t'))
					{
						i++;
						continue;
					}
					return -EOVERFLOW;
				}
				return 0;
			}
			continue;
		}
		if ((caj->mode == ABCE_CAJ_MODE_VAL || caj->mode == ABCE_CAJ_MODE_FIRSTVAL) && data[i] == 'n')
		{
			caj->mode = ABCE_CAJ_MODE_NULL;
			caj->sz = 1;
			continue;
		}
		if ((caj->mode == ABCE_CAJ_MODE_VAL || caj->mode == ABCE_CAJ_MODE_FIRSTVAL) && data[i] == 'f')
		{
			caj->mode = ABCE_CAJ_MODE_FALSE;
			caj->sz = 1;
			continue;
		}
		if ((caj->mode == ABCE_CAJ_MODE_VAL || caj->mode == ABCE_CAJ_MODE_FIRSTVAL) && data[i] == 't')
		{
			caj->mode = ABCE_CAJ_MODE_TRUE;
			caj->sz = 1;
			continue;
		}
		if ((caj->mode == ABCE_CAJ_MODE_KEY || caj->mode == ABCE_CAJ_MODE_FIRSTKEY) && data[i] == '"')
		{
			caj->mode = ABCE_CAJ_MODE_KEYSTRING;
			caj->keysz = 0;
			continue;
		}
		if ((caj->mode == ABCE_CAJ_MODE_VAL || caj->mode == ABCE_CAJ_MODE_FIRSTVAL) && data[i] == '"')
		{
			caj->mode = ABCE_CAJ_MODE_STRING;
			caj->valsz = 0;
			continue;
		}
		if ((caj->mode == ABCE_CAJ_MODE_VAL || caj->mode == ABCE_CAJ_MODE_FIRSTVAL) && (isdigit((unsigned char)data[i]) || data[i] == '-'))
		{
			caj->mode = ABCE_CAJ_MODE_NUMBER;
			caj->is_integer = 1;
			abce_streaming_atof_init_strict_json(&caj->streamingatof);
		}
		if (caj->mode == ABCE_CAJ_MODE_NUMBER)
		{
			size_t tofeed;
			ssize_t szret;
			ssize_t j;
			tofeed = (size_t)(sz - i);
			szret = abce_streaming_atof_feed(&caj->streamingatof, &cdata[i], tofeed);
			if (szret < 0)
			{
				return -EINVAL;
			}
			if (szret > sz - i)
			{
				abort();
			}
			for (j = i; j < i + szret; j++)
			{
				if (cdata[j] == '.' || cdata[j] == 'e')
				{
					caj->is_integer = 0;
				}
			}
			if (szret < sz - i)
			{
				caj->mode = ABCE_CAJ_MODE_COMMA;
				if (abce_streaming_atof_is_error(&caj->streamingatof))
				{
					return -EINVAL;
				}
				if (caj->handler->vtable->handle_number == NULL)
				{
					i += szret;
					i--;
					if (caj->keystacksz <= 0)
					{
						i++;
						while (i < sz)
						{
							if ((data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
							     data[i] == '\t'))
							{
								i++;
								continue;
							}
							return -EOVERFLOW;
						}
						return 0;
					}
					continue;
				}
				ret = caj->handler->vtable->handle_number(
					caj->handler,
					abce_caj_get_key(caj), abce_caj_get_keysz(caj),
					abce_streaming_atof_end(&caj->streamingatof),
					caj->is_integer);
				if (ret != 0)
				{
					return ret;
				}
				i += szret;
				i--;
				if (caj->keystacksz <= 0)
				{
					i++;
					while (i < sz)
					{
						if ((data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
						     data[i] == '\t'))
						{
							i++;
							continue;
						}
						return -EOVERFLOW;
					}
					return 0;
				}
			}
			else
			{
				i += szret;
				i--;
			}
			continue;
		}
		return -EINVAL;
	}
	if (caj->mode == ABCE_CAJ_MODE_NUMBER && eof)
	{
		caj->mode = ABCE_CAJ_MODE_COMMA;
		if (caj->handler->vtable->handle_number == NULL)
		{
			if (caj->keystacksz <= 0)
			{
				return 0;
			}
			return -EINPROGRESS;
		}
		ret = caj->handler->vtable->handle_number(
			caj->handler,
			abce_caj_get_key(caj), abce_caj_get_keysz(caj),
			abce_streaming_atof_end(&caj->streamingatof),
			caj->is_integer);
		if (ret != 0)
		{
			return ret;
		}
		if (caj->keystacksz <= 0)
		{
			return 0;
		}
		return -EINPROGRESS;
	}
	return -EINPROGRESS;
}

int abce_pullcaj_set_buf(struct abce_pullcaj_ctx *pc, const void *vdata, size_t usz, int eof)
{
	ssize_t sz = (ssize_t)usz;
	if (vdata == NULL && usz > 0)
	{
		return -EFAULT;
	}
	if (sz < 0 || (size_t)sz != usz)
	{
		return -EFAULT;
	}
	if (pc->eof)
	{
		return -EFAULT;
	}
	if (pc->i < pc->usz)
	{
		return -EINTR;
	}
	pc->i = 0;
	pc->usz = usz;
	pc->vdata = vdata;
	pc->eof = eof;
	return 0;
}

int abce_pullcaj_get_event(struct abce_pullcaj_ctx *caj, struct abce_pullcaj_event_info *ev)
{
	const unsigned char *data = (const unsigned char*)caj->vdata;
	const char *cdata = (const char*)caj->vdata;
	switch(caj->state) {
		case 0:
			break;
		case 1:
			goto state1;
		case 2:
			goto state2;
		case 3:
			goto state3;
		case 4:
			goto state4;
		case 5:
			goto state5;
		case 6:
			goto state6;
		case 7:
			goto state7;
		case 8:
			goto state8;
		case 9:
			goto state9;
		case 10:
			goto state10;
		default:
			abort();
	}
	for (; caj->i < caj->usz; caj->i++)
	{
		if (caj->mode == ABCE_CAJ_MODE_KEYSTRING)
		{
			if (data[caj->i] == '\\')
			{
				caj->mode = ABCE_CAJ_MODE_KEYSTRING_ESCAPE;
			}
			else if (data[caj->i] == '"')
			{
				if (abce_pullcaj_put_key(caj, '\0') != 0)
				{
					return -ENOMEM;
				}
				caj->keysz--;
				caj->keypresent = 1;
				caj->mode = ABCE_CAJ_MODE_COLON;
			}
			else
			{
				if (abce_pullcaj_put_key(caj, (char)data[caj->i]) != 0)
				{
					return -ENOMEM;
				}
			}
			continue;
		}
		else if (caj->mode == ABCE_CAJ_MODE_STRING)
		{
			if (data[caj->i] == '\\')
			{
				caj->mode = ABCE_CAJ_MODE_STRING_ESCAPE;
			}
			else if (data[caj->i] == '"')
			{
				if (abce_pullcaj_put_val(caj, '\0') != 0)
				{
					return -ENOMEM;
				}
				caj->valsz--;
				caj->mode = ABCE_CAJ_MODE_COMMA;
				ev->ev = ABCE_CAJ_EV_STR;
				ev->key = abce_pullcaj_get_key(caj);
				ev->keysz = abce_pullcaj_get_keysz(caj);
				ev->u.str.val = caj->val;
				ev->u.str.valsz = caj->valsz;
				caj->state = 1;
				return 1;
state1:
				if (caj->keystacksz <= 0)
				{
					caj->i++;
					while (caj->i < caj->usz)
					{
						if ((data[caj->i] == ' ' || data[caj->i] == '\n' || data[caj->i] == '\r' ||
						     data[caj->i] == '\t'))
						{
							caj->i++;
							continue;
						}
						return -EOVERFLOW;
					}
					return 0;
				}
			}
			else
			{
				if (abce_pullcaj_put_val(caj, (char)data[caj->i]) != 0)
				{
					return -ENOMEM;
				}
			}
			continue;
		}
		else if (caj->mode == ABCE_CAJ_MODE_KEYSTRING_ESCAPE)
		{
			int res = 0;
			switch (data[caj->i])
			{
				case 'b':
					res = abce_pullcaj_put_key(caj, '\b');
					break;
				case 'f':
					res = abce_pullcaj_put_key(caj, '\f');
					break;
				case 'n':
					res = abce_pullcaj_put_key(caj, '\n');
					break;
				case 'r':
					res = abce_pullcaj_put_key(caj, '\r');
					break;
				case 't':
					res = abce_pullcaj_put_key(caj, '\t');
					break;
				case 'u':
					caj->mode = ABCE_CAJ_MODE_KEYSTRING_UESCAPE;
					caj->sz = 0;
					break;
				default:
					return -EILSEQ;
			}
			if (res != 0)
			{
				return res;
			}
			continue;
		}
		else if (caj->mode == ABCE_CAJ_MODE_KEYSTRING_UESCAPE)
		{
			unsigned long codepoint;
			if (!isxdigit((unsigned char)data[caj->i]))
			{
				return -EILSEQ;
			}
			caj->uescape[caj->sz++] = (char)data[caj->i];
			if (caj->sz < 4)
			{
				continue;
			}
			caj->uescape[caj->sz] = '\0';
			codepoint = strtoul(caj->uescape, NULL, 16);
			if (codepoint < 0x80)
			{
				if (abce_pullcaj_put_key(caj, (char)codepoint) != 0)
				{
					return -ENOMEM;
				}
				caj->mode = ABCE_CAJ_MODE_KEYSTRING;
				continue;
			}
			if (codepoint < 0x800)
			{
				if (abce_pullcaj_put_key(caj, (char)(0xC0 | (codepoint>>6))) != 0)
				{
					return -ENOMEM;
				}
				if (abce_pullcaj_put_key(caj, (char)(0x80 | (codepoint & 0x3F))) != 0)
				{
					return -ENOMEM;
				}
				caj->mode = ABCE_CAJ_MODE_KEYSTRING;
				continue;
			}
			if (abce_pullcaj_put_key(caj, (char)(0xE0 | (codepoint>>12))) != 0)
			{
				return -ENOMEM;
			}
			if (abce_pullcaj_put_key(caj, (char)(0x80 | ((codepoint>>6)&0x3F))) != 0)
			{
				return -ENOMEM;
			}
			if (abce_pullcaj_put_key(caj, (char)(0x80 | ((codepoint>>0)&0x3F))) != 0)
			{
				return -ENOMEM;
			}
			caj->mode = ABCE_CAJ_MODE_KEYSTRING;
			continue;
		}
		else if (caj->mode == ABCE_CAJ_MODE_STRING_UESCAPE)
		{
			unsigned long codepoint;
			if (!isxdigit((unsigned char)data[caj->i]))
			{
				return -EILSEQ;
			}
			caj->uescape[caj->sz++] = (char)data[caj->i];
			if (caj->sz < 4)
			{
				continue;
			}
			caj->uescape[caj->sz] = '\0';
			codepoint = strtoul(caj->uescape, NULL, 16);
			if (codepoint < 0x80)
			{
				if (abce_pullcaj_put_val(caj, (char)codepoint) != 0)
				{
					return -ENOMEM;
				}
				caj->mode = ABCE_CAJ_MODE_STRING;
				continue;
			}
			if (codepoint < 0x800)
			{
				if (abce_pullcaj_put_val(caj, (char)(0xC0 | (codepoint>>6))) != 0)
				{
					return -ENOMEM;
				}
				if (abce_pullcaj_put_val(caj, (char)(0x80 | (codepoint & 0x3F))) != 0)
				{
					return -ENOMEM;
				}
				caj->mode = ABCE_CAJ_MODE_STRING;
				continue;
			}
			if (abce_pullcaj_put_val(caj, (char)(0xE0 | (codepoint>>12))) != 0)
			{
				return -ENOMEM;
			}
			if (abce_pullcaj_put_val(caj, (char)(0x80 | ((codepoint>>6)&0x3F))) != 0)
			{
				return -ENOMEM;
			}
			if (abce_pullcaj_put_val(caj, (char)(0x80 | ((codepoint>>0)&0x3F))) != 0)
			{
				return -ENOMEM;
			}
			caj->mode = ABCE_CAJ_MODE_STRING;
			continue;
		}
		else if (caj->mode == ABCE_CAJ_MODE_STRING_ESCAPE)
		{
			int res = 0;
			switch (data[caj->i])
			{
				case 'b':
					res = abce_pullcaj_put_val(caj, '\b');
					break;
				case 'f':
					res = abce_pullcaj_put_val(caj, '\f');
					break;
				case 'n':
					res = abce_pullcaj_put_val(caj, '\n');
					break;
				case 'r':
					res = abce_pullcaj_put_val(caj, '\r');
					break;
				case 't':
					res = abce_pullcaj_put_val(caj, '\t');
					break;
				case 'u':
					caj->mode = ABCE_CAJ_MODE_STRING_UESCAPE;
					caj->sz = 0;
					break;
				default:
					return -EILSEQ;
			}
			if (res != 0)
			{
				return res;
			}
			continue;
		}

		if ((data[caj->i] == ' ' || data[caj->i] == '\n' || data[caj->i] == '\r' ||
		     data[caj->i] == '\t') && (
		       caj->mode == ABCE_CAJ_MODE_COLON ||
		       caj->mode == ABCE_CAJ_MODE_COMMA ||
		       caj->mode == ABCE_CAJ_MODE_FIRSTKEY ||
		       caj->mode == ABCE_CAJ_MODE_FIRSTVAL ||
		       caj->mode == ABCE_CAJ_MODE_KEY ||
		       caj->mode == ABCE_CAJ_MODE_VAL))
		{
			continue;
		}

		if (caj->mode == ABCE_CAJ_MODE_COLON)
		{
			if (data[caj->i] != ':')
			{
				return -EINVAL;
			}
			caj->mode = ABCE_CAJ_MODE_VAL;
			continue;
		}
		if (caj->mode == ABCE_CAJ_MODE_COMMA && data[caj->i] == ',')
		{
			if (data[caj->i] == ',')
			{
				if (caj->keypresent)
				{
					caj->mode = ABCE_CAJ_MODE_KEY;
					caj->keypresent = 0;
				}
				else
				{
					caj->mode = ABCE_CAJ_MODE_VAL;
				}
				continue;
			}
		}
		if ((caj->mode == ABCE_CAJ_MODE_COMMA || caj->mode == ABCE_CAJ_MODE_FIRSTKEY) && data[caj->i] == '}')
		{
			if (data[caj->i] == '}')
			{
				if (caj->mode == ABCE_CAJ_MODE_COMMA)
				{
					if (!caj->keypresent)
					{
						return -EINVAL;
					}
					// could be array or dict
				}
				caj->mode = ABCE_CAJ_MODE_COMMA;

				if (abce_pullcaj_get_keystack(caj) != 0)
				{
					return -ENOMEM;
				}

				ev->ev = ABCE_CAJ_EV_END_DICT;
				ev->key = abce_pullcaj_get_key(caj);
				ev->keysz = abce_pullcaj_get_keysz(caj);
				caj->state = 2;
				return 1;
state2:
				if (caj->keystacksz <= 0)
				{
					caj->i++;
					while (caj->i < caj->usz)
					{
						if ((data[caj->i] == ' ' || data[caj->i] == '\n' || data[caj->i] == '\r' ||
						     data[caj->i] == '\t'))
						{
							caj->i++;
							continue;
						}
						return -EOVERFLOW;
					}
					return 0;
				}
				continue;
			}
		}
		if ((caj->mode == ABCE_CAJ_MODE_COMMA || caj->mode == ABCE_CAJ_MODE_FIRSTVAL) && data[caj->i] == ']')
		{
			if (data[caj->i] == ']')
			{
				if (caj->mode == ABCE_CAJ_MODE_COMMA)
				{
					if (caj->keypresent)
					{
						return -EINVAL;
					}
					// could be array or dict
				}
				caj->mode = ABCE_CAJ_MODE_COMMA;

				if (abce_pullcaj_get_keystack(caj) != 0)
				{
					return -ENOMEM;
				}

				ev->ev = ABCE_CAJ_EV_END_ARRAY;
				ev->key = abce_pullcaj_get_key(caj);
				ev->keysz = abce_pullcaj_get_keysz(caj);
				caj->state = 3;
				return 1;
state3:
				if (caj->keystacksz <= 0)
				{
					caj->i++;
					while (caj->i < caj->sz)
					{
						if ((data[caj->i] == ' ' || data[caj->i] == '\n' || data[caj->i] == '\r' ||
						     data[caj->i] == '\t'))
						{
							caj->i++;
							continue;
						}
						return -EOVERFLOW;
					}
					return 0;
				}
				continue;
			}
		}
		if ((caj->mode == ABCE_CAJ_MODE_FIRSTVAL || caj->mode == ABCE_CAJ_MODE_VAL) && data[caj->i] == '{')
		{
			if (abce_pullcaj_put_keystack_1(caj) != 0)
			{
				return -ENOMEM;
			}
			caj->mode = ABCE_CAJ_MODE_FIRSTKEY;

			ev->ev = ABCE_CAJ_EV_START_DICT;
			ev->key = abce_pullcaj_get_key(caj);
			ev->keysz = abce_pullcaj_get_keysz(caj);
			caj->state = 4;
			return 1;
state4:
			abce_pullcaj_put_keystack_2(caj);
			continue;
		}
		if ((caj->mode == ABCE_CAJ_MODE_FIRSTVAL || caj->mode == ABCE_CAJ_MODE_VAL) && data[caj->i] == '[')
		{
			if (abce_pullcaj_put_keystack_1(caj) != 0)
			{
				return -ENOMEM;
			}
			caj->mode = ABCE_CAJ_MODE_FIRSTVAL;

			ev->ev = ABCE_CAJ_EV_START_ARRAY;
			ev->key = abce_pullcaj_get_key(caj);
			ev->keysz = abce_pullcaj_get_keysz(caj);
			caj->state = 5;
			return 1;
state5:
			abce_pullcaj_put_keystack_2(caj);
			continue;
		}

		if (caj->mode == ABCE_CAJ_MODE_TRUE)
		{
			if (data[caj->i] != "true"[caj->sz++])
			{
				return -EINVAL;
			}
			if (caj->sz < 4)
			{
				continue;
			}
			caj->mode = ABCE_CAJ_MODE_COMMA;
			ev->ev = ABCE_CAJ_EV_BOOL;
			ev->key = abce_pullcaj_get_key(caj);
			ev->keysz = abce_pullcaj_get_keysz(caj);
			ev->u.b.b = 1;
			caj->state = 6;
			return 1;
state6:
			if (caj->keystacksz <= 0)
			{
				caj->i++;
				while (caj->i < caj->usz)
				{
					if ((data[caj->i] == ' ' || data[caj->i] == '\n' || data[caj->i] == '\r' ||
					     data[caj->i] == '\t'))
					{
						caj->i++;
						continue;
					}
					return -EOVERFLOW;
				}
				return 0;
			}
			continue;
		}
		else if (caj->mode == ABCE_CAJ_MODE_FALSE)
		{
			if (data[caj->i] != "false"[caj->sz++])
			{
				return -EINVAL;
			}
			if (caj->sz < 5)
			{
				continue;
			}
			caj->mode = ABCE_CAJ_MODE_COMMA;
			ev->ev = ABCE_CAJ_EV_BOOL;
			ev->key = abce_pullcaj_get_key(caj);
			ev->keysz = abce_pullcaj_get_keysz(caj);
			ev->u.b.b = 0;
			caj->state = 7;
			return 1;
state7:
			if (caj->keystacksz <= 0)
			{
				caj->i++;
				while (caj->i < caj->usz)
				{
					if ((data[caj->i] == ' ' || data[caj->i] == '\n' || data[caj->i] == '\r' ||
					     data[caj->i] == '\t'))
					{
						caj->i++;
						continue;
					}
					return -EOVERFLOW;
				}
				return 0;
			}
			continue;
		}
		else if (caj->mode == ABCE_CAJ_MODE_NULL)
		{
			if (data[caj->i] != "null"[caj->sz++])
			{
				return -EINVAL;
			}
			if (caj->sz < 4)
			{
				continue;
			}
			caj->mode = ABCE_CAJ_MODE_COMMA;
			ev->ev = ABCE_CAJ_EV_NULL;
			ev->key = abce_pullcaj_get_key(caj);
			ev->keysz = abce_pullcaj_get_keysz(caj);
			caj->state = 8;
			return 1;
state8:
			if (caj->keystacksz <= 0)
			{
				caj->i++;
				while (caj->i < caj->usz)
				{
					if ((data[caj->i] == ' ' || data[caj->i] == '\n' || data[caj->i] == '\r' ||
					     data[caj->i] == '\t'))
					{
						caj->i++;
						continue;
					}
					return -EOVERFLOW;
				}
				return 0;
			}
			continue;
		}
		if ((caj->mode == ABCE_CAJ_MODE_VAL || caj->mode == ABCE_CAJ_MODE_FIRSTVAL) && data[caj->i] == 'n')
		{
			caj->mode = ABCE_CAJ_MODE_NULL;
			caj->sz = 1;
			continue;
		}
		if ((caj->mode == ABCE_CAJ_MODE_VAL || caj->mode == ABCE_CAJ_MODE_FIRSTVAL) && data[caj->i] == 'f')
		{
			caj->mode = ABCE_CAJ_MODE_FALSE;
			caj->sz = 1;
			continue;
		}
		if ((caj->mode == ABCE_CAJ_MODE_VAL || caj->mode == ABCE_CAJ_MODE_FIRSTVAL) && data[caj->i] == 't')
		{
			caj->mode = ABCE_CAJ_MODE_TRUE;
			caj->sz = 1;
			continue;
		}
		if ((caj->mode == ABCE_CAJ_MODE_KEY || caj->mode == ABCE_CAJ_MODE_FIRSTKEY) && data[caj->i] == '"')
		{
			caj->mode = ABCE_CAJ_MODE_KEYSTRING;
			caj->keysz = 0;
			continue;
		}
		if ((caj->mode == ABCE_CAJ_MODE_VAL || caj->mode == ABCE_CAJ_MODE_FIRSTVAL) && data[caj->i] == '"')
		{
			caj->mode = ABCE_CAJ_MODE_STRING;
			caj->valsz = 0;
			continue;
		}
		if ((caj->mode == ABCE_CAJ_MODE_VAL || caj->mode == ABCE_CAJ_MODE_FIRSTVAL) && (isdigit((unsigned char)data[caj->i]) || data[caj->i] == '-'))
		{
			caj->mode = ABCE_CAJ_MODE_NUMBER;
			caj->is_integer = 1;
			abce_streaming_atof_init_strict_json(&caj->streamingatof);
		}
		if (caj->mode == ABCE_CAJ_MODE_NUMBER)
		{
			size_t tofeed;
			ssize_t szret;
			ssize_t j;
			tofeed = (size_t)(caj->usz - caj->i);
			szret = abce_streaming_atof_feed(&caj->streamingatof, &cdata[caj->i], tofeed);
			if (szret < 0)
			{
				return -EINVAL;
			}
			if (szret > (ssize_t)(caj->usz - caj->i))
			{
				abort();
			}
			for (j = (ssize_t)caj->i; j < (ssize_t)caj->i + szret; j++)
			{
				if (cdata[j] == '.' || cdata[j] == 'e')
				{
					caj->is_integer = 0;
				}
			}
			if (szret < (ssize_t)(caj->usz - caj->i))
			{
				caj->mode = ABCE_CAJ_MODE_COMMA;
				if (abce_streaming_atof_is_error(&caj->streamingatof))
				{
					return -EINVAL;
				}
				ev->ev = ABCE_CAJ_EV_NUM;
				ev->key = abce_pullcaj_get_key(caj);
				ev->keysz = abce_pullcaj_get_keysz(caj);
				ev->u.num.d = abce_streaming_atof_end(&caj->streamingatof);
				ev->u.num.is_integer = caj->is_integer;
				caj->state = 9;
				caj->i += (size_t)szret;
				caj->i--;
				return 1;
state9:
				if (caj->keystacksz <= 0)
				{
					caj->i++;
					while (caj->i < caj->usz)
					{
						if ((data[caj->i] == ' ' || data[caj->i] == '\n' || data[caj->i] == '\r' ||
						     data[caj->i] == '\t'))
						{
							caj->i++;
							continue;
						}
						return -EOVERFLOW;
					}
					return 0;
				}
			}
			else
			{
				caj->i += (size_t)szret;
				caj->i--;
			}
			continue;
		}
		return -EINVAL;
	}
	if (caj->mode == ABCE_CAJ_MODE_NUMBER && caj->eof)
	{
		caj->mode = ABCE_CAJ_MODE_COMMA;
		ev->ev = ABCE_CAJ_EV_NUM;
		ev->key = abce_pullcaj_get_key(caj);
		ev->keysz = abce_pullcaj_get_keysz(caj);
		ev->u.num.d = abce_streaming_atof_end(&caj->streamingatof);
		ev->u.num.is_integer = caj->is_integer;
		caj->state = 10;
		return 1;
state10:
		if (caj->keystacksz <= 0)
		{
			return 0;
		}
		caj->state = 0;
		return -EINPROGRESS;
	}
	caj->state = 0;
	return -EINPROGRESS;
}
