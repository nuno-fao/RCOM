HEADERS = funcs.h
OBJECTS = main.o funcs.o

default: download

%.o: %.c $(HEADERS)
	gcc -c $< -o $@

download: $(OBJECTS)
	gcc $(OBJECTS) -o $@

clean:
	-rm -f $(OBJECTS)
	-rm -f download
