CC=g++
CFLAGS=-Wall -std=c++11 -ggdb3

all: grandkid.o kid.o myprime.o prime_functions.o ordered_list.o grandkid kid myprime

kid.o: kid.cpp
	$(CC) $(CFLAGS) -c kid.cpp

grandkid.o: grandkid.cpp
	$(CC) $(CFLAGS) -c grandkid.cpp

myprime.o: myprime.cpp
	$(CC) $(CFLAGS) -c myprime.cpp

prime_functions.o: prime_functions.cpp
	$(CC) $(CFLAGS) -c prime_functions.cpp

ordered_list.o: ordered_list.cpp
	$(CC) $(CFLAGS) -c ordered_list.cpp

grandkid: grandkid.o prime_functions.o
	$(CC) $(CFLAGS) -o grandkid grandkid.o prime_functions.o

kid: kid.o ordered_list.o
	$(CC) $(CFLAGS) -o kid kid.o ordered_list.o

myprime: myprime.o ordered_list.o
	$(CC) $(CFLAGS) -o myprime myprime.o ordered_list.o

.PHONY: clean

clean:
	rm -f grandkid.o kid.o myprime.o prime_functions.o ordered_list.o grandkid kid myprime