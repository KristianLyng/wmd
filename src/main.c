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

#include <stdarg.h>
#include "param.h"
#include "com.h"
#include "core.h"

core wmd;

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
	assert(param_verify(-1));
}

/* Connect to the X display, check if it worked and update state. 
 *
 * XXX: Should verify XSynchronize
 */
static void x_connect(void)
{
	assert(wmd.x.dpy == NULL);
	wmd.x.dpy = XOpenDisplay(NULL);
	assert(wmd.x.dpy);

	if (P(SYNC).b)
		XSynchronize(wmd.x.dpy, 1);
	else
		XSynchronize(wmd.x.dpy, 0);

	set_state(STATE_CONNECTED);
}

/* Temporary feature-testing function for stuff that isn't available
 * elsewhere yet.
 */
static void xxx_poc(void)
{
	inform_describe_verbosity(stdout,-1);
}

/* Let's keep it simple; ten-ish lines max. */
int main(int argc, char **argv)
{
	wmd.state = 0;
	inform_init();
	set_defaults();
	x_connect();
	xxx_poc();
	inform(V(STATE), "Everything is done");
	return 0;
}
