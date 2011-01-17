/*
 * This file tries to make up for the difference between OpenBSD's
 * <util.h> and FreeBSD's <libutil.h>.
 */

/*
 * fmt_scaled(3) specific flags. (from OpenBSD util.h)
 */
#define FMT_SCALED_STRSIZE	7	/* minus sign, 4 digits, suffix, null byte */

int	fmt_scaled(long long number, char *result);
