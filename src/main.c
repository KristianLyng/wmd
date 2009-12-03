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

/* Essentially a controlled reset of wmd (the structure) and all
 * parameters. Note that this will mess things up if you run it twice, as
 * you'll do funny things like reset the dpy regardless of whether you were
 * connected or not (as we can't trust state, we can't tell if we're
 * connected or not).
 */
static void set_defaults(void)
{
	wmd.state = 0;
	wmd.x.dpy = NULL;
	assert(param_set_default(-1));
	set_state(CONFIGURED);
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

	if (P(sync).b)
		XSynchronize(wmd.x.dpy, 1);
	else
		XSynchronize(wmd.x.dpy, 0);

	set_state(CONNECTED);
}

/* Temporary feature-testing function for stuff that isn't available
 * elsewhere yet.
 */
static void xxx_poc(void)
{
	param_show(stdout, -1,UINT_MAX);
}

/* Let's keep it simple; ten-ish lines max. */
int main(int argc, char **argv)
{
	set_defaults();
	inform_init();
	x_connect();
	xxx_poc();
	return 0;
}
