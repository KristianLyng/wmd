#!/usr/bin/tclsh8.4
# wmd - Parameter and verbosity-level generation
# Copyright (C) 2010 Kristian Lyngstol <kristian@bohemians.org>
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
# Generates various .h and .c files for parameters. One place to edit
# instead of two.

# Used to generate the param_type_id enum
set types {bool uint int mask string key}

# Parameters.
#
# TCL-syntax means {} and "" is roughly the same. The indentation is a bit
# funky here to avoid sliding the descriptions too far to the right.
#
# XXX: Need an integer-demo too, and support for uint, int and key.
set params {
	{replace	BOOL	true {
		"If set to true, wmd will attempt to replace the running"
		"window manager, otherwise, it will exit if a window manager"
		"already has control over the X session"
		""
		"For this to work, the running WM must understand the"
		"underlying protocol"
		"FIXME: Clarification"
	}}
	{sync		BOOL	false {
		"Run in synchronized X-mode."
		""
		"Easier to tebug, but slower since we have to wait for X"
	}}
	{verbosity	MASK
	 {(UINT_MAX ^ ((1<<VER_FILELINE|(1<<VER_STATE))))} {
		"Bitmask deciding how verbose wmd should be"
		""
		"See --help verbosity for a list of possibilites"
		""
		"Cheat sheet: 0: display nothing. -1: Display everything"
	}}
	{config		string	 "~/.config/wmd" {
		"The location of the configuration files"
	}}
}

#########
# Actual parsing starts here. Normally no need to modify it.
#########

# Print the boilerplate-warning to avoid confusing people.
proc warn {fd} {
	puts $fd "/*"
	puts $fd " * Automatically generated from gen_objects.tcl."
	puts $fd " *"
	puts $fd " * Do not modify. Modify gen_objects.tcl instead"
	puts $fd " */"
	puts $fd ""
}

# The header-file for paramters
set head [open "../include/param-list.h" w]

warn $head
set n 0
puts $head "enum param_type_id {"
foreach type $types {
	puts -nonewline $head "\tPTYPE_[string toupper $type]"
	if {$n == 0} {
		puts -nonewline $head " = 0"
	}
	incr n
	puts $head ","
}
puts $head "\tPTYPE_NUM"
puts $head "};"
puts $head ""

set n 0
puts $head "enum param_id {"
foreach param $params {
	puts -nonewline $head "\tP_[string tolower [lindex $param 0]]"
	if {$n == 0} {
		puts -nonewline $head " = 0"
	}
	incr n
	puts $head ","
}

puts $head "\tP_NUM,"
# P_ALL is used to signal to a function to iterate all parameters
puts $head "\tP_ALL"
puts $head "};"
puts $head ""

# Actual data structure for parameters
set c [open "../include/param-list.c" w]
warn $c

puts $c "
/*
 * Ensures that the parameter is mapped to the right enum.
 *
 * __VA_ARGS__ is a bit hacky right now. Better suggestions?
 */
"

puts $c "
#define PD(name,type,def,field,min,max, ...) 	\\
	\[P_ ## name\] =				\\
	{ #name, PTYPE_ ## type, 0,		\\
	{ .v = NULL },				\\
	{ .field = def }, min, max , 		\\
	{ __VA_ARGS__ , NULL } },
"
puts $c "
/*
 * Actual settings matching the param enum in param.h
 *
 * Arguments:
 * name (P_name must exist in param.h)
 * type (See param-private.h, or the list underneath)
 * default value
 * field-name in the data-struct
 * minimum value (int)
 * maximum value (int)
 * description (separate lines with commas for formatting)
 *
 * XXX: Note that the min/max are not necessarily used, it depends on the
 * 	type of parameter.
 *
 * FIXME: The min/max should probably be printed by the type, to avoid
 * 	showing -1 as the max for MASK, for instance.
 *
 * FIXME: the field-name shouldn't be necessary.
 */
"
puts $c "static struct param param\[P_NUM\] = {"

set n 0
foreach param $params {
	set name [lindex $param 0]
	set type [string toupper [lindex $param 1]]
	set def [string toupper [lindex $param 2]]
	set min 0
	set max 0
	set bf ""
	if {$n != 0} {
		puts $c ""
	}
	incr n
	if {$type == "BOOL"} {
		set min 0
		set max 1
		if {$def == "TRUE"} {
			set def "1"
		} elseif {$def == "FALSE"} {
			set def "0"
		} else {
			puts "Error: Boolean type with non-boolean default: ${name}"
			exit 1
		}
		set bf "b"
	} elseif {$type == "STRING"} {
		set min 0
		set max 65565
		if {$def != "NULL"} {
			set def "\"[lindex $param 2]\""
		}
		set bf "str"
	} elseif {$type == "MASK"} {
		set min 0
		set max "UINT_MAX"
		set bf "u"
	} else {
		puts "Unsupported param-type: ${type}."
		exit 1
	}
	puts $c "PD(${name}, ${type}, ${def}, ${bf}, ${min}, ${max}, "
	set mn 0
	foreach com [lindex $param 3] {
		if {$mn != 0} {
			puts $c ","
		}
		incr mn
		puts -nonewline $c "\t\"${com}\""
	}
	puts -nonewline $c ")"
}
puts $c "};"
puts $c "#undef PD"
