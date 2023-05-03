OBJ = main.o


all: dimon kreator
dimon: $(OBJ)
	gcc $(OBJ) -o dimon -lssl -lcrypto

kreator: kreator.o
	gcc kreator.o -o kreator

.PHONY: clean
clean:
	rm -f *.o dimon
	rm -f *.o kreator
