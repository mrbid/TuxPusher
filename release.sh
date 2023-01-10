clang main.c glad_gl.c -I inc -Ofast -lglfw -lm -o tux
i686-w64-mingw32-gcc main.c glad_gl.c -Ofast -I inc -Llib -lglfw3dll -lm -o tux.exe
strip --strip-unneeded tux
strip --strip-unneeded tux.exe
upx --lzma --best tux
upx --lzma --best tux.exe