/* wmd - parameter handling
 * Copyright (C) 2009 Kristian Lyngstøl <kristian@bohemians.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* Basic parameter support for wmd.
 *
 * WMD allows setting and reading parameters from various sources (in
 * theory) and therefor needs a reasonably generic and flexible interface
 * to make sure the values are sane.
 *
 * Verify, verify, verify and then make sure that you can't access these
 * values without verifying them some more. But make it easy.
 *
 * We want as few entry-points the parameters as possible to assure a
 * consistent behavior.
 *
 * XXX: We may want to split the actual values into it's own file to avoid
 * 	mixing the values of the parameters and the framework to use it.
 *
 * XXX: A param-test framework should be included to dry-run param-changes.
 * 	This is particularly important for bindings.
 */

#include <sys/param.h>

#include "param.h"
#include "param-private.h"
#include "com.h"
#include "core.h"

/* Actual settings, keep this in sync with param.h paramlist.
 *
 * XXX: To avoid sorting-issues, we probably want to do this a bit smarter.
 * XXX: Probably also want to move this somewhere I suppose, it's
 * 	reasonably important, after all.
 */
t_param param[P_NUM] =
{
	// F_REPLACE
	{
		"replace",
		"If set to true, wmd will attempt to replace the running \n"
		"window manager, otherwise, it will exit if a window \n"
		"manager already has control over the X session.",
		PTYPE_BOOL,
		0,
		0,
	},
	// F_SYNC
	{
		"sync"
		"Run in synchronized X-mode. Easier debugging but \n"
		"slower, since we have to wait for X.",
		PTYPE_BOOL,
		0,
		0,
	},
	// F_VERBOSITY
	// XXX: When you have the function, file/line is generally just
	//	noisy.
	{
		"verbosity",
		"The verbosity bitmask.",
		PTYPE_UINT,
		(t_data)(UINT_MAX ^ (1<<VER_FILELINE)),
		(t_data)(UINT_MAX ^ (1<<VER_FILELINE)),
	},
};

/* These are static because you do not want to call the verification
 * params from outside param, instead just call the param-verification
 * functions which in turn will figure out the correct function to call.
 */
static int ptype_check_int(const paramtype type,
			   const int min,
			   const int max,
			   const t_data data);

static int ptype_check_string(const paramtype type,
			      const int min,
			      const int max,
			      const t_data data);

static int ptype_check_key(const paramtype type,
			   const int min,
			   const int max,
			   const t_data data);

/* Different param-types we support. Not all min/max values are actually
 * used, as that depends on the specific check function.
 *
 * XXX: Should other functions be allowed to fetch this? Perhaps through
 * 	a get-function? The idea is to make is as little tempting as
 * 	possible to circumvent proper param tests.
 *
 * From param.h:
 *
 * char *desc,
 * int min,
 * int max,
 * *verify
 */
static t_ptype ptype[PTYPE_NUM] = {
	{
		"Boolean",
		0,
		1,
		ptype_check_int,
	},
	{
		"Unsigned integer",
		0,
		UINT_MAX,
		ptype_check_int,
	},
	{
		"Signed integer",
		INT_MIN,
		INT_MAX,
		ptype_check_int,
	},
	{
		"Character string",
		0,
		WMD_MAX_STRING,
		ptype_check_string,
	},
	{
		"Key binding",
		0,
		0,
		ptype_check_key,
	}
};

/* Verifies that data contains an int of some sort and that it's within
 * the given range.
 *
 * XXX: Turns out that uint and int isn't the same. Perhaps using a larger
 * 	int to store the unsigned to get it within the same bounds... A bit
 * 	pointless just to avoid a tiny bit of code duplication.
 */
static int ptype_check_int(const paramtype type,
			   const int min,
			   const int max,
			   const t_data data)
{
	unsigned int uint;

	assert(PTYPE_IS_INT(type));

	if (type == PTYPE_UINT) {
		if (data.u < min || data.u > max) {
			inform(V(CONFIG), "Value of integer outside of range. "
					"(%d < %d < %d)", min, data.i, max);
			return 0;
		}
	} else {
		if (data.i < min || data.i > max) {
			inform(V(CONFIG), "Value of integer outside of range. "
					"(%d < %d < %d)", min, data.i, max);
			return 0;
		}
	}
	return 1;
}

/* Verify that *data points to a string that is no longer than max. Note
 * that there is no way (?) to verify that *data is valid before accessing
 * it, so if it points to a bogus area, we will still fall apart.
 *
 * min is ignored.
 *
 * Returns 0 for invalid string or the length of the string.
 */
static int ptype_check_string(const paramtype type,
			      const int min,
			      const int max,
			      const t_data data)
{
	int i;

	assert(type == PTYPE_STRING);
	if (data.str == NULL) {
		inform(V(CONFIG),"NULL-pointer when verifying a string");
		return 0;
	}
	
	for(i == 0; i < MAX(max, WMD_MAX_STRING); i++) {
		if (data.str[i] == '\0')
			return i;
	}
	inform(V(CONFIG),"String is larger than expected");
	return 0;
}

/* Dummy-function until keys/actions are implemented */
static int ptype_check_key(const paramtype type,
			   const int min,
			   const int max,
			   const t_data data)
{
	WMD_DUMMY_RETURN(0);
}

/* Just checks the range of p. Since this should never fail, asserts will
 * do.
 */
static void param_is_in_range(int p) {
	assert(p >= 0);
	assert(p < P_NUM);
}

/* Sanity check and verify a param and it's value based on the
 * verification function.
 *
 * Returns true/false based on success.
 */
int param_verify(int p)
{
	t_ptype *type = NULL;
	int i = 0;
	int ret = 0;
	if(p == -1) {
		for(i = 0; i < P_NUM; i++)
			ret += param_verify(i);
		return ret;
	}
	param_is_in_range(p);

	/* This should be impossible, as ptype[] is rather static.
	 * If this ever happens, we are in big trouble and should quit
	 * before it's too late.
	 */
	assert(&ptype[param[p].type] != NULL);
	assert(ptype[param[p].type].verify);

	type = &ptype[param[p].type];

	if (type->verify(param[p].type, type->min, type->max, param[p].d)) {
		return 1;
	} else {
		inform(V(CONFIG), "Parameter-verification failed for "
			"%s of type %s",
			param[p].name,
			type->desc);
		return 0;
	}
}

t_data param_get_data(t_param_enum p) {
	param_is_in_range(p);
	if (param[p].d.v == NULL)
		inform(V(CONFIG), "Fetching a null-parameter(%s).",
			param[p].name);
	return param[p].d;
}

