INCLUDES = 					\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" 	\
	-I$(top_srcdir)

bin_PROGRAMS = checkcopy
checkcopy_headers = 				\
	error.h					\
	progress-dialog.h			\
	checkcopy-cancel.h			\
	checkcopy-file-handler.h		\
	checkcopy-planner.h			\
	checkcopy-processor.h			\
	checkcopy-input-stream.h		\
	checkcopy-traversal.h

checkcopy_SOURCES = 				\
	$(checkcopy_headers)			\
	checkcopy.c				\
	error.c					\
	progress-dialog.c			\
	checkcopy-cancel.c			\
	checkcopy-file-handler.c		\
	checkcopy-planner.c			\
	checkcopy-processor.c			\
	checkcopy-input-stream.c		\
	checkcopy-traversal.c

checkcopy_CFLAGS = 				\
	$(MD5COPY_CFLAGS)			\
	$(LIBXFCEGUI4_CFLAGS)			\
	$(GTHREAD_CFLAGS)

checkcopy_LDADD = 				\
	$(MD5COPY_LIBS)				\
	$(LIBXFCEGUI4_LIBS)			\
	$(GTHREAD_LIBS)

# vi:set ts=8 sw=8 noet ai nocindent softtabstop=0 list listchars=tab\:>-:
