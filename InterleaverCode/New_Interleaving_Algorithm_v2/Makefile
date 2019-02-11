CC=gcc
CFLAGS=-Wall -g

default : interleaver


interleaver : interleaver.o hash_drgb.o sha256.o
	$(CC) -o $@ $(CFLAGS) $< hash_drgb.o sha256.o

interleaver.o : interleaver.c hash_drgb.h sha256.h
	$(CC) -c -o $@ $(CFLAGS) $<

hash_drgb.o : hash_drgb.c hash_drgb.h
	$(CC) -c -o $@ $(CFLAGS) $<

sha256.o : sha256.c sha256.h
	$(CC) -c -o $@ $(CFLAGS) $<


clean : 
	rm *.o
	rm interleaver
	rm *.tmp