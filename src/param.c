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
 * Verify, verify, verify and then make sure that you can't access these
 * values without verifying them some more. But make it easy.
 *
 * We want as few entry-points to the parameters as possible to assure a
 * consistent behavior and that we only have to solve a problem in one
 * place.
 *
 * XXX: We may want to split the actual values into it's own file to avoid
 * 	mixing the values of the parameters and the framework to use it.
 *
 * XXX: A param-test framework should be included to dry-run param-changes.
 * 	This is particularly important for bindings.
 */

#include <sys/param.h>
#include <string.h>
#include <stdarg.h>
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
	{ .field = def },			\
	{ .field = def }, min, max , 		\
	{ __VA_ARGS__ } },


/* Actual settings matching the param enum in param.h
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
static param_t param[P_NUM] = {
PD(replace,	BOOL,	0,	b,
	0,	1,
	"If set to true, wmd will attempt to replace the running window\n",
	"manager, otherwise, it will exit if a window manager already has\n",
	"control over the X session.\n",
	"\n",
	"For this to work, the running window manager needs to understand\n",
	"the underlying protocol.\n",
	"FIXME: Clarification.\n", NULL
	)

PD(sync,	BOOL,	0,	b,
	0,	1, 
	"Run in synchronized X-mode.\n","\n",
	"Easier debugging but slower, since we have to wait for X.\n",
	NULL)

PD(verbosity, 	MASK,	(UINT_MAX ^ ((1<<VER_FILELINE)|(1<<VER_STATE))), u,
	0,	UINT_MAX,
	"Bitmask deciding what information to print and how to format it.\n",
	"\n",
	"See --help verbosity list for a list of bits and what they do.\n",
	"\n",
	"You probably want to inverse the mask to see what is disabled\n",
	"instead of what is enabled (which is everything except FILELINE\n",
	"by default.)\n", NULL)
};

#undef PD

/* These are static because you do not want to call the verification
 * params from outside param, instead just call the param-verification
 * functions which in turn will figure out the correct function to call.
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

/* Different param-types we support.
 *
 * The "family" concept is introduced to simplify what parameter types can
 * be handled by the same set of function(s).
 *
 * set should free old values and allocate resources as needed. The
 * value_changed of the parameter should only be run if a real change takes
 * place.
 * FIXME: no value_changed (yet).
 *
 * print should print just the value specified, the data is an argument so
 * both default_d and d can be printed.
 *
 * verify should sanity check that the data is within the expected
 * confines. It can safely ignore min/max where relevant, but should verify
 * that the min/max isn't set if it is ignored (since that will give the
 * false impression that verify takes it into account).
 */
