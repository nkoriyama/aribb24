AC_PREREQ([2.69])
AC_INIT([aribb24], [1.0.3], [nkoriyama@gmail.com])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_CONFIG_MACRO_DIR([m4])

AC_USE_SYSTEM_EXTENSIONS

AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
LT_INIT

# Checks for libraries.
pkg_requires=""
PKG_CHECK_MODULES(PNG, "libpng", [
  AC_DEFINE(HAVE_PNG, 1, "have libpng")
  pkg_requires="libpng"
], [])

AC_SUBST([PKG_REQUIRES], [$(test x$enable_shared = xno && echo ${pkg_requires})])

AC_CHECK_FUNCS([vasprintf])

AC_CONFIG_FILES([Makefile src/aribb24.pc])
AC_OUTPUT
