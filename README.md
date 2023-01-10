# tuxpusher
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

### Snapcraft
https://snapcraft.io/tuxpusher

### Linux Binary
https://github.com/mrbid/tuxpusher/raw/main/tux

### Windows Binaries
https://github.com/mrbid/tuxpusher/raw/main/tux.exe<br>
https://github.com/mrbid/tuxpusher/raw/main/glfw3.dll

### Debian/Ubuntu Build Instructions
1. Install Depencency libglfw3
`apt-get install libglfw3-dev`
2. Clone respository
`git clone https://github.com/mrbid/TuxPusher.git;cd TuxPusher`
3. Compile
`gcc main.c glad_gl.c -I inc -Ofast -lglfw -lm -o tuxpusher`
or
`make`
or
`./compile.sh`

### License
This software TuxPusher is released under the [GPL-2.0-only](https://spdx.org/licenses/GPL-2.0-only.html) license. Being released under 'GPL-2.0-only' means that any subsequent versions of the GPL-2.0 license are NOT applicable to the source code or assets in this GitHub repository. This GPL-2.0-only licensing includes source code and all assets apart from the Tux 3D asset ([tux.h](assets/tux.h)) created by [Andy Cuccaro](https://sketchfab.com/andycuccaro) which is licenced under [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).
