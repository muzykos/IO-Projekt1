OBJ = main.o

all: dimon
dimon: $(OBJ)
	gcc $(OBJ) -o dimon -lssl -lcrypto

.PHONY: clean
clean:
	rm -f *.o dimon
