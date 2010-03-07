/* wmd - Overall X handling
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

#include "param.h"
#include "inform.h"
#include "core.h"

extern core wmd;

/*
 * Checks if an old WM is present and tries to replace it if it is.
 */
static int x_replace(void)
{
	WMD_DUMMY_RETURN(1);
}

/*
 * Reset/null the wmd.x structure. Only used for setting
 * reasonable defaults.
 */
static void x_reset_core(void)
{
	wmd.x.connection = NULL;
	wmd.x.default_screen = 0;
}

/*
 * Initializes the X connection
 *
 * If an existing WM is running - kindly ask it to go away if replace is
 * set. Returns true on success.
 *
 * XXX: Should verify XSynchronize
 */
int x_init(void)
{
	int ret = 0;
	x_reset_core();
	if (P(replace).b) {
		ret = x_replace();
		if (!ret) {
			inform(V(XIGNORED), "Unable to replace the old WM. "
			       "You will have to stop it manually.");
			return ret;
		}
	}

	assert(wmd.x.connection == NULL);
	wmd.x.connection = xcb_connect(NULL, &wmd.x.default_screen);
	assert(wmd.x.connection);
/*
	if (P(sync).b)
		XSynchronize(wmd.x.dpy, 1);
	else
		XSynchronize(wmd.x.dpy, 0);
*/
	set_state(CONNECTED);
	return ret;
}

/*
 * X mainloop
 */
int x_start(void)
{
	WMD_DUMMY_RETURN(1);
}
