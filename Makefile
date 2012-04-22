CC = g++

all: main.o
	$(CC) main.o `sdl-config --libs` -lSDL_ttf -lSDL_mixer -lSDL_image  -lpng

main.o: main.cpp
	$(CC) -Wall -g -c main.cpp `sdl-config --cflags` -o main.o
