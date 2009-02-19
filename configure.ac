dnl Initialize autoconf
AC_COPYRIGHT([Copyright (c) 2008-2009 David Mohr <david@mcbf.net>])
AC_INIT([md5copy], [0.2], [david@mcbf.net])
AC_PREREQ([2.61])
AC_CONFIG_SRCDIR([src/md5copy.c])
AC_CONFIG_AUX_DIR([build-aux])

dnl Initialize automake
AM_INIT_AUTOMAKE([AC_PACKAGE_TARNAME()], [AC_PACKAGE_VERSION()], [-Wall 1.10.1 dist-bzip2])
AM_CONFIG_HEADER([config.h])
AM_MAINTAINER_MODE()


AC_PROG_CC()
AC_PROG_INSTALL()
AC_PROG_LIBTOOL()
AC_PROG_INTLTOOL([0.31], [no-xml])

dnl AM_PROG_CC_C_O
dnl PKG_CHECK_MODULES(PACKAGE, gtk+-2.0)

XDT_I18N(@LINGUAS@)

XDT_CHECK_PACKAGE([GTHREAD], [gthread-2.0], [2.6.0])
XDT_CHECK_PACKAGE([GTK], [gtk+-2.0], [2.10.0])

dnl AC_CONFIG_HEADERS([config.h])
AC_CHECK_HEADERS([string.h])
AC_CONFIG_FILES([
  Makefile
  po/Makefile.in
  src/Makefile
])
AC_OUTPUT
