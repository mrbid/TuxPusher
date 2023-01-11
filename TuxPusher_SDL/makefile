CC ?= cc
INCLUDE_HEADERS = -I ../inc
LINK_DEPS = -lSDL2 -lGLESv2 -lEGL
CFLAGS ?= -Ofast 
LDFLAGS ?=
DESTDIR ?=

ifneq ($(LDFLAGS),)
LINK_DEPS += $(LDFLAGS)
endif

.PHONY: all

all: tuxpusher

tuxpusher:
	$(CC) $(CFLAGS) $(LDFLAGS) -c main.c \
		$(INCLUDE_HEADERS) $(LINK_DEPS) \
		-o tuxpusher

install:
	install -Dm 0755 tuxpusher -t $(DESTDIR)/usr/bin

uninstall:
	rm $(DESTDIR)/tuxpusher

clean:
	rm -f tuxpusher
