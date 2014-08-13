ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libaribb24.la
libaribb24_la_SOURCES = src/aribb24.c src/aribb24_private.h	\
	src/decoder.c src/decoder_private.h src/parser.c	\
	src/parser_private.h src/md5.c src/md5.h src/drcs.c	\
	src/drcs.h src/convtable.h			\
	src/decoder_macro.h
libaribb24_la_LIBADD = $(PNG_LIBS)
libaribb24_la_CFLAGS = -Wall -fvisibility=hidden

pkginclude_HEADERS = src/aribb24/decoder.h src/aribb24/parser.h	\
	src/aribb24/bits.h src/aribb24/aribb24.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = src/aribb24.pc

dist_doc_DATA = README.md COPYING
