/* Window Management Daemon
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

#include <stdio.h>
#include <stdarg.h>

#include "param.h"
#include "core.h"

core wmd;

#define ADD_VER(name,desc) \
	{ VER_ ## name, 1<<VER_ ## name, #name, desc }
	
t_verbosity verbosity[VER_NUM] = {
	ADD_VER(XIGNORED,
		"Ignored X errors/warnings"),
	ADD_VER(XHANDLED,
		"X errors that have been dealt with"),
	ADD_VER(CONFIG_CHANGES,
		"Changes to the configuration"),
	ADD_VER(CONFIG,
		"Configuration parsing/verification"),
	ADD_VER(STATE,
		"Changes in state, ie: connect/disconnect \n"
		"from X, event-handling/state-handling."),
	ADD_VER(NOTIMPLEMENTED,
		"Attempted access to features that only \n"
		"have placeholders."),
	ADD_VER(FILELINE,
		"Include source-file and line number in \n"
		"messages."),
	ADD_VER(FUNCTION,
		"Include the function that sent the message \n"
		"in the output."),
};

/* Communicate the information and possibly where it came from
 * (funct/file/line) if the verbosity dictates it.
 *
 * Note that we do not distinguish between user and developer. All
 * information should be available upon request.
 *
 * FIXME: Doesn't belong here, and should eventually be expanded a bit.
 */
void inform_real(const unsigned int v,
		 const char *func,
		 const char *file,
		 unsigned int line,
		 const char *fmt, ...)
{
	va_list ap;

	if (wmd.state == STATE_UNINIT || param[P_VERBOSITY].d.u & v) {

		if (param[P_VERBOSITY].d.u & V(FILELINE))
			printf("0x%X:%s:%u: ", v, file, line);

		if (param[P_VERBOSITY].d.u & V(FUNCTION))
			printf("%s(): ", func);

		va_start(ap, fmt);
		vfprintf(stdout, fmt, ap);
		va_end(ap);

		printf("\n");
	}
}

/* Safe setting of state */
void set_state(const unsigned int state)
{
	wmd.state |= state;
	ASSERT_STATE(state);
	inform(V(STATE), "state set");
}

void unset_state(const unsigned int state)
{
	wmd.state &= !state;
	ASSERT_STATE_NOT(state);
	inform(V(STATE), "state unset");
}

/* Essentially a controlled reset of wmd (the structure). 
 * Keep in mind that state should already be explicitly set
 * to avoid any confusion.
 */
static void set_defaults(void)
{
	ASSERT_STATE_NOT(STATE_ANY);
	wmd.x.dpy = NULL;
	assert(verify_all_params());
}

/* Connect to the X display, check if it worked and update state.  */
static void k_connect(void)
{
	wmd.x.dpy = XOpenDisplay(NULL);
	assert(wmd.x.dpy);
	if (param[P_SYNC].d.b)
		XSynchronize(wmd.x.dpy, 1);
	else 
		XSynchronize(wmd.x.dpy, 0);

	set_state(STATE_CONNECTED);
}

/* Let's keep it simple; ten-ish lines max. */
int main(int argc, char **argv)
{
	wmd.state = 0;
	set_defaults();
	k_connect();
	inform(V(STATE), "Everything is done");
	return 0;
}
