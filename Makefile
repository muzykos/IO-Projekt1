OBJ = main.o

all: prog
prog: $(OBJ)
	gcc $(OBJ) -o prog

.PHONY: clean
clean:
	rm -f *.o prog
