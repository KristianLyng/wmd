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

typedef enum _p_type_enum_t {
	PTYPE_BOOL = 0,
	PTYPE_UINT,
	PTYPE_INT,
	PTYPE_MASK,
	PTYPE_STRING,
	PTYPE_KEY,
	PTYPE_NUM
} p_type_enum_t;

/* Container for data types available for params, passed to various
 * helper-functions.
 */
typedef union _p_data_t {
	void *v;
	char *str;
	char c;
	int i;
	unsigned int u;
	int b;
} p_data_t;

typedef enum _p_enum_t {
	P_replace = 0,
	P_sync,
	P_verbosity,
	P_NUM
} p_enum_t;

typedef enum _p_what_enum {
	P_WHAT_COMMENT = 0, 	// /* and */
	P_WHAT_BOILER,		// name, type, min/max
	P_WHAT_VALUE,
	P_WHAT_DEFAULT,
	P_WHAT_DESCRIPTION,
	P_WHAT_KEYVALUE,	// name=value (outside comment)
	P_WHAT_SOURCE,
	P_WHAT_STATE_DEFAULTS,	// Params not changed from the default.
	P_WHAT_NUM
} p_what_enum_t;
#define P_WHAT_BIT(s) (1<<P_WHAT_ ## s)

/* Defines where the parameter-value came from in it's current form, which
 * allows us to set precedence. Higher values override lower ones,
 * regardless of when they are parsed.
 */
typedef enum _p_state_enum_t {
	P_STATE_DEFAULT = 0,
	P_STATE_CONFIG,
	P_STATE_ARGV,
	P_STATE_USER,
	P_STATE_NUM
} p_state_enum_t;

/* Fetch the data of a parameter defined by p.  */
p_data_t param_get_data(p_enum_t p);

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
int param_set(int p, p_data_t d, int origin);

/* Set the default value of a parameter. If p is -1, all parameters are
 * reset to defaults.
 */
int param_set_default(int p, int origin);

/* Show parameters on fd, possibly all of them.
 * If p is -1, all parameters are described.
 */
void param_show(FILE *fd, int p, unsigned int what);

#define P(i) param_get_data(P_ ## i)

#endif // _PARAM_H
