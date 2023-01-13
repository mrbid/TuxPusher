CC ?= cc
INCLUDE_HEADERS = -I ../inc
LINK_DEPS = -lSDL2 -lGLESv2 -lEGL
CFLAGS ?= -Ofast 

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
	cp tuxpusher $(DESTDIR)

uninstall:
	rm $(DESTDIR)/tuxpusher

clean:
	rm -f tuxpusher
