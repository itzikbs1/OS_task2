CC = gcc
AR = ar
FLAGS = -Wall -g

OBJECTS_Shell = shell.o
OBJECTS_Server =  server.o
SOURCES = shell.c, server.c

all: server shell

server: $(OBJECTS_Server)
	$(CC) $(FLAGS) -o Server $(OBJECTS_Server)

shell: $(OBJECTS_Shell) lib_shell.a
	$(CC) $(FLAGS) -o shell $(OBJECTS_Shell) lib_shell.a 

#Make commands as singles:

lib_shell.a: $(OBJECTS_Shell) #static library for shell
	$(AR) -rcs lib_shell.a $(OBJECTS_Shell)

shell.o: shell.c# shell.h
	$(CC) $(FLAGS) -c shell.c


.PHONY: clean all

clean:
	rm -f *.o *.so Shell Server lib_shell.a