lib_LTLIBRARIES = libaribb24.la
libaribb24_la_SOURCES = decoder.c parser.c md5.c aribb24.c
libaribb24_la_LIBADD = -lpng
libaribb24_la_CFLAGS = -Wall -fvisibility=hidden
pkginclude_HEADERS = aribb24/decoder.h aribb24/parser.h aribb24/bits.h aribb24/aribb24.h
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = aribb24.pc
CLEANFILES = ${pkgconfig_DATA}

