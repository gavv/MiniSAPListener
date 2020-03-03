PREFIX = /usr/local

all: saplisten

saplisten: $(patsubst %.c,%.o,$(wildcard *.c))
	gcc -ggdb -o $@ $^

%.o: %.c
	gcc -std=gnu99 -ggdb -c -o $@ $^

clean:
	rm -f *.o
	rm -f saplisten

install: saplisten
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp $< $(DESTDIR)$(PREFIX)/bin/saplisten
