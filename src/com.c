/* wmd - communication stuff
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

/* See com.h for usage.
 *
 * The communication interface should be able to send information to a
 * generic file descriptor. At the moment, we're not quite there, but for
 * the purpose of using inform(), com.c is feature-complete enough.
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "param.h"
#include "com.h"
#include "core.h"

/* Defines the various levels of verbosity we may or may not want. Position
 * and bit is both kept around even though they could be deduced from the
 * position and enum. This is to ensure that we can verify the integrity of
 * the structure easily.
 */
typedef struct _t_verbosity {
	const unsigned int position;
	const unsigned int bit;
	const char *name;
	const char *desc;
} t_verbosity;

/* Short for "Add Verbosity" ... Or something like that. */
#define AV(name,desc) \
	{ VER_ ## name, 1<<VER_ ## name, #name, desc }

/* Different verbosity levels, as defined in t_verbosity_enum. VER_ isn't
 * included as it's only needed internally. (V() and AV() both add it as
 * needed) 
 */	
t_verbosity verbosity[VER_NUM] = {
	AV(XIGNORED,
		"Ignored X errors/warnings"),
	AV(XHANDLED,
		"X errors that have been dealt with"),
	AV(CONFIG_CHANGES,
		"Changes to the configuration"),
	AV(CONFIG,
		"Configuration parsing/verification"),
	AV(STATE,
		"Changes in state, ie: connect/disconnect \n"
		"from X, event-handling/state-handling."),
	AV(NOTIMPLEMENTED,
		"Attempted access to features that only \n"
		"have placeholders."),
	AV(FILELINE,
		"Include source-file and line number in \n"
		"messages."),
	AV(FUNCTION,
		"Include the function that sent the message \n"
		"in the output."),
	AV(CORE,
		"Information related to the core functionality \n"
		"of wmd. This should almost always be set."),
};
#undef AV

/* Communicate the information and possibly where it came from
 * (function/file/line) if the verbosity dictates it.
 *
 * Note that we do not distinguish between user and developer. All
 * information should be available upon request. There is no debug().
 *
 * FIXME: Doesn't belong here, and should eventually be expanded a bit.
 */
void inform_real(const unsigned int v,
		 const char *func,
		 const char *file,
		 const unsigned int line,
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

/* Sanity-check of a verbosity-level.
 *
 * XXX: Since verbosity-levels aren't really prone to breaking, this is a
 * 	developer-failsafe more than anything. Also catches mismatches
 * 	between t_verbosity_enum and verbosity[].
 */
static void inform_verify_verbosity(int p)
{
	assert(verbosity[p].position == p);
	assert(verbosity[p].bit == 1<<p);
	assert(verbosity[p].desc);
	assert(verbosity[p].name);
}

/* Set up whatever is needed to inform the user and verify that verbosity
 * is complete and sane while we're at it.
 *
 * XXX: Probably not very close to the final result, but a reasonable
 * 	place-holder nevertheless.
 */
void inform_init(void) {
	int i;
	for (i = 0; i < VER_NUM; i++) {
		inform_verify_verbosity(i);
		if (strlen(verbosity[i].desc) < 10) {
			inform(V(CORE),"Description of verbosity level %s "
				"(%d, 0x%X) is alarmingly short", verbosity[i].name,
				i, verbosity[i].bit);
		}
	}
}

/* Describe the verbosity level defined by p on fd. If p is -1, describe
 * all of them.
 */
void inform_describe_verbosity(FILE *fd, const int p)
{
	int i;

	if (p == -1) {
		for (i = 0; i < VER_NUM; i++)
			inform_describe_verbosity(fd, i);
		return;
	}
	inform_verify_verbosity(p);

	fprintf(fd, "0x%X\t1<<%d\t\t%s\n",
		verbosity[p].bit,
		verbosity[p].position,
		verbosity[p].name);
	fprintf(fd, "%s\n\n", verbosity[p].desc);
	return;
}
