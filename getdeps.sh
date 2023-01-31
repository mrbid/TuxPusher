rm -f appimagetool-x86_64.AppImage
apt install libsdl2-2.0-0 libsdl2-dev libglfw3 libglfw3-dev gcc-mingw-w64-i686-win32 upx-ucl zip wget dpkg
wget https://github.com/AppImage/AppImageKit/releases/download/13/appimagetool-x86_64.AppImage
chmod 0755 appimagetool-x86_64.AppImage
