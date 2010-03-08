AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CFLAGS = -Wall -Werror

bin_PROGRAMS = wmd
wmd_SOURCES = main.c param.c inform.c arg.c config.c x.c
wmd_LDADD = $(xcb_LIBS)

$(top_srcdir)/include/param-list.c: $(srcdir)/generate_structs.tcl
	cd $(srcdir) && @TCLSH@ generate_structs.tcl || true

$(top_srcdir)/include/param-list.h: $(srcdir)/generate_structs.tcl
	cd $(srcdir) && @TCLSH@ generate_structs.tcl || true
