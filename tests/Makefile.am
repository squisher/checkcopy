INCLUDES = 					\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" 	\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/src

bin_PROGRAMS = checksum

checksum_headers = 				\
	checkcopy-file-info.h			\
	checkcopy-checksum-type.h		\
	checkcopy-input-stream.h

checksum_SOURCES = 				\
	$(checksum_headers)			\
	../src/checkcopy-input-stream.c		\
	../src/checkcopy-file-info.c		\
	../src/checkcopy-checksum-type.c	\
	checksum.c

checksum_CFLAGS = 				\
	$(MD5COPY_CFLAGS)			\
	$(GTK_CFLAGS)				\
	$(LIBXFCE4UTIL_CFLAGS)			\
	$(ZLIB_CFLAGS)				\
	$(GTHREAD_CFLAGS)

checksum_LDADD = 				\
	$(MD5COPY_LIBS)				\
	$(LIBXFCE4UTIL_LIBS)			\
	$(GTK_LIBS)				\
	$(ZLIB_LIBS)				\
	$(GTHREAD_LIBS)

# vi:set ts=8 sw=8 noet ai nocindent softtabstop=0 list listchars=tab\:>-:
