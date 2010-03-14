AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CFLAGS = -Wall -Werror

bin_PROGRAMS = wmd
wmd_SOURCES = main.c param.c inform.c arg.c config.c x.c
wmd_LDADD = $(xcb_LIBS)


${top_srcdir}/include/param.h: generate_structs.tcl

${top_srcdir}/include/param-list.c: generate_structs.tcl
	cd $(top_srcdir)/src/ && @TCLSH@ generate_structs.tcl

${top_srcdir}/include/param-list.h: generate_structs.tcl
	cd $(top_srcdir)/src/ && @TCLSH@ generate_structs.tcl

${top_srcdir}/include/verbosities.c:generate_structs.tcl
	cd $(top_srcdir)/src/ && @TCLSH@ generate_structs.tcl

${top_srcdir}/include/verbosities.h: generate_structs.tcl
	cd $(top_srcdir)/src/ && @TCLSH@ generate_structs.tcl
	
