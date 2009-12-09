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
#include "inform.h"
#include "core.h"
#include "WIP.h"

core wmd;

/*
 * Only run this once, or mountain trolls may carry you off to the
 * wilderness.
 */
static void set_defaults(void)
{
	wmd.state = 0;
	wmd.x.dpy = NULL;
	assert(param_set_default(P_ALL, P_STATE_DEFAULT));
}

/*
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

/*
 * See WIP for rationale.
 */
static void wip(void)
{
	int i;
	for (i = 0; WIP_list[i] != NULL; i++)
		inform(V(NOTIMPLEMENTED), "%s", WIP_list[i]);
}

/*
 * Let's keep it simple.
 */
int main(int argc, char **argv)
{
	set_defaults();
	inform_init(stderr);
	argv_init(argc, argv);
	assert(config_init());
	wip();
	x_connect();
	return 0;
}
