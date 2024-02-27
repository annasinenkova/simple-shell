CC = gcc
CFLAGS = -O2 -ftrapv -fsanitize=undefined -Wall -Werror -Wformat-security -Wignored-qualifiers -Winit-self -Wswitch-default -Wfloat-equal -Wshadow -Wpointer-arith -Wtype-limits -Wempty-body -Wlogical-op -Wstrict-prototypes -Wold-style-declaration -Wold-style-definition -Wmissing-parameter-type -Wmissing-field-initializers -Wnested-externs -Wno-pointer-sign -std=gnu11 -lm

all: My_Shell clean

My_Shell: command.o main.o
	$(CC) $(CFLAGS) main.o command.o -o My_Shell

command.o: command.c command.h
	$(CC) $(CFLAGS) -c command.c -o command.o
	
main.o: main.c command.h
	$(CC) $(CFLAGS) -c main.c -o main.o
	
clean:
	rm command.o main.o
