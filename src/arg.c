/* wmd - argument handling
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
#include <getopt.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "param.h"
#include "com.h"
#include "core.h"

/* Getopt is a bit fugly....
 *
 * note:
 * 	name, 
 * 	{0=no arg, 1=require arg, 2=optional arg with = specified*},
 *	target variable/flag,
 *	return value or value to set at the flag if present
 *
 * Thus: { "foo", 0, blatti, "hai" }, will point the blatti-variable to
 * "hai" if it's found...
 */
static struct option long_options[] = {
	{"help", 2, 0, 'h'},
	{"version", 0, 0, 'V'},
	{"param", 1, 0, 'p'},
	{NULL}
};

/* Just used for describing the above stuff. (Wish this wasn't necessary)
 */
typedef struct _argv_opt_desc_t {
	char *name;
	char *arguments;
} argv_opt_desc_t;

static const argv_opt_desc_t argv_options[] = {
	{
	 "help",
	 "[param|paramlist|verbosity]"},
	{
	 "version",
	 NULL,
	 },
	{
	 "param",
	 "key=value"},
	{NULL}
};

/* Getopt() again. : == requires an argument */
static char *short_options = "h::Vp:";

static void argv_version(FILE * fd)
{
	fprintf(fd, PACKAGE_STRING "\n");
}

static int argv_param(char *arg)
{
	if (!param_parse(arg, P_STATE_ARGV)) {
		inform(V(CORE), "Unable to parse a parameter specified "
		       "on the command line.");
		exit(1);
	}
	return 0;
}

static void argv_usage(FILE * fd)
{
	fprintf(fd, "Usage: ");
	for (int i = 0; argv_options[i].name != NULL; i++) {
		fprintf(fd, " --%s%s%s",
			argv_options[i].name,
			argv_options[i].arguments ? "=" : "",
			argv_options[i].
			arguments ? argv_options[i].arguments : "");
	}
	fprintf(fd, "\n");
}

void argv_generic_help(FILE * fd)
{

	fprintf(fd, PACKAGE_STRING "\n");
	argv_usage(fd);
	fprintf(fd,
		"\nIf run without any parameters, wmd cures cancer.\n");
	fprintf(fd,
		"That being said, read the individual parts of --help"
		" to learn more\n");
}

/* Print various types of help upon request.
 *
 * The style is a bit ugly, perhaps, but this should result in a nice
 * configuration file (among other things).
 *
 * XXX: strncmp is, perhaps, better?
 *
 * XXX: stdout is used for the param-stuff so it can be piped to a file
 * 	without inform()-noise if necessary.
 */
static void argv_help(char *arg)
{
	if (arg == NULL) {
		argv_generic_help(stdout);
	} else if (!strcmp(arg, "param")) {
/* *INDENT-OFF* */
		fprintf(stdout,
"/* Parameters in WMD are essentially options, or settings.\n"
" *\n"
" * All parameters have certain boundaries depending on the type of\n"
" * parameter. You can set a default by using the \"default\" as the value\n"
" * of the parameter. The order parameters are prioritized is:\n"
" * - Interactively set values and arguments on the command line;\n"
" * - Parameters stored in a configuration file\n"
" * - Default values\n"
" *\n"
" * This output uses C-style comments, which is the same as the\n"
" * configuration file of wmd, which means you can pipe it directly into a\n"
" * file and use it as a configuration file. In fact, that might be how\n"
" * you are reading this, so just in case: This text was originally\n"
" * generated with the command: wmd --help param\n"
" *\n"
" * If you prefer a smaller configuration file, you can use:\n"
" * wmd --help paramlist\n"
" * Which will print just the parameter-name and it's current value\n"
" *\n"
" * The following is the documentation on all the parameters available\n"
" * form wmd. They can be be modified with --param key=value, or by using\n"
" * This output as a configuration file.\n"
" */\n");
/* *INDENT-ON* */
		param_show(stdout, -1, UINT_MAX);
	} else if (!strcmp(arg, "paramlist")) {
		param_show(stdout, -1,
			   P_WHAT_BIT(KEYVALUE) |
			   P_WHAT_BIT(STATE_DEFAULTS));

	} else if (!strcmp(arg, "verbosity")) {
		inform_describe_verbosity(stdout, -1);
	} else if (*arg == '\0') {
		argv_generic_help(stdout);
	} else {
		inform(V(CORE), "--help without a valid argument.");
		argv_usage(stderr);
	}
}

/* Handle arguments, getopt()-style. May re-arrange argv. May also blow up.
 * Kaboom.
 *
 * This needs some polish...
 */
int argv_init(int argc, char **argv)
{
	int c;

	while (1) {
		int option_index = 1;

		c = getopt_long(argc, argv, short_options, long_options,
				&option_index);
		if (c == -1)
			break;
		switch (c) {
		case 'h':
			argv_help(optarg);
			exit(0);
			break;
		case 'V':
			argv_version(stdout);
			exit(0);
			break;
		case 'p':
			argv_param(optarg);
			break;
		default:
			argv_usage(stderr);
			exit(1);
			break;
		}
	}

	if (optind < argc) {
		inform(V(CORE), "Non-option argv elements found:");
		while (optind < argc)
			inform(V(CORE), "%s", argv[optind++]);
		argv_usage(stderr);
		exit(1);
	}
	return 1;
}
