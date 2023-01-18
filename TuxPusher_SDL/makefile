CC ?= cc
INCLUDE_HEADERS = -I ../inc
LINK_DEPS = -lSDL2 -lGLESv2 -lEGL
CFLAGS ?= -Ofast
LDFLAGS = -lm

ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

ifneq ($(LDFLAGS),)
LINK_DEPS += $(LDFLAGS)
endif

.PHONY: all

all: tuxpusher

tuxpusher: tuxpusher.o
	$(CC) $(CFLAGS) $(LDFLAGS) main.o \
		$(INCLUDE_HEADERS) $(LINK_DEPS) \
		-o tuxpusher

tuxpusher.o: main.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c main.c \
		$(INCLUDE_HEADERS) $(LINK_DEPS) 

install:
	install -Dm 0755 tuxpusher -t $(DESTDIR)$(PREFIX)/bin

uninstall:
	rm $(DESTDIR)$(PREFIX)/bin/tuxpusher

clean:
	rm -f *.o tuxpusher
