PREFIX ?= $(DESTDIR)

.PHONY: all

all: tuxpusher

tuxpusher:
	cc main.c -I ../inc -lSDL2 -lGLESv2 -lEGL -Ofast -lm -o tuxpusher

install:
	install -Dm 0755 tuxpusher -t $(PREFIX)/bin

uninstall:
	rm $(DESTDIR)/tuxpusher

clean:
	rm -f tuxpusher
