ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libaribb24.la
libaribb24_la_SOURCES = src/decoder.c src/parser.c src/md5.c src/aribb24.c src/drcs.c
libaribb24_la_LIBADD = $(PNG_LIBS)
libaribb24_la_CFLAGS = -Wall -fvisibility=hidden

pkginclude_HEADERS = src/aribb24/decoder.h src/aribb24/parser.h src/aribb24/bits.h src/aribb24/aribb24.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = src/aribb24.pc
