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
#include "core.h"

/* These are static because you do not want to call the verification
 * features from outside param, instead just call the feature-verification
 * functions which in turn will figure out the correct function to call.
 */
static int ftype_check_int(const featuretype type,
			   const int min,
			   const int max,
			   const t_data data);

static int ftype_check_string(const featuretype type,
			      const int min,
			      const int max,
			      const t_data data);

static int ftype_check_key(const featuretype type,
			   const int min,
			   const int max,
			   const t_data data);

/* Different feature-types we support. Not all min/max values are actually
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
static t_ftype ftype[FTYPE_NUM] = {
	{
		"Boolean",
		0,
		1,
		ftype_check_int,
	},
	{
		"Unsigned integer",
		0,
		UINT_MAX,
		ftype_check_int,
	},
	{
		"Signed integer",
		INT_MIN,
		INT_MAX,
		ftype_check_int,
	},
	{
		"Character string",
		0,
		KWMD_MAX_STRING,
		ftype_check_string,
	},
	{
		"Key binding",
		0,
		0,
		ftype_check_key,
	}
};

/* Actual settings, keep this in sync with param.h featurelist.
 *
 * XXX: To avoid sorting-issues, we probably want to do this a bit smarter.
 */
t_feature feature[F_NUM] =
{
	// F_REPLACE
	{
		"replace",
		"Try to replace the running WM if it supports it.",
		FTYPE_BOOL,
		0,
	},
	// F_SYNC
	{
		"sync"
		"Run in synchronized X-mode. Easier debugging but "
		"slower, since we have to wait for X.",
		FTYPE_BOOL,
		0,
	},
	// F_VERBOSITY
	// XXX: When you have the function, file/line is generally just
	//	noisy.
	{
		"verbosity",
		"The verbosity bitmask.",
		FTYPE_UINT,
		(void *)(UINT_MAX ^ VER_FILELINE),
	},
};

/* Verifies that data contains an int of some sort and that it's within
 * the given range.
 *
 * XXX: Turns out that uint and int isn't the same. Perhaps using a larger
 * 	int to store the unsigned to get it within the same bounds... A bit
 * 	pointless just to avoid a tiny bit of code duplication.
 */
static int ftype_check_int(const featuretype type,
			   const int min,
			   const int max,
			   const t_data data)
{
	unsigned int uint;

	assert(FTYPE_IS_INT(type));

	if (type == FTYPE_UINT) {
		if (data.u < min || data.u > max) {
			inform(VER_CONFIG, "Value of integer outside of range. "
					"(%d < %d < %d)", min, data.i, max);
			return 0;
		}
	} else {
		if (data.i < min || data.i > max) {
			inform(VER_CONFIG, "Value of integer outside of range. "
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
static int ftype_check_string(const featuretype type,
			      const int min,
			      const int max,
			      const t_data data)
{
	int i;

	assert(type == FTYPE_STRING);
	if (data.str == NULL) {
		inform(VER_CONFIG,"NULL-pointer when verifying a string");
		return 0;
	}
	
	for(i == 0; i < MAX(max, KWMD_MAX_STRING); i++) {
		if (data.str[i] == '\0')
			return i;
	}
	inform(VER_CONFIG,"String is larger than expected");
	return 0;
}

/* Dummy-function until keys/actions are implemented */
static int ftype_check_key(const featuretype type,
			   const int min,
			   const int max,
			   const t_data data)
{
	KWMD_DUMMY_RETURN(0);
}

/* Sanity check and verify a feature and it's value based on the
 * verification function.
 *
 * Returns true/false based on success.
 */
int verify_feature(const t_feature *feature)
{
	t_ftype *type = NULL;
	if (feature == NULL) {
		inform(VER_CONFIG, "Feature verification attempted on "
			"a NULL-feature.");
		return 0;
	}
	
	if (feature->type < 0 || feature->type >= FTYPE_NUM) {
		inform(VER_CONFIG, "Feature verification attempted on a "
			"feature with an invalid featuretype.");
		return 0;
	}

	/* This should be impossible, as ftype[] is rather static.
	 * If this ever happens, we are in big trouble and should quit
	 * before it's too late.
	 */
	assert(&ftype[feature->type] != NULL);
	assert(ftype[feature->type].verify);

	type = &ftype[feature->type];

	if (type->verify(feature->type, type->min, type->max, feature->d)) {
		return 0;
	} else {
		inform(VER_CONFIG, "Feature-verification failed for "
			"%s of type %s",
			feature->name,
			type->desc);
		return 1;
	}
}


/* Walks through the feature[] data structure to verify its contents.
 * Returns true upon success.
 */
int verify_all_features(void)
{
	int i;
	int failed = 0;

	/* A bit ugly? Or lubly? Let's have a vote! -K
	 */
	for(i = 0; i < F_NUM; i++)
		failed += !verify_feature(&feature[i]);
	
	return failed;
}
