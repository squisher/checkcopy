CFLAGS=`pkg-config --cflags gtk+-2.0` -Wall -g -DDEBUG=1 -DSTATS
LDFLAGS= `pkg-config --libs gtk+-2.0` -lmhash `pkg-config --libs gthread-2.0`

all: md5copy

md5copy: md5copy.o progress-dialog.o ring-buffer.o
ring-buffer-test: ring-buffer-test.o ring-buffer.o

md5copy.o: md5copy.c global.h
progress-dialog.o: progress-dialog.c
ring-buffer.o: ring-buffer.c global.h

clean:
	rm -f md5copy ring-buffer-test *.o
