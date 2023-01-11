all:
	gcc main.c -I ../inc -lSDL2 -lGLESv2 -lEGL -Ofast -lm -o tuxpusher

install:
	cp tuxpusher $(DESTDIR)

uninstall:
	rm $(DESTDIR)/tuxpusher

clean:
	rm tuxpusher