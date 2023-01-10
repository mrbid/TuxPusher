all:
	gcc main.c glad_gl.c -I inc -Ofast -lglfw -lm -o tuxpusher

install:
	cp tuxpusher $(DESTDIR)

uninstall:
	rm $(DESTDIR)/tuxpusher

clean:
	rm tuxpusher