all: main.c
	gcc -g -o spaceinvaders main.c -lglfw -lGLEW -lGL