#define PA(name, family) 					\
	[PTYPE_ ## name] = {					\
		PTYPE_ ## name, #name,				\
		ptype_set_ ## family,				\
		ptype_print_ ## family, 			\
		ptype_verify_ ## family },

static p_type_t ptype[PTYPE_NUM] = {
	PA(BOOL, simple)
	PA(UINT, simple)
	PA(INT, simple)
	PA(MASK, simple)
	PA(STRING, string)
	PA(KEY,	key)
};

#undef PA

/***************************************************************
 * Common sanity-check and utility-functions.
 ***************************************************************/

/* Checks the range of p. Since this should never fail, asserts will do.
 *
 * XXX: Used extensively. It's not like you should have to worry about this
 * 	anyway. May be rewritten as a macro to ensure that it's in-line
 * 	even with debugging on.
 */
static inline void param_is_in_range(int p)
{
	assert(p >= 0);
	assert(p < P_NUM);
	assert(param[p].type >= 0);
	assert(param[p].type < PTYPE_NUM);
}

/* Verifies that the d-data is valid for the param stored at p, using the
 * verify() function specified for that type.
 */
static int param_verify_data(int p, const p_data_t d)
{
	p_type_t *type = NULL;
	param_is_in_range(p);
	/* This should be impossible, as ptype[] is rather static.
	 * If this ever happens, we are in big trouble and should quit
	 * before it's too late.
	 */
	assert(&ptype[param[p].type] != NULL);
	assert(ptype[param[p].type].verify);

	type = &ptype[param[p].type];

	if (type->verify(param[p].type, param[p].min, param[p].max, d)) {
		return 1;
	} else {
		inform(V(CONFIG), "Parameter-verification failed for "
			"%s of type %s",
			param[p].name,
			type->name);
		return 0;
	}
}

/* Verify the parameter p.
 *
 * XXX: Somewhat of a left-over at the moment since there isn't a lot we
 * 	know about a parameter besides it data at the moment.
 * FIXME: Update for new description format.
 */
static int param_verify(unsigned int p)
{
	param_is_in_range(p);
	assert(param[p].name);
	assert(param[p].description);
	assert(*param[p].name != '\0');
/*	assert(*param[p].description != '\0');
	i = strlen(param[p].description);
	if (i< 10) {
		inform(V(CONFIG), "Alarmingly short description of parameter "
			"\"%s\" found (%d characters long) during "
			"verification.", param[p].name, i);
	}
*/	return param_verify_data(p, param[p].d);
}

/* Inform if the param p is a datatype which we can know if has a value or
 * not (ie: a pointer to NULL). This can't check simple(int)-type params
 * since 0 is a valid value to set.
 *
 * XXX: This is mostly used for fetching, and may or may not be a problem.
 *	Bottom line: Check the value in the modules that know if NULL is
 *	valid or not.
 */
static void param_warn_if_unset(const p_enum_t p)
{
	if (PTYPE_IS_INT(param[p].type))
		return;
	if (param[p].d.v == NULL)
		inform(V(CONFIG), "null-parameter(%s) accessed  "
			"or fetched. This could be perfectly ok "
			"or have serious side-issues.", param[p].name);
}

/* Searches for the parameter with the name 'key' and returns the
 * index-number (p). Returns negative if it wasn't found.
 */
static int param_search_key(char *key)
{
	int i;
	assert(key);

	for (i = 0; i < P_NUM; i++) {
		if (!strcasecmp(param[i].name,key)) {
			return i;
		}
	}
	return -1;
}

/***************************************************************
 * Parameter type-specific verification, setting and printing.
 ***************************************************************/

/* Verifies that data contains an int of some sort and that it's within
 * the given range.
 *
 * XXX: Turns out that uint and int isn't the same. Perhaps using a larger
 * 	int to store the unsigned to get it within the same bounds... A bit
 * 	pointless just to avoid a tiny bit of code duplication.
 */
static int ptype_verify_simple(const p_type_enum_t type,
			      const int min,
			      const int max,
			      const p_data_t data)
{
	assert(PTYPE_IS_INT(type));

	if (type == PTYPE_UINT || type == PTYPE_MASK) {
		if (data.u < min || data.u > max) {
			inform(V(CONFIG), "Value of integer outside of range. "
					"(%d < %d < %d)", min, data.i, max);
			return 0;
		}
	} else {
		if (data.i < min || data.i > max) {
			inform(V(CONFIG), "Value of integer outside of range. "
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
 * Returns 0 for invalid string or the length of the string.
 */
static int ptype_verify_string(const p_type_enum_t type,
			      const int min,
			      const int max,
			      const p_data_t data)
{
	int i;

	assert(type == PTYPE_STRING);
	if (data.str == NULL) {
		inform(V(CONFIG),"NULL-pointer when verifying a string");
		return 0;
	}
	
	for(i = 0; i < MAX(max, WMD_MAX_STRING); i++) {
		if (data.str[i] == '\0')
			return i;
	}
	inform(V(CONFIG),"String is larger than expected");
	return 0;
}

/* Dummy-function until keys/actions are implemented */
static int ptype_verify_key(const p_type_enum_t type,
			   const int min,
			   const int max,
			   const p_data_t data)
{
	WMD_DUMMY_RETURN(0);
}
/* Set the value of the param p to that of data, assuming it can do by
 * assignment. Only valid for INT, UINT, BOOL etc, not pointer-data.
 *
 * Returns true if value was changed or already set.
 */
static int ptype_set_simple(int p, p_data_t data)
{
	int ret;
	param_is_in_range(p);
	ret = param_verify_data(p, data);
	if (!ret) {
		inform(V(CONFIG), "Failed to set a simple (int,uint,bool) parameter due to"
			" failed verification.");
		return 0;
	}

	/* XXX: During initialization, we want to make sure that we set
	 * 	everything up.
	 */	
	if (!memcmp(&param[p].d, &data, sizeof(p_data_t)) && STATE_IS(CONFIGURED)) {
		inform(V(CONFIG_CHANGES), "Not changing parameter \"%s\" - value already set.",
			param[p].name);	
		return 1;
	}

	param[p].d = data;
	assert(!memcmp(&param[p].d, &data, sizeof(p_data_t)));
	return 1;
}

static int ptype_set_string(int p, p_data_t data)
{
	WMD_DUMMY_RETURN(0);
}

static int ptype_set_key(int p, p_data_t data)
{
	WMD_DUMMY_RETURN(0);
}

static int ptype_print_key(int p, p_data_t d, FILE *fd)
{
	WMD_DUMMY_RETURN(0);
}

static int ptype_print_simple(int p, p_data_t d, FILE *fd)
{
	param_is_in_range(p);
	assert(PTYPE_IS_INT(param[p].type));
	switch(param[p].type) {
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
			inform(V(CORE),"The impossible happened! Abandon"
				" ship! (printing a simple parameter that"
				" is apparently not an integer, unsigned "
				"integer, bitmask or boolean. Now what...");
			return 0;
			break;
	}
	return 1;
}

static int ptype_print_string(int p, p_data_t d, FILE *fd)
{
	param_is_in_range(p);
	assert(param[p].type == PTYPE_STRING);
	fprintf(fd, "%s", d.str ? d.str : "(NULL)");
	return 1;
}

/***************************************************************
 * "API"/External access. Check. And. Verify. Everything.
 ***************************************************************/


/* Set the value of param[p] to that of d, if possible. Origin is the
 * origin of the value being set.
 *
 * Returns true on success.
 */
int param_set(int p, p_data_t d, int origin)
{
	int ret;
	param_is_in_range(p);
	if (origin < param[p].state) {
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
		inform(V(CONFIG_CHANGES), "Failed to set value of parameter "
			"\"%s\". *set() returned %d", ret);
	param[p].state = origin;
	return ret;
}

/* Parse a string to set a parameter.
 *
 * Typically passed directly from the config-engine, an argument or over
 * some other interactive means, thus ample error handling is needed.
 *
 * Return:
 *  0 - Parameter set.
 *  1 - Bad syntax (not foo=bar)
 *  2 - Unknown parameter.
 *  3 - Parameter-data failed verification.
 */
int param_parse(char *str, int origin)
{
	char *sep = NULL;
	// FIXME: Hardcoded max-length.
	char key[1024];
	int p = -1;
	int length = 0;
	char *tmp;

	if (str == NULL) {
		inform(V(CONFIG), "Not parsing NULL-string as a parameter");
		return 1;
	}
	
	sep = index(str,'=');
	if (sep == NULL) {
		inform(V(CONFIG), "Missing '=' in parameter "
			"key-value pair: %s", str);
		return 1;
	}
	length = sep - str;
	if(length >= 1024) {
		inform(V(CONFIG), "Parameter-name absurdly long?");
		return 1;
	}
	sep++;	
	tmp = strncpy(key, str, length);
	assert(tmp == key);
	
	key[length] = '\0';

	p = param_search_key(key);

	if (p < 0) {
		inform(V(CONFIG),"Unknown parameter: %s", key);
		return 2;
	}
	inform(V(CONFIG), "Parsed parameter string to: "
		"key: %s. length: %d, str: %s, p: %d",
		key, length, str, p);
	if (!strcasecmp(sep,"default")) {
		assert(param_set_default(p, origin));
		return 0;
	}
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
int param_set_default(int p, int origin)
{
	int ret = 0;
	if (p == -1) {
		if (STATE_IS(CONFIGURED))
			inform(V(CONFIG), "Resetting values for all parameters to default");
		for (p = 0; p < P_NUM; p++)
			ret += ! param_set(p, param[p].default_d, origin);
		return !ret;
	}

	if (STATE_IS(CONFIGURED))
		inform(V(CONFIG_CHANGES), "Resetting value of parameter "
			"\"%s\" to default", param[p].name);
	return param_set(p, param[p].default_d, origin);
}

/* Print information about a single parameter, or all parameter (p == -1).
 * what defines what is printed, so it could be suitable for using in a
 * configuration file, passing to a pipe, --help, man-file or whatever
 * else.
 *
 * XXX: This is a bit extensive in length, but fairly simple, so bare with
 * 	it for now.
 */
#define WB(s) ((P_WHAT_BIT(s) & what) == P_WHAT_BIT(s))
void param_show(FILE *fd, int p, unsigned int what)
{
	char *comment = "";
	if(p == -1) {
		for(p=0; p < P_NUM; p++)
			param_show(fd, p, what);
		return;
	}
	param_is_in_range(p);
	if (!WB(STATE_DEFAULTS)) {
		if (param[p].state == P_STATE_DEFAULT)
			return;
	}
	if (WB(COMMENT)) {
		fprintf(fd, "\n");
		comment = "/* ";
	}

	if (WB(BOILER)) {
		fprintf(fd, "%s%-14s %-8s %-10d %d\n",
			comment,
			param[p].name,
			ptype[param[p].type].name,
			param[p].min,
			param[p].max);
		comment = " * ";
	}
	
	if (WB(VALUE)) {
		fprintf(fd, "%s%-15s",comment,"Value:");
		ptype[param[p].type].print(p, param[p].d, fd);
		fprintf(fd, "\n");
		comment = " * ";
	}

	if (WB(DEFAULT)) {
		fprintf(fd, "%s%-15s", comment, "Default:");
		ptype[param[p].type].print(p, param[p].default_d, fd);
		fprintf(fd, "\n");
		comment = " * ";
	}
	if (WB(SOURCE)) {
		fprintf(fd, "%s%-15s",comment, "Source:");
		switch(param[p].state) {
			case P_STATE_DEFAULT:
				fprintf(fd, "Default");
				break;
			case P_STATE_CONFIG:
				fprintf(fd, "Configuration file");
				break;
			case P_STATE_ARGV:
				fprintf(fd, "Argument");
				break;
			case P_STATE_USER:
				fprintf(fd, "User-set at run-time");
				break;
			default:
				assert("Fell through to the default-case "
					"while describing the parameter "
					"state.");
		}
		fprintf(fd, "\n");
		comment = " * ";
	}

	if (WB(DESCRIPTION)) {
		for (int i = 0; param[p].description[i] != NULL; i++) {
			fprintf(fd, "%s%s", comment, param[p].description[i]);
		}
		comment = " * ";
	}

	if (WB(COMMENT))
		fprintf(fd, " */\n");

	if (WB(KEYVALUE)) {
		fprintf(fd, "%s=", param[p].name);
		ptype[param[p].type].print(p, param[p].d, fd);
		fprintf(fd, "\n");
	}
}
#undef WB

/* Return the data of the param p. Should be accessed through the P()
 * macro.
 *
 * XXX: A method to enforce? Is this necessary?
 */
p_data_t param_get_data(p_enum_t p) {
	param_is_in_range(p);
	param_warn_if_unset(p);
	return param[p].d;
}

