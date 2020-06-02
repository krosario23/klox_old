C=gcc
CFLAGS=-I
DEPS = chunk.h common.h compiler.h debug.h memory.h scanner.h value.h vm.h
OBJ  = main.o chunk.o compiler.o debug.o memory.o scanner.o value.o vm.o

%.o: %.c $(DEPS)
	$(C) -c -o $@ $< $(CFLAGS)

klox: $(OBJ)
	$(C) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f klox $(OBJ)
