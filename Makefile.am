SUBDIRS = herd

DIST_SUBDIRS = herd

dist-hook:
	rm -rf `find $(distdir)/packaging -name CVS`
	rm -rf `find $(distdir)/packaging -name .svn`

EXTRA_DIST = \
	packaging/*.in	\
	packaging/herd.init.*

mrproper:
	make clean ||:
	find . -name Makefile -exec rm {} \;
	find . -name Makefile.in -exec rm {} \;
	rm -fr herd/.deps
	rm -f configure config.log config.h config.status libtool
	rm -f aclocal.m4 stamp-h1
	rm -fr autom4te.cache
	rm -f packaging/*.spec
	rm -fr herd/.libs

