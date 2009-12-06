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

#include <stdio.h>
/* Test for various generic integer param-functions.  */
#define PTYPE_IS_INT(s) (s == PTYPE_INT || s == PTYPE_UINT || s == PTYPE_BOOL || s == PTYPE_MASK)

typedef struct _param_t {
	const char *name;
	p_type_enum_t type;
	unsigned int state; // Where does the value come from
	p_data_t d; // data/value. Frequently used shorthand.
	const p_data_t default_d;
	int min;
	int max;	
	// FIXME: Better hack than this anyone? Hate limiting docs...
	const char *description[1024];
} param_t;

typedef int (param_verify_func)(
	const p_type_enum_t type,
	const int min,
	const int max,
	const p_data_t data);
typedef int (param_set_func)(int p, p_data_t in);
typedef int (param_print_func)(int p, p_data_t d, FILE *fd);

/* Different param-types. parse() will malloc if necessary and free() will
 * only actually free something if the param-type requires it (ie: it wont
 * free an integer, but a string will be freed).
 */
typedef struct _p_type_t {
	const int position;
	const char *name;
	param_set_func *set;
	param_print_func *print;
	param_verify_func *verify;
} p_type_t;

#endif
