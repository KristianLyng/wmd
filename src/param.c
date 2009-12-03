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
 * We want as few entry-points the parameters as possible to assure a
 * consistent behavior.
 *
 * XXX: We may want to split the actual values into it's own file to avoid
 * 	mixing the values of the parameters and the framework to use it.
 *
 * XXX: A param-test framework should be included to dry-run param-changes.
 * 	This is particularly important for bindings.
 */

#include <sys/param.h>
#include <string.h>

#include "param.h"
#include "param-private.h"
#include "com.h"
#include "core.h"

/* Param Definition (...).
 * Ensures it's mapped to the enum regardless of sorting.
 */
#define PD(name,type,def,field,min,max,desc) 	\
	[P_ ## name] =				\
	{ #name, desc, PTYPE_ ## type,		\
	{ .field = def },			\
	{ .field = def }, min, max },

/* Actual settings matching the param enum in param.h
 *
 * XXX: Probably also want to move this somewhere I suppose, it's
 * 	reasonably important, after all.
 */
t_param param[P_NUM] = {
	PD(REPLACE,	BOOL,	0,	b,
		0,	1,
		"If set to true, wmd will attempt to replace the running \n"
		"window manager, otherwise, it will exit if a window \n"
		"manager already has control over the X session.")

	PD(SYNC,	BOOL,	0,	b,
		0,	1,
		"Run in synchronized X-mode. Easier debugging but \n"
		"slower, since we have to wait for X.")

	PD(VERBOSITY, 	MASK,	(UINT_MAX ^ (1<<VER_FILELINE)),	u,
		0,	UINT_MAX,
		"The verbosity is a bitmask to filter out what sort of \n"
		"information is sent, and how detailed it is. See \n"
		"FIXME:arglist for a list of bits and what they do.\n"
		"You probably want to inverse the mask to see what's\n"
		"disabled instead of what's enabled (which is everything\n"
		"except FILELINE by default.")
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

/* Different param-types we support. Not all min/max values are actually
 * used, as that depends on the specific check function.
 *
 */
#define PA(name, family) 					\
	[PTYPE_ ## name] = {					\
		PTYPE_ ## name, #name,				\
		ptype_set_ ## family,				\
		ptype_print_ ## family, 			\
		ptype_verify_ ## family },

static t_ptype ptype[PTYPE_NUM] = {
	PA(BOOL, simple)
	PA(UINT, simple)
	PA(INT, simple)
	PA(MASK, simple)
	PA(STRING, string)
	PA(KEY,	key)
};

#undef PA

/* Verifies that data contains an int of some sort and that it's within
 * the given range.
 *
 * XXX: Turns out that uint and int isn't the same. Perhaps using a larger
 * 	int to store the unsigned to get it within the same bounds... A bit
 * 	pointless just to avoid a tiny bit of code duplication.
 */
static int ptype_verify_simple(const paramtype type,
			      const int min,
			      const int max,
			      const t_data data)
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
static int ptype_verify_string(const paramtype type,
			      const int min,
			      const int max,
			      const t_data data)
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
static int ptype_verify_key(const paramtype type,
			   const int min,
			   const int max,
			   const t_data data)
{
	WMD_DUMMY_RETURN(0);
}

/* Just checks the range of p. Since this should never fail, asserts will
 * do.
 */
static void param_is_in_range(int p)
{
	assert(p >= 0);
	assert(p < P_NUM);
}

static int param_verify_data(int p, const t_data d)
{
	t_ptype *type = NULL;
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

/* Set the value of the param p to that of data, assuming it can do by
 * assignment. Only valid for INT, UINT, BOOL etc, not pointer-data.
 *
 * Returns true if value was changed or already set.
 */
static int ptype_set_simple(int p, t_data data)
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
	if (!memcmp(&param[p].d, &data, sizeof(t_data)) && STATE_IS(CONFIGURED)) {
		inform(V(CONFIG_CHANGES), "Not setting parameter %s - value already set.",
			param[p].description);	
		return 1;
	}

	param[p].d = data;
	assert(param_verify(p));
	assert(!memcmp(&param[p].d, &data, sizeof(t_data)));
	return 1;
}

static int ptype_set_string(int p, t_data data)
{
	WMD_DUMMY_RETURN(0);
}
static int ptype_set_key(int p, t_data data)
{
	WMD_DUMMY_RETURN(0);
}

static int ptype_print_key(int p, t_data d, FILE *fd)
{
	WMD_DUMMY_RETURN(0);
}
static int ptype_print_simple(int p, t_data d, FILE *fd)
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
			fprintf(fd, "%s", d.b ? "TRUE" : "FALSE");
			break;
		default:
			assert("Reached default-case when it should be "
				"impossible. Is PTYPE_IS_INT() in sync?"
				"(And yeah, this is an assert)");
			inform(V(CORE),"The impossible happened! Abandon"
				" ship! (printing a simple parameter that"
				" is apparently not an integer, unsigned "
				"integer or boolean. Now what...");
			return 0;
			break;
	}
	return 1;
}
static int ptype_print_string(int p, t_data d, FILE *fd)
{
	param_is_in_range(p);
	assert(param[p].type == PTYPE_STRING);
	fprintf(fd, "%s", d.str ? d.str : "(NULL)");
	return 1;
}

static void param_warn_if_unset(const t_param_enum p)
{
	if (PTYPE_IS_INT(param[p].type))
		return;
	if (param[p].d.v == NULL)
	if (param[p].d.v == NULL)
		inform(V(CONFIG), "Fetching a null-parameter(%s).",
			param[p].name);
}

/* Set the default value for param p, or for all parameters if p is -1.
 * Returns true if successful.
 */
int param_set_default(int p)
{
	int i;
	int ret;
	if (p == -1) {
		inform(V(CONFIG), "Setting default values for all parameters");
		for (i = 0; i < P_NUM; i++) {
			ret += ! param_set_default(i);
		}
		return 1;
	}

	param_is_in_range(p);
	return ptype[param[p].type].set(p, param[p].default_d);
}

/* Sanity check and verify a param and it's value based on the
 * verification function.
 *
 * Returns true/false based on success.
 */
int param_verify(int p)
{
	int i = 0;
	int ret = 0;
	if(p == -1) {
		for(i = 0; i < P_NUM; i++)
			ret += param_verify(i);
		return ret;
	}
	param_is_in_range(p);

	return param_verify_data(p, param[p].d);
}

/* Describe the param p on fd. If p is -1, all parameters are described. */
void param_describe(FILE *fd, int p)
{
	int i;
	if(p == -1) {
		for(i=0; i < P_NUM; i++)
			param_describe(fd, i);
		return;
	}
	fprintf(fd, "%-14s %-8s %-10d %d\n",
		param[p].name,
		ptype[param[p].type].name,
		param[p].min,
		param[p].max);
	fprintf(fd, "%-15s", "Default:");
	ptype[param[p].type].print(p, param[p].default_d, fd);
	fprintf(fd, "\n");
	fprintf(fd, "%-15s","Value:");
	ptype[param[p].type].print(p, param[p].d, fd);
	fprintf(fd, "\n");
	fprintf(fd, "%s\n\n", param[p].description);
}

/* List the param p on fd (name, type and value). If p is -1, all
 * parameters are listed
 */
void param_list(FILE *fd, int p)
{
	int i;
	if(p == -1) {
		for(i=0; i < P_NUM; i++)
			param_list(fd, i);
		return;
	}
	fprintf(fd, "%-14s %-8s\t\t",
		param[p].name,
		ptype[param[p].type].name);
	ptype[param[p].type].print(p, param[p].d, fd);
	fprintf(fd, "\n");
}
t_data param_get_data(t_param_enum p) {
	param_is_in_range(p);
	param_warn_if_unset(p);
	return param[p].d;
}

