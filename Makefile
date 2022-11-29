CC = gcc
AR = ar
FLAGS = -Wall -g

OBJECTS_Shell = shell.o

all: shell tee nc

tee: mytee.o
	$(CC) $(FLAGS) -o tee mytee.o

nc: nc.o 
	$(CC) $(FLAGS) -o nc nc.o

shell: $(OBJECTS_Shell)
	$(CC) $(FLAGS) -o shell $(OBJECTS_Shell) 

.PHONY: clean all

clean:
	rm -f *.o *.so shell tee nc
