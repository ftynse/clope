CC = gcc
CFLAGS = -Wall

LD = gcc
LDFLAGS = -lclay -lcandl -losl -lpiplib_gmp -lgmp


.PHONY: all
all: clope

clope: clope.o options.o
	$(LD) $^ $(LDFLAGS) -o clope

clope.o: clope.c clope.h
	$(CC) $(CFLAGS) -c $<

options.o: options.c options.h
	$(CC) $(CFLAGS) -c $<
