
/* wmd core data structures
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

#ifndef _CORE_H
#define _CORE_H

#include <X11/Xlib.h>
#include <limits.h>
#include <assert.h>

/* Needed to find out where the information came from. */
#define inform(v, ...) inform_real(v, __func__, __FILE__, __LINE__, __VA_ARGS__)
void inform_real(const unsigned int v,
		 const char *func,
		 const char *file,
		 unsigned int line,
		 const char *fmt, ...);

/* Maximum length of input-strings. */
#define	WMD_MAX_STRING 1024

/* Simple way to insert a dummy-function */
#define WMD_DUMMY_RETURN(r) 						\
	do { 								\
		inform(VER_NOTIMPLEMENTED, "A function that's not yet "	\
			"implemented was accessed at "); 		\
		return (r); 						\
	} while (0);

/* Used for wmd.state to indicate what state we are currently in.
 * Verify state at any given generic function.
 */
#define STATE_UNINIT 		0x0 // Semi-dummy.
#define STATE_CONFIGURED 	1<<0
#define STATE_CONNECTED 	1<<1 // To X
#define STATE_INITILIAZED	1<<2 // Basics are up
#define STATE_EVENT		1<<3 // Event processing
#define	STATE_TIMEOUT		1<<4 // No events - timed out
#define STATE_RECONFIGURE	1<<5 // Reconfiguring
#define STATE_MULTIHEAD		1<<6 // Running in multi-head mode. Not yet supported.
#define STATE_ANY		UINT_MAX

#define ASSERT_STATE(s) assert((wmd.state & s) == s)
#define ASSERT_STATE_NOT(s) assert((wmd.state & s) == 0)

typedef enum _t_verbosity_enum {
	VER_XIGNORED = 0,
	VER_XHANDLED,
	VER_CONFIG_CHANGES,
	VER_CONFIG,
	VER_STATE,
	VER_NOTIMPLEMENTED,
	VER_FILELINE,
	VER_FUNCTION,
	VER_NUM
} t_verbosity_enum;
/* Defines the various levels of verbosity we may or may not want.
 * The VER_X* masks are not necessarily limited to X. Only IGNORED or
 * HANDLED is provided because we assert on anything else for now. HANDLED
 * could be used for backing out too.
 *
 * FIXME: Has to be possible to backtrace
 */
typedef struct _t_verbosity {
	unsigned int position; // From t_verbosity_enum for backtracing
	unsigned int bit;
	const char *name;
	const char *description;
} t_verbosity;

extern t_verbosity verbosity[];

#define V(s) verbosity[VER_ ## s].bit
/******************************
 * Core state structures
 */
	
/* X-only state */
typedef struct _x {
	Display *dpy;
} x;

/* The core structure. Max length: 10ish entries.  */
typedef struct _core {
	unsigned int state;
	x x;
} core;

#endif // _CORE_H
