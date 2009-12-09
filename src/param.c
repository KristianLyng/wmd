/* wmd - parameter handling
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

/* Basic parameter support for wmd.
 *
 * WMD allows setting and reading parameters from various sources (in
 * theory) and therefor needs a reasonably generic and flexible interface
 * to make sure the values are sane.
 *
 * Policies affecting param.c:
 *  - Verify everything
 *  - Make smart interfaces instead of many
 *  - Do not expose internal data structures and make sure it's not
 *    something we miss elsewhere.
 *  - strip leading and trailing white space
 *  - Ignore case where relevant.
 *  - Handle allocation internally - let the caller do whatever with the
 *    input data when we're done with it. In fact, do not modify input data
 *    at all if it can be avoided.
 *  - Configuration-file comments do not reach param_*()
 *
 */

#include <sys/param.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "param.h"
#include "param-private.h"
#include "com.h"
#include "core.h"

/* Param Definition (...).
 * Ensures it's mapped to the enum regardless of sorting.
 *
 * Using __VA_ARGS__ instead of just a large char * is to get a list of
 * lines which can then be optionally commented out in output. A bit hacky,
 * but the important bit is the readability of the PD() underneath.
 */
#define PD(name,type,def,field,min,max, ...) 	\
	[P_ ## name] =				\
	{ #name, PTYPE_ ## type, 0,		\
	{ .v = NULL },				\
	{ .field = def }, min, max , 		\
	{ __VA_ARGS__ } },

/*  *INDENT-OFF*  */

/*
 * (The above tells `indent' to not parse this section)
 *
 * Actual settings matching the param enum in param.h
 *
 * Arguments:
 * name (P_name must exist in param.h)
 * type (See param-private.h, or the list underneath)
 * default value
 * field-name in the data-struct (FIXME!)
 * minimum value (int)
 * maximum value (int)
 * description (separate lines with commas for formatting, PD() takes care
 * 	of the nasty bits).
 *
 * XXX: Note that the min/max are not necessarily used, it depends on the
 * 	type of parameter.
 *
 * FIXME: The min/max should probably be printed by the type, to avoid
 * 	showing -1 as the max for MASK, for instance.
 *
 * FIXME: the field-name shouldn't be necessary.
 */
static struct param param[P_NUM] = {
PD(replace,	BOOL,	0,	b,
	0,	1,
	"If set to true, wmd will attempt to replace the running window",
	"manager, otherwise, it will exit if a window manager already has",
	"control over the X session.",
	"",
	"For this to work, the running window manager needs to understand",
	"the underlying protocol.",
	"FIXME: Clarification.", NULL
	)

PD(sync,	BOOL,	0,	b,
	0,	1, 
	"Run in synchronized X-mode.","",
	"Easier debugging but slower, since we have to wait for X.",
	NULL)

PD(verbosity, 	MASK,	(UINT_MAX ^ ((1<<VER_FILELINE)|(1<<VER_STATE))), u,
	0,	UINT_MAX,
	"Bitmask deciding what information to print and how to format it.",
	"",
	"See --help verbosity list for a list of bits and what they do.",
	"",
	"You probably want to inverse the mask to see what is disabled",
	"instead of what is enabled (which is everything except FILELINE",
	"by default.)", NULL)

PD(name,	STRING,	"test", str,
	0,	0,
	"String used mainly to test the STRING-data type until we need it.",
	NULL)
};

#undef PD

/*  *INDENT-ON*  */

/*
 * Everything done by param.c on behalf of other modules is verified, so
 * explicitly telling param.c to verify something is redundant, thus not
 * exposed.
 */

static param_set_func ptype_set_simple;
static param_set_func ptype_set_string;
static param_set_func ptype_set_key;

static param_print_func ptype_print_simple;
static param_print_func ptype_print_string;
static param_print_func ptype_print_key;

static param_verify_func ptype_verify_simple;
static param_verify_func ptype_verify_string;
static param_verify_func ptype_verify_key;

static param_parse_func ptype_parse_simple;
static param_parse_func ptype_parse_string;
static param_parse_func ptype_parse_key;

/*
 * Different param-types we support.
 *
 * The "family" concept is introduced to simplify what parameter types can
 * be handled by the same set of function(s).
 *
 * set should free old values and allocate resources as needed. The
 *
 * print should print just the value specified, the data is an argument so
 * both default_d and d can be printed.
 *
 * verify should sanity check that the data is within the expected
 * confines. It can safely ignore min/max where relevant, but should verify
 * that the min/max isn't set if it is ignored (since that will give the
 * false impression that verify takes it into account).
 *
 * parse should be able to back out. Avoid assert() based on user input -
 * encourage it based on stupid code (ie: NULL-data).
 */
#define PA(name, family) 					\
	[PTYPE_ ## name] = {					\
		PTYPE_ ## name, #name,				\
		ptype_set_ ## family,				\
		ptype_print_ ## family, 			\
		ptype_verify_ ## family,			\
		ptype_parse_ ## family }

static struct param_type ptype[PTYPE_NUM] = {
	PA(BOOL, simple),
	PA(UINT, simple),
	PA(INT, simple),
	PA(MASK, simple),
	PA(STRING, string),
	PA(KEY, key)
};

#undef PA

/***************************************************************
 * Common sanity-check and utility-functions.                  *
 ***************************************************************/

/*
 * Checks the range of p. Should never fail.
 */
static inline void param_is_in_range(enum param_id p)
{
	assert(p >= 0);
	assert(p < P_NUM);
	assert(param[p].type >= 0);
	assert(param[p].type < PTYPE_NUM);
}

/*
 * Verifies that the d-data is valid for param[p] based on type.
 */
static int param_verify_data(enum param_id p, const union param_data d)
{
	enum param_type_id t;

	param_is_in_range(p);
	t = param[p].type;	
	assert(&ptype[t] != NULL);
	assert(ptype[t].verify);

	if (ptype[t].verify(t, param[p].min, param[p].max, d)) {
		return 1;
	} else {
		inform(V(CONFIG), "Parameter-verification failed for "
		       "%s of type %s", param[p].name, ptype[t].name);
		return 0;
	}
}

static int param_verify(enum param_id p)
{
	int len = 0, i = 0;
	param_is_in_range(p);
	assert(param[p].name);
	assert(param[p].description);
	assert(*param[p].name != '\0');
	for (i = 0; param[p].description[i] != NULL; i++) {
		len += strlen(param[p].description[i]);
	}
	if (len < 10) {
		inform(V(CONFIG),
		       "Alarmingly short description of parameter "
		       "\"%s\" found (%d characters long) during "
		       "verification.", param[p].name, len);
	}
	return param_verify_data(p, param[p].d);
}

/*
 * Searches for the parameter with the name 'key' and returns the
 * index-number (p). Returns negative if it wasn't found.
 */
static int param_search_key(char *key)
{
	int i;
	assert(key);

	for (i = 0; i < P_NUM; i++) {
		if (!strcasecmp(param[i].name, key)) {
			return i;
		}
	}
	return -1;
}

/***************************************************************
 * Parameter type-specific verification, setting and printing. *
 ***************************************************************/

/*
 * INT, UINT, BOOL, MASK
 */
static int ptype_verify_simple(const enum param_type_id type,
			       const int min,
			       const int max, const union param_data data)
{
	assert(PTYPE_IS_INT(type));

	if (type == PTYPE_UINT || type == PTYPE_MASK) {
		if (data.u < min || data.u > max) {
			inform(V(CONFIG),
			       "Value of integer outside of range. "
			       "(%d < %d < %d)", min, data.i, max);
			return 0;
		}
	} else {
		if (data.i < min || data.i > max) {
			inform(V(CONFIG),
			       "Value of integer outside of range. "
			       "(%d < %d < %d)", min, data.i, max);
			return 0;
		}
	}
	return 1;
}

/* Verify that *data points to a string that is no longer than max. Note
 * that there is no way (?) to verify that *data is valid before accessing
 * it, so if it points to a bogus area, we will still fall apart.
 *
 * min is ignored.
 *
 * Returns 0 for invalid string or the length of the string + 1
 */
static int ptype_verify_string(const enum param_type_id type,
			       const int min,
			       const int max, const union param_data data)
{
	int i;

	assert(type == PTYPE_STRING);
	if (data.str == NULL) {
		inform(V(CONFIG), "NULL-pointer when verifying a string");
		return 0;
	}

	for (i = 0; i < MAX(max, WMD_MAX_STRING); i++) {
		if (data.str[i] == '\0')
			return 1 + i;
	}
	inform(V(CONFIG), "String is larger than expected");
	return 0;
}

static int ptype_verify_key(const enum param_type_id type,
			    const int min,
			    const int max, const union param_data data)
{
	WMD_DUMMY_RETURN(0);
}

/* Set the value of the param p to that of data, assuming it can do by
 * assignment. Only valid for INT, UINT, BOOL etc, not pointer-data.
 *
 * Returns true if value was changed or already set.
 */
static int ptype_set_simple(enum param_id p, union param_data data)
{
	int ret;
	param_is_in_range(p);
	ret = param_verify_data(p, data);
	if (!ret) {
		inform(V(CONFIG),
		       "Failed to set a simple (int,uint,bool) parameter due to"
		       " failed verification.");
		return 0;
	}

	/*
	 * XXX: During initialization, we want to make sure that we set
	 *      everything up.
	 */
	if (!memcmp(&param[p].d, &data, sizeof(union param_data))
	    && STATE_IS(CONFIGURED)) {
		inform(V(CONFIG_CHANGES),
		       "Not changing parameter \"%s\" - value already set.",
		       param[p].name);
		return 1;
	}

	param[p].d = data;
	assert(!memcmp(&param[p].d, &data, sizeof(union param_data)));
	return 1;
}

/* Assign a (new) string to the parameter p.
 *
 * Note that this is where allocation and free() takes place; if the
 * calling function wishes to, it can and should free the data in data.
 *
 * In other words: Parameters are entirely self contained.
 */
static int ptype_set_string(enum param_id p, union param_data data)
{
	char *old = NULL;
	char *new;
	param_is_in_range(p);
	assert(param[p].type == PTYPE_STRING);
	assert(data.str);

	old = param[p].d.str;

	new = malloc(strlen(data.str) * sizeof(char));
	assert(new);
	strcpy(new, data.str);
	param[p].d.str = new;

	if (!param_verify(p)) {
		inform(V(CONFIG),
		       "Failed to verify param %s after setting "
		       "new string. Rolling back if possible. Side note: "
		       "this is strange, you may want to alert someone...",
		       param[p].name);
		param[p].d.str = old;
		free(new);
		return 0;
	}
	if (old)
		free(old);

	return 1;
}

static int ptype_set_key(enum param_id p, union param_data data)
{
	WMD_DUMMY_RETURN(0);
}

/* Parses to a simple data type. Uses goto out for a safe exit, ensuring
 * the working copy is freed.
 *
 * XXX: This is a bit messy, and might benefit from being split up, since
 * 	it handles both bool and int/uint/mask.
 */
static int ptype_parse_simple(enum param_id p, char *orig, enum param_origin origin)
{
	union param_data d;
	int i, ret = 0;
	char *str, *full;
	assert(orig);
	param_is_in_range(p);

	str = malloc(strlen(orig) * sizeof(char));
	assert(str);
	// Wheeee!
	strcpy(str, orig);
	full = str;

	i = strlen(str);

	if (i == 0) {
		inform(V(CONFIG), "String size for simple parameter is 0,"
		       " did you mean 'default'? Param: %s.",
		       param[p].name);
		ret = 1;
		goto out;
	}

	str[i] = '\0';

	if (param[p].type == PTYPE_BOOL) {
		d.b = 2;
		if (!strcasecmp(str, "true"))
			d.b = 1;
		else if (!strcasecmp(str, "false"))
			d.b = 0;
		else if (!strcasecmp(str, "0"))
			d.b = 0;
		else if (!strcasecmp(str, "1"))
			d.b = 1;
		if (d.b == 2) {
			inform(V(CONFIG), "Invalid boolean value for "
			       "'%s' (value: %s)", param[p].name, str);
			ret = 2;
			goto out;
		}
		/*
		 * FIXME: What if it fails? 
		 */
		ret = param_set(p, d, origin);
		goto out;
	} else if (param[p].type == PTYPE_MASK ||
		   param[p].type == PTYPE_UINT ||
		   param[p].type == PTYPE_INT) {
		union param_data d;
		char *end;
		long int l;
		l = strtol(str, &end, 0);
		if (l == LONG_MIN || l == LONG_MAX) {
			inform(V(CONFIG), "Param %s way out of range.",
			       param[p].name);
			ret = 3;
			goto out;
		}
		if (end && *end != '\0') {
			inform(V(CONFIG), "Param %s had extra garbage: %s",
			       param[p].name, end);
			ret = 4;
			goto out;
		}
		if (l > UINT_MAX) {
			inform(V(CONFIG), "Param %s out of range.",
			       param[p].name);
			ret = 5;
			goto out;
		}
		d.u = l;
		ret = param_set(p, d, origin);
		goto out;
	}
out:
	free(full);
	return ret;
}

/* Not much to parse, really. 
 *
 *
 * XXX: Probably want to make strings bound by "" eventually, to
 * 	circumvent whitespace stripping. Probably. Maybe. I dunno, leave me
 * 	alone.
 */
static int ptype_parse_string(enum param_id p, char *str, enum param_origin origin)
{
	union param_data d;

	assert(str);
	param_is_in_range(p);

	d.str = str;
	return param_set(p, d, origin);
}

static int ptype_parse_key(enum param_id  p, char *str, enum param_origin origin)
{
	WMD_DUMMY_RETURN(0);
}

static int ptype_print_key(enum param_id p, union param_data d, FILE * fd)
{
	WMD_DUMMY_RETURN(0);
}

static int ptype_print_simple(enum param_id p, union param_data d, FILE * fd)
{
	param_is_in_range(p);
	assert(PTYPE_IS_INT(param[p].type));
	switch (param[p].type) {
	case PTYPE_INT:
		fprintf(fd, "%d", d.i);
		break;
	case PTYPE_UINT:
		fprintf(fd, "%u", d.u);
		break;
	case PTYPE_MASK:
		fprintf(fd, "0x%.8X", d.u);
		break;
	case PTYPE_BOOL:
		fprintf(fd, "%s", d.b ? "true" : "false");
		break;
	default:
		assert("Reached default-case when it should be "
		       "impossible. Is PTYPE_IS_INT() in sync?"
		       "(And yeah, this is an assert)");
		inform(V(CORE), "The impossible happened! Abandon"
		       " ship! (printing a simple parameter that"
		       " is apparently not an integer, unsigned "
		       "integer, bitmask or boolean. Now what...");
		return 0;
		break;
	}
	return 1;
}

static int ptype_print_string(enum param_id p, union param_data d, FILE * fd)
{
	param_is_in_range(p);
	assert(param[p].type == PTYPE_STRING);
	fprintf(fd, "%s", d.str ? d.str : "(NULL)");
	return 1;
}

/***************************************************************
 * "API"/External access. Check. And. Verify. Everything.
 ***************************************************************/

/*
 * Returns true if param[p].d was set to d
 */
int param_set(enum param_id p, union param_data d, enum param_origin origin)
{
	int ret;
	param_is_in_range(p);
	if (origin < param[p].origin) {
		inform(V(CONFIG_CHANGES), "Not setting parameter %s,"
		       " current value has higher priority",
		       param[p].name);
		return 0;
	}
	if (STATE_IS(CONFIGURED))
		inform(V(CONFIG_CHANGES), "Setting value of parameter "
		       "\"%s\"", param[p].name);
	ret = ptype[param[p].type].set(p, d);
	if (ret)
		assert(param_verify(p));
	else
		inform(V(CONFIG_CHANGES),
		       "Failed to set value of parameter "
		       "\"%s\". *set() returned %d", ret);
	param[p].origin = origin;
	return ret;
}

/* Parse a string to set a parameter.
 *
 * Typically passed directly from the config-engine, an argument or over
 * some other interactive means, thus ample error handling is needed.
 * Ignore extra leading and trailing white space, and case.
 *
 * Returns true if it succeeded.
 *
 * FIXME: Needs a cleanup, reads like a script, not program code.
 */
int param_parse(char *str, enum param_origin origin)
{
	char *sep = NULL;
	// FIXME: Hardcoded max-length.
	char key[1024];
	int p = -1;
	int length = 0;
	int value_length = 0;
	char *tmp;

	if (str == NULL) {
		inform(V(CONFIG), "Not parsing NULL-string as a parameter");
		return 0;
	}

	while (*str != '\0' && isspace(*str))
		str++;

	sep = index(str, '=');
	if (sep == NULL) {
		inform(V(CONFIG), "Missing '=' in parameter "
		       "key-value pair: %s", str);
		return 0;
	}
	length = sep - str;
	assert(length >= 0);
	if (length >= 1024) {
		inform(V(CONFIG), "Parameter-name absurdly long?");
		return 0;
	}

	/*
	 * Skip the = 
	 */
	sep++;
	tmp = strncpy(key, str, length);
	assert(tmp == key);

	while (length > 0 && isspace(key[length - 1]))
		length--;

	key[length] = '\0';

	p = param_search_key(key);

	if (p < 0) {
		inform(V(CONFIG), "Unknown parameter: %s", key);
		return 0;
	}

	while (*sep != '\0' && isspace(*sep))
		sep++;

	value_length = strlen(sep);
	while (value_length > 0 && isspace(sep[value_length - 1]))
		value_length--;
	sep[value_length] = '\0';
	if (!strcasecmp(sep, "default")) {
		assert(param_set_default(p, origin));
		return 1;
	}

	if (ptype[param[p].type].parse(p, sep, origin))
		return param_verify(p);

	return 0;
}

/* Set the default value for param p, or for all parameters if p is -1. 
 * Origin is where the request came from. Typically this will be DEFAULT
 * during startup, CONFIG while parsing the config file and USER later on,
 * even if the user issues a reset to the default.
 *
 * Returns true if successful.
 *
 * XXX: Any origin is valid here, since 'default' should be available
 * 	during configuration and something that could be issued
 * 	interactively.
 */
int param_set_default(enum param_id p, enum param_origin origin)
{
	int ret = 0;
	if (p == -1) {
		if (STATE_IS(CONFIGURED))
			inform(V(CONFIG),
			       "Resetting values for all parameters to default");
		for (p = 0; p < P_NUM; p++)
			ret += !param_set(p, param[p].default_d, origin);
		return !ret;
	}

	if (STATE_IS(CONFIGURED))
		inform(V(CONFIG_CHANGES), "Resetting value of parameter "
		       "\"%s\" to default", param[p].name);
	return param_set(p, param[p].default_d, origin);
}

/*
 * Print information about a single parameter, or all parameter (p == -1).
 * what defines what is printed, so it could be suitable for using in a
 * configuration file, passing to a pipe, --help, man-file or whatever
 * else.
 */
#define WB(s) ((P_WHAT_BIT(s) & what) == P_WHAT_BIT(s))
void param_show(FILE * fd, enum param_id p, unsigned int what)
{
	if (p == -1) {
		for (p = 0; p < P_NUM; p++) {
			param_show(fd, p, what);
			fprintf(fd, "\n");
		}
		return;
	}
	param_is_in_range(p);
	if (!WB(STATE_DEFAULTS)) {
		if (param[p].origin == P_STATE_DEFAULT)
			return;
	}

	if (WB(BOILER1)) {
		fprintf(fd, "# key: %s, type:%s\n",
			param[p].name,
			ptype[param[p].type].name);
	}

	if (WB(BOILER2)) {
		fprintf(fd, "# value: ");
		ptype[param[p].type].print(p, param[p].d, fd);
		fprintf(fd, " default: ");
		ptype[param[p].type].print(p, param[p].default_d, fd);
		fprintf(fd, " source: ");
		switch (param[p].origin) {
		case P_STATE_DEFAULT:
			fprintf(fd, "default");
			break;
		case P_STATE_CONFIG:
			fprintf(fd, "configuration file");
			break;
		case P_STATE_ARGV:
			fprintf(fd, "argument");
			break;
		case P_STATE_USER:
			fprintf(fd, "user-set at run-time");
			break;
		default:
			assert("Fell through to the default-case "
			       "while describing the parameter state.");
		}
		fprintf(fd,"\n");
	}

	if (WB(DESCRIPTION)) {
		int i;

		fprintf(fd, "#\n");
		for (i = 0; param[p].description[i] != NULL; i++) {
			fprintf(fd, "#%s%s\n",
				*param[p].description[i] ? " " : "",
				param[p].description[i]);
		}
	}

	if (WB(KEYVALUE)) {
		fprintf(fd, "%s=", param[p].name);
		ptype[param[p].type].print(p, param[p].d, fd);
		fprintf(fd, "\n");
	}
}

#undef WB

union param_data param_get(enum param_id p)
{
	param_is_in_range(p);
	return param[p].d;
}
