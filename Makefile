CC     = gcc
CFLAGS = -Wall -Wextra -std=c23 -pedantic `pkg-config --cflags sdl2` -ggdb -fsanitize=address
LIBS   = `pkg-config --libs sdl2` -lm

main : main.c
	$(CC) $(CFLAGS) -o main main.c $(LIBS)

