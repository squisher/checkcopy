bin_PROGRAMS=md5copy
md5copy_headers=                                \
        error.h                                 \
        global.h                                \
        progress-dialog.h                       \
        ring-buffer.h                           \
        settings.h                              \
        thread-copy.h                           \
        thread-hash.h
        
md5copy_SOURCES=                                \
        $(md5copy_headers)                      \
        md5copy.c                               \
        error.c                                 \
        progress-dialog.c                       \
        ring-buffer.c                           \
        thread-copy.c                           \
        thread-hash.c

md5copy_CFLAGS=                                 \
        $(MD5COPY_CFLAGS)                       \
        $(GTHREAD_CFLAGS)                       

md5copy_LDADD=                                  \
        $(MD5COPY_LIBS)                         \
        $(GTHREAD_LIBS)                         
