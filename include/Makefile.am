nobase_noinst_HEADERS = core.h param.h param-private.h inform.h WIP.h
CLEANFILES = WIP.h
WIP.h: $(top_srcdir)/WIP Makefile
	echo "/* Generated from $(top_srcdir)/WIP at build time */" >$@
	awk 'BEGIN{ print "const char *WIP_list[] = { " }; /^-/ { print "\t\""$$0"\","; }; END{print "\tNULL\n};"};' < $(top_srcdir)/WIP >>$@
	
