#include "abceprettyftoa.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <limits.h>
#include <string.h>

void abce_pretty_ftoa_fix_exponent(char *buf)
{
	char *eloc;
	int skip;

	// Standardize on lower case 'e'
	eloc = strchr(buf, 'E');
	if (eloc != NULL)
	{
		*eloc = 'e';
	}
	else
	{
		eloc = strchr(buf, 'e');
	}
	if (eloc == NULL)
	{
		abort();
	}
	skip = 0;
	if (eloc[skip + 1] == '+')
	{
		skip++;
	}
	else if (eloc[skip + 1] == '-')
	{
		eloc++;
	}
	while (eloc[skip + 1] == '0')
	{
		skip++;
	}
	if (eloc[skip + 1] == '\0')
	{
		if (eloc[0] == '-')
		{
			eloc--;
		}
		else
		{
			skip--;
		}
	}
	memcpy(&eloc[1], &eloc[skip+1], strlen(&eloc[skip+1]) + 1);
}

static void abce_ftoa_iter(char *buf, size_t bufsiz, int digits, double d,
                      size_t *plen, int *p_is_expo)
{
	const char * const fmts[] = {
		"%.1g", "%.2g", "%.3g", "%.4g", "%.5g", "%.6g", "%.7g",
		"%.8g", "%.9g", "%.10g", "%.11g", "%.12g", "%.13g",
		"%.14g", "%.15g", "%.16g", "%.17g"};
	ssize_t sbufsiz;
	const char *fmt;
	size_t len;
	int is_expo;
	// 1 character '-'
	// 18 characters mantissa with period
	// 5 characters 'e-123'
	// 1 character '\0'
	if (bufsiz < 25)
	{
		abort();
	}
	if (digits < 1 || digits > 17)
	{
		abort();
	}
	sbufsiz = (ssize_t)bufsiz;
	if (sbufsiz < 25)
	{
		abort();
	}
	if (bufsiz > SSIZE_MAX)
	{
		sbufsiz = SSIZE_MAX;
	}
	fmt = fmts[digits-1];
	if (snprintf(buf, bufsiz, fmt, d) >= sbufsiz)
	{
		abort();
	}
	len = strlen(buf);
	is_expo = (strchr(buf, 'e') != NULL || strchr(buf, 'E') != NULL);
	if (is_expo)
	{
		abce_pretty_ftoa_fix_exponent(buf);
		len = strlen(buf);
	}
	if (!is_expo && strchr(buf, '.') == NULL)
	{
		if (len >= bufsiz - 2)
		{
			abort();
		}
		// Uh-oh. Should reduce precision?
		buf[len++] = '.';
		buf[len++] = '0';
		buf[len] = '\0';
	}
	*plen = len;
	*p_is_expo = is_expo;
}

/*
 * 20.0 vs 2e+01 (4 vs 5)
 * 2000.0 vs 2e+03 (6 vs 5)
 *
 * but:
 * 201000.0 vs 2.01e+05 (8 vs 8)
 * - pick the one without exponent
 */

void abce_pretty_ftoa(char *buf, size_t bufsiz, double d)
{
	char hibound[25];
	size_t hilen;
	int hi_is_expo;
	ssize_t sbufsiz;
	char hinoexpo[25];
	char noexpo[25];
	char curbuf[25];
	size_t len;
	int is_expo;
	int cur;
	int lo, hi;

	hi = 17;
	abce_ftoa_iter(hibound, sizeof(hibound), hi, d, &hilen, &hi_is_expo);
	if (bufsiz < 25)
	{
		abort();
	}
	sbufsiz = (ssize_t)bufsiz;
	if (sbufsiz < 25)
	{
		abort();
	}
	if (bufsiz > SSIZE_MAX)
	{
		sbufsiz = SSIZE_MAX;
	}
	if (atof(hibound) != d)
	{
		// Uh-oh. Even high bound does not work.
		if (snprintf(buf, bufsiz, "%s", hibound) >= sbufsiz)
		{
			abort();
		}
		return;
	}
	/*
	 * Here we are trying to find the shortest representation of
	 * the floating-point number that does not have an exponent in
	 * the representation, but that still converts accurately to the
	 * original value.
	 */
	if (!hi_is_expo)
	{
		if (snprintf(hinoexpo, sizeof(hinoexpo), "%s", hibound) >= (int)sizeof(hinoexpo))
		{
			abort();
		}

		lo = 1;
		abce_ftoa_iter(noexpo, sizeof(noexpo), lo, d, &len, &is_expo);
		if (is_expo || atof(noexpo) != d)
		{
			// Need more precision.
			while (lo + 1 < hi)
			{
				cur = (lo + hi) / 2;
				if (cur <= lo || cur >= hi)
				{
					abort();
				}
				abce_ftoa_iter(noexpo, sizeof(noexpo), cur, d, &len, &is_expo);
				if (is_expo || atof(noexpo) != d)
				{
					lo = cur;
				}
				else
				{
					hi = cur;
					if (snprintf(hinoexpo, sizeof(hinoexpo), "%s", noexpo) >= (int)sizeof(hinoexpo))
					{
						abort();
					}
				}
			}
		}
		else
		{
			if (snprintf(hinoexpo, sizeof(hinoexpo), "%s", noexpo) >= (int)sizeof(hinoexpo))
			{
				abort();
			}
		}
	}

	lo = 1;
	hi = 17;
	abce_ftoa_iter(curbuf, sizeof(curbuf), lo, d, &len, &is_expo);
	if (!is_expo)
	{
		// We won't be successful in trying to get an exponent.
		char *copyfrom = hi_is_expo ? hibound : hinoexpo;
		if (snprintf(buf, bufsiz, "%s", copyfrom) >= sbufsiz)
		{
			abort();
		}
		return;
	}
	if (atof(curbuf) != d)
	{
		// Need more precision.
		while (lo + 1 < hi)
		{
			cur = (lo + hi) / 2;
			if (cur <= lo || cur >= hi)
			{
				abort();
			}
			abce_ftoa_iter(curbuf, sizeof(curbuf), cur, d, &len, &is_expo);
			if (atof(curbuf) != d)
			{
				lo = cur;
			}
			else
			{
				hi = cur;
				if (snprintf(hibound, sizeof(hibound), "%s", curbuf) >= (int)sizeof(hibound))
				{
					abort();
				}
			}
		}
	}
	else
	{
		if (snprintf(hibound, sizeof(hibound), "%s", curbuf) >= (int)sizeof(hibound))
		{
			abort();
		}
	}

	// Ok. Now we have two strings.
	// hibound may or may not have an exponent.
	// hinoexpo does not have an exponent.
	// Pick the shorter one. If equal pick the one with no exponent.
	if (!hi_is_expo && strlen(hinoexpo) <= strlen(hibound))
	{
		if (snprintf(buf, bufsiz, "%s", hinoexpo) >= sbufsiz)
		{
			abort();
		}
		return;
	}
	if (snprintf(buf, bufsiz, "%s", hibound) >= sbufsiz)
	{
		abort();
	}
}
