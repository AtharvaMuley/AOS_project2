CC=g++

compile: all

all:
	g++ -std=c++11 -o causal_multicast causal_multicast.cpp -lpthread
	g++ -std=c++11 -o fifo_multicast fifo_multicast.cpp -lpthread

clean:
	rm causal_multicast fifo_multicast