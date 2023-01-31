rm -f appimagetool-x86_64.AppImage
apt install libsdl2-2.0-0 libsdl2-dev libglfw3 libglfw3-dev gcc-mingw-w64-i686-win32 upx-ucl zip unzip wget dpkg
wget https://github.com/AppImage/AppImageKit/releases/download/13/appimagetool-x86_64.AppImage
chmod 0755 appimagetool-x86_64.AppImage
mkdir lib
wget https://github.com/glfw/glfw/releases/download/3.3.8/glfw-3.3.8.bin.WIN32.zip
unzip glfw-3.3.8.bin.WIN32.zip
rm -f glfw-3.3.8.bin.WIN32.zip
cp -r /home/g/Desktop/TuxPusher/glfw-3.3.8.bin.WIN32/lib-mingw-w64/* lib
rm -rf glfw-3.3.8.bin.WIN32
