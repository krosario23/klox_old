C=gcc
CFLAGS=-I
DEPS = chunk.h common.h debug.h memory.h value.h
OBJ  = main.o chunk.o debug.o memory.o value.o

%.o: %.c $(DEPS)
	$(C) -c -o $@ $< $(CFLAGS)

klox: $(OBJ)
	$(C) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f klox $(OBJ)
