INCLUDES = 					\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" 	\
	-I$(top_srcdir)

bin_PROGRAMS = checkcopy
checkcopy_headers = 				\
	error.h					\
	ompa-list.h				\
	progress-dialog.h			\
	checkcopy-cancel.h			\
	checkcopy-file-handler.h		\
	checkcopy-file-handler-base.h		\
	checkcopy-planner.h			\
	checkcopy-processor.h			\
	checkcopy-input-stream.h		\
	checkcopy-worker.h			\
	checkcopy-file-info.h			\
	checkcopy-file-list.h			\
	checkcopy-details-window.h		\
	checkcopy-checksum-types.h		\
	checkcopy-traversal.h

checkcopy_SOURCES = 				\
	$(checkcopy_headers)			\
	ompa-list.c				\
	checkcopy-main.c			\
	error.c					\
	progress-dialog.c			\
	checkcopy-cancel.c			\
	checkcopy-file-handler.c		\
	checkcopy-file-handler-base.c		\
	checkcopy-planner.c			\
	checkcopy-processor.c			\
	checkcopy-input-stream.c		\
	checkcopy-worker.c			\
	checkcopy-file-info.c			\
	checkcopy-file-list.c			\
	checkcopy-details-window.c		\
	checkcopy-checksum-type.c			\
	checkcopy-traversal.c

checkcopy_CFLAGS = 				\
	$(MD5COPY_CFLAGS)			\
	$(GTK_CFLAGS)				\
	$(LIBXFCE4UTIL_CFLAGS)			\
	$(ZLIB_CFLAGS)				\
	$(GTHREAD_CFLAGS)

checkcopy_LDADD = 				\
	$(MD5COPY_LIBS)				\
	$(LIBXFCE4UTIL_LIBS)			\
	$(GTK_LIBS)				\
	$(ZLIB_LIBS)				\
	$(GTHREAD_LIBS)

# vi:set ts=8 sw=8 noet ai nocindent softtabstop=0 list listchars=tab\:>-:
