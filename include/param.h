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

#include <stdio.h>

/* FIXME: Does not belong here! Duplicated from core.h for reasons of
 * lazyness.
 */
#define	WMD_MAX_STRING 1024

#include "param-list.h"

/* Container for data types available for params, passed to various
 * helper-functions.
 */
union param_data {
	void *v;
	char *str;
	char c;
	int i;
	unsigned int u;
	int b;
};

typedef enum _p_what_enum {
	P_WHAT_BOILER1 = 0,	// name, type, min/max
	P_WHAT_BOILER2,		// value, default, source
	P_WHAT_DESCRIPTION,
	P_WHAT_KEYVALUE,	// name=value (outside comment)
	P_WHAT_STATE_DEFAULTS,	// Params not changed from the default.
	P_WHAT_NUM
} p_what_enum_t;
#define P_WHAT_BIT(s) (1<<P_WHAT_ ## s)

/*
 * Defines where the parameter-value came from in it's current form, which
 * allows us to set precedence. Higher values override lower ones,
 * regardless of when they are parsed.
 */
enum param_origin {
	P_STATE_DEFAULT = 0,
	P_STATE_CONFIG,
	P_STATE_ARGV,
	P_STATE_USER,
	P_STATE_NUM
};

union param_data param_get(enum param_id p);

/* Set the value of the param p to that of d. Origin is used to determine
 * if this value came from a config, user or default.
 *
 * Returns true if the operation was successful.
 *
 * XXX: Numerous fail checks are in place to catch the most grave errors
 * 	that can be done. Assuming the high-level value of the structure is
 * 	correct, param_set should be able to catch most syntax errors and
 * 	such, but there is no real guarantee, beyond the guarantee that the
 * 	structure is either set to the value of d or returns false.
 *
 * XXX: May trigger reinitialization of relevant portions of wmd.
 */
int param_set(enum param_id p, union param_data d, enum param_origin origin);

/* Set the default value of a parameter. If p is -1, all parameters are
 * reset to defaults.
 */
int param_set_default(enum param_id p, enum param_origin origin);

int param_parse(char *str, enum param_origin origin);
/* Show parameters on fd, possibly all of them.
 * If p is -1, all parameters are described.
 */
void param_show(FILE * fd, enum param_id p, unsigned int what);

#define P(i) param_get(P_ ## i)

#endif				// _PARAM_H
