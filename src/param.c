
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

#include "param.h"
#include "core.h"


t_ftype ftype[FTYPE_NUM] = {
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
		"Run in synchronized X-mode. Easier debugging but slower, since we have to wait for X.",
		FTYPE_BOOL,
		0,
	},
	// F_VERBOSITY
	{
		"verbosity",
		"The verbosity bitmask.",
		FTYPE_UINT,
		(void *)(~0),
	},
};

/* Verifies that *data contains an int of some sort and that it's within
 * the given range. Note that *data is _not_ assumed to be an actual
 * pointer, as this is a generic prototype.
 *
 * XXX: Turns out that uint and int isn't the same ;)
 */
int ftype_check_int(
	const featuretype type, 
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
 * Returns 0 for invalid string or the length of the string.
 */
int ftype_check_string(
	const featuretype type,
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
	
	for(i == 0; i < max; i++) {
		if (data.str[i] == '\0')
			return i;
	}
	inform(VER_CONFIG,"String is larger than expected");
	return 0;
}

/* Dummy-function until keys are implemented.
 */
int ftype_check_key(
	const featuretype type,
	const int min,
	const int max,
	const t_data data)
{
	KWMD_DUMMY_RETURN(0);
}

/* Sanity check and verify a feature
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
		inform(VER_CONFIG, "Featureverification failed for %s of type %s",
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
