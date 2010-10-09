dnl Process this file with autoconf to produce a configure script.

AC_INIT([herd],[3.0])

AC_CONFIG_SRCDIR(herd/main.c)
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE
AC_CANONICAL_HOST

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_CHECK_PROG(AR,ar,ar)
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

dnl Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h getopt.h malloc.h unistd.h sys/mman.h sched.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN
AC_TYPE_SIGNAL
AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)


AC_DEFINE([HAVE_OPENIPMI_SMI], [], [We have a local IPMI interface])
AC_SUBST(OPENIPMI_SMI)

AC_CONFIG_FILES([
	herd/Makefile
	packaging/herd.rhel4.spec
	packaging/herd.rhel5.spec
	packaging/herd.sles9.spec
	packaging/herd.sles10.spec
        Makefile
        ])

AC_OUTPUT
