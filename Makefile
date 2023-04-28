OBJ = main.o

all: prog
prog: $(OBJ)
	gcc $(OBJ) -o prog -lssl -lcrypto

.PHONY: clean
clean:
	rm -f *.o prog
