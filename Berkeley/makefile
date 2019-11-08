CC=g++

compile: berkeley_time_demon
berkeley_time_demon: berkeley_time_demon.o
	$(CC) -o berkeley_time_demon berkeley_time_demon.o -lpthread
berkeley_time_demon.o: berkeley_time_demon.cpp
	$(CC) -c berkeley_time_demon.cpp -lpthread