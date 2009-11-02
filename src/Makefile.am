INCLUDES = 					\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" 	\
	-I$(top_srcdir)

bin_PROGRAMS = md5copy
md5copy_headers = 				\
	error.h					\
	global.h				\
	progress-dialog.h			\
	ring-buffer.h				\
	settings.h				\
	thread-copy.h				\
	thread-hash.h				\
	checkcopy-cancel.h			\
	checkcopy-file-handler.h		\
	checkcopy-planner.h			\
	checkcopy-traversal.h

md5copy_SOURCES = 				\
	$(md5copy_headers)			\
	md5copy.c				\
	error.c					\
	progress-dialog.c			\
	ring-buffer.c				\
	thread-copy.c				\
	thread-hash.c				\
	checkcopy-cancel.c			\
	checkcopy-file-handler.c		\
	checkcopy-planner.c			\
	checkcopy-traversal.c

md5copy_CFLAGS = 				\
	$(MD5COPY_CFLAGS)			\
	$(LIBXFCEGUI4_CFLAGS)			\
	$(GTHREAD_CFLAGS)

md5copy_LDADD = 				\
	$(MD5COPY_LIBS)				\
	$(LIBXFCEGUI4_LIBS)			\
	$(GTHREAD_LIBS)

# vi:set ts=8 sw=8 noet ai nocindent softtabstop=0 list listchars=tab\:>-:
