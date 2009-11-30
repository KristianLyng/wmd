/* Kristian's Window Management Daemon
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

core kwmd;

/* Handle program-messages.
 *
 * Note that we do not distinguish between user and developer. All
 * information should be available upon request.
 * FIXME: Dummy until I bother fixing it.
 */
void inform_real(const unsigned int v,
		 const char *func,
		 const char *file,
		 unsigned int line,
		 const char *fmt, ...)
{
	va_list ap;

	if (kwmd.state == STATE_UNINIT || feature[F_VERBOSITY].d.u & v) {

		if (feature[F_VERBOSITY].d.u & VER_FILELINE)
			printf("0x%X:%s:%u: ", v, file, line);

		if (feature[F_VERBOSITY].d.u & VER_FUNCTION)
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
	kwmd.state |= state;
	ASSERT_STATE(state);
	inform(VER_STATE, "state set");
}

void unset_state(const unsigned int state)
{
	kwmd.state &= !state;
	ASSERT_STATE_NOT(state);
	inform(VER_STATE, "state unset");
}

/* Essentially a controlled reset of kwmd (the structure). 
 * Keep in mind that state should already be explicitly set
 * to avoid any confusion.
 */
static void set_defaults(void)
{
	ASSERT_STATE_NOT(STATE_ANY);
	kwmd.x.dpy = NULL;
	assert(verify_all_features());
}

/* Connect to the X display, check if it worked and update state.  */
static void k_connect(void)
{
	kwmd.x.dpy = XOpenDisplay(NULL);
	assert(kwmd.x.dpy);
	if (feature[F_SYNC].d.b)
		XSynchronize(kwmd.x.dpy, 1);
	else 
		XSynchronize(kwmd.x.dpy, 0);

	set_state(STATE_CONNECTED);
}

/* Let's keep it simple; ten-ish lines max. */
int main(int argc, char **argv)
{
	kwmd.state = 0;
	set_defaults();
	k_connect();
	inform(VER_STATE, "Everything is done");
	return 0;
}
