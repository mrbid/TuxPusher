# TuxPusher
A Tux themed 3D coin pusher game.

### Screendump
![Image of the TuxPusher game](https://dashboard.snapcraft.io/site_media/appmedia/2023/01/Screenshot_2023-01-10_04-47-51.png)

### Mechanics / Rules
- Getting a gold coin in a silver slot rewards you 2x silver coins.
- Getting a gold coin in a gold slot rewards you 2x gold coins.
- Getting a tux in a slot when you already have the tux gives you 6x gold coins and 6x silver coins.

### Controls
_Just move around your mouse and click to release a coin._
- Left Click = Release coin.
- Right Click = Show/Hide cursor.
- C = Orthographic/Perspective.
- F = FPS to console.

### Play Options
- **WebGL:** https://tuxpusher.github.io
- **Snapcraft:** https://TuxPusher.com / https://snapcraft.io/tuxpusher
- **Linux Binary (GLFW):** https://github.com/mrbid/TuxPusher/releases/download/1.0/tux_glfw
- **Linux Binary (SDL2):** https://github.com/mrbid/TuxPusher/releases/download/1.0/tux_sdl
- **Windows Binary (GLFW):** https://github.com/mrbid/TuxPusher/releases/download/1.0/tux_win.zip

---
### Debian/Ubuntu Build Instructions
---
### GLFW
Install depencency libglfw3
```
apt-get install libglfw3-dev
```
Clone respository & change directory to TuxPusher
```
git clone https://github.com/mrbid/TuxPusher.git;cd TuxPusher
```
Compile
```
gcc main.c glad_gl.c -I inc -Ofast -lglfw -lm -o tuxpusher
```
or `make` or `./compile.sh`

---

### SDL2
Install depencency libsdl2
```
apt-get install libsdl2-dev
```
Clone respository & change directory to TuxPusher/TuxPusher_SDL
```
git clone https://github.com/mrbid/TuxPusher.git;cd TuxPusher/TuxPusher_SDL
```
Compile
```
gcc main.c -I ../inc -lSDL2 -lGLESv2 -lEGL -Ofast -lm -o tuxpusher
```
or `make` or `./compile.sh`

---
### Compiling for other targets
---
Install dependencies
```
apt-get install gcc-mingw-w64-i686-win32
```
Execute `./release.sh`

---

## License
This software TuxPusher is released under the [GPL-2.0-only](https://spdx.org/licenses/GPL-2.0-only.html) license. Being released under 'GPL-2.0-only' means that any subsequent versions of the GPL-2.0 license are NOT applicable to the source code or assets in this GitHub repository. This GPL-2.0-only licensing includes source code and all assets apart from the Tux 3D asset ([tux.h](assets/tux.h)) created by [Andy Cuccaro](https://sketchfab.com/andycuccaro) which is licenced under [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).
