CC = gcc#clang
CFLAGS = #-I/tmp/usr/include -g

LD = gcc# clang
LDFLAGS =# /tmp/usr/lib/libcandl.a /tmp/usr/lib/libosl.a /tmp/usr/lib/libpiplib_gmp.a -lclay -lgmp -g


.PHONY: all
all: clope

clope: clope.o
	$(LD) $< $(LDFLAGS) -o clope

clope.o: clope.c clope.h
	$(CC) $(CFLAGS) -c $<

