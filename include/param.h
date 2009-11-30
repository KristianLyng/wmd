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

#ifndef _PARAM_H
#define _PARAM_H

/* FIXME: Does not belong here! Duplicated from core.h for reasons of
 * lazyness.
 */
#define	KWMD_MAX_STRING 1024

/*******
 * Feature-related options.
 * XXX: WIP
 */

typedef enum _featuretype {
	FTYPE_BOOL = 0,
	FTYPE_UINT,
	FTYPE_INT,
	FTYPE_STRING,
	FTYPE_KEY,
	FTYPE_NUM
} featuretype;

/* Container for data types available for features, passed to various
 * helper-functions.
 */
typedef union _t_data {
	void *v;
	char *str;
	char c;
	int i;
	unsigned int u;
	int b;
} t_data;

/* Test for various generic integer feature-functions.  */
#define FTYPE_IS_INT(s) (s == FTYPE_INT || s == FTYPE_UINT || s == FTYPE_BOOL)

typedef int (*verify_function)(
	const featuretype type,
	const int min,
	const int max,
	const t_data data);

/* Different feature-types. The min/max is passed to the verification
 * function to allow re-use of verification functions in case of int, uint
 * and bool.
 */
typedef struct _t_ftype {
	const char *desc;
	int min;
	int max;
	verify_function verify;
} t_ftype;

typedef enum _t_feature_enum {
	F_REPLACE = 0,
	F_SYNC,
	F_VERBOSITY,
	F_NUM
} t_feature_enum;

typedef struct _t_feature {
	const char *name;
	const char *description;
	featuretype type;
	t_data d; // data/value. Frequently used shorthand.
} t_feature;

extern t_feature feature[];

int verify_all_features(void);
int verify_feature(const t_feature *feature);

#endif // _PARAM_H
