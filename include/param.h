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

typedef enum _paramtype {
	PTYPE_BOOL = 0,
	PTYPE_UINT,
	PTYPE_INT,
	PTYPE_MASK,
	PTYPE_STRING,
	PTYPE_KEY,
	PTYPE_NUM
} paramtype;

/* Container for data types available for params, passed to various
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

typedef enum _t_param_enum {
	P_REPLACE = 0,
	P_SYNC,
	P_VERBOSITY,
	P_NUM
} t_param_enum;

typedef struct _t_param {
	const char *name;
	const char *description;
	paramtype type;
	t_data d; // data/value. Frequently used shorthand.
	const t_data default_d;
	int min;
	int max;	
} t_param;

/* Verify a parameter with the p-enum. If p is -1, all parameters are
 * verified.
 */
int param_verify(int p);

/* Fetch the data of a parameter defined by p.  */
t_data param_get_data(t_param_enum p);

/* Set the default value of a parameter. If p is -1, all parameters are
 * reset to defaults.
 */
int param_set_default(int p);

/* Describe the parameter p on fd, including default and current value.
 * If p is -1, all parameters are described.
 */
void param_describe(FILE *fd, int p);

/* Show the name and value of a parameter on fd. If p is -1, all parameters
 * are listed.
 */
void param_list(FILE *fd, int p);

#define P(i) param_get_data(P_ ## i)

#endif // _PARAM_H
