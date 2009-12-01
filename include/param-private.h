/* wmd - private parameter handling
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

/* Only to be used in param.c
 */

#ifndef _PARAM_PRIVATE_H
#define _PARAM_PRIVATE_H

/* Test for various generic integer param-functions.  */
#define PTYPE_IS_INT(s) (s == PTYPE_INT || s == PTYPE_UINT || s == PTYPE_BOOL)

typedef int (*verify_function)(
	const paramtype type,
	const int min,
	const int max,
	const t_data data);

/* Different param-types. The min/max is passed to the verification
 * function to allow re-use of verification functions in case of int, uint
 * and bool.
 */
typedef struct _t_ptype {
	const char *desc;
	int min;
	int max;
	verify_function verify;
} t_ptype;

#endif
