# IrrlichtVideoPlayerLib
## _An FFMPEG project that works with Irrlicht_

Based on the following GitHub projects, which were outdated and uplifted in this project:

- [IrrlichtVideoPlayerLib](https://github.com/mahmoudgalal/IrrlichtVideoPlayerLib)
- [screen-recorder-ffmpeg-cpp](https://github.com/abdullahfarwees/screen-recorder-ffmpeg-cpp)

## Features

- Play video files and display them on 3D textures.
- Screen capture and display it on 3D textures.

## Installation

Build Irrlicht however you'd like.

Build FFMPEG using the following steps (used [this roxlu article](https://www.roxlu.com/2019/062/compiling-ffmpeg-with-x264-on-windows-10-using-msvc) as a reference):

1. Install [MSYS2](https://www.msys2.org/) to c:/msys64
2. Edit c:/msys64/msys2_shell.cmd and remove rem from the line with `rem MSYS2\_PATH\_TYPE=inherit`
3. Open a x64 Native Tools Command Prompt for VS 2019
4. Run `c:/msys64/msys2_shell.cmd`
5. In the new MSYS2 shell, perform:
```
pacman -Syu
pacman -S make
pacman -S diffutils
pacman -S yasm
pacman -S nasm
 
mv /usr/bin/link.exe /usr/bin/link.exe.bak
```
6. Get sources for x264 and ffmpeg, then fix build scripts:
```
mkdir tmp
cd tmp
mkdir sources
mkdir build
cd sources
 
git clone --depth 1 https://code.videolan.org/videolan/x264.git
git clone --depth 1 git://source.ffmpeg.org/ffmpeg.git

cd tmp/sources/x264
curl "http://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.guess;hb=HEAD" > config.guess
sed -i 's/host_os = mingw/host_os = msys/' configure
```
7. Compile x264
```
cd tmp/build
mkdir x264
cd x264
 
CC=cl ./../../sources/x264/configure --prefix=./../../installed --enable-shared
make -j 8
make install
mv ./../../installed/lib/libx264.dll.lib ./../../installed/lib/libx264.lib
```
8. Compile FFMPEG (_IMPORTANT NOTE:_ This is different than the article referenced earlier):
```
cd tmp/build
mkdir ffmpeg
cd ffmpeg
 
export CC=cl
 
./../../sources/ffmpeg/configure \
             --prefix=./../../installed \
             --toolchain=msvc \
             --arch=x86_64 \
             --enable-yasm  \
             --enable-asm \
             --enable-shared \
             --disable-static \
             --disable-programs \
             --enable-libx264 \
             --enable-gpl \
             --extra-ldflags="-LIBPATH:./../../installed/lib/" \
             --extra-cflags="-I./../../installed/include/"
 
make V=1 -j 8
 
make install
```
9. To compile out of the box, make sure your project directory structure looks like this:
**SomeFolder/IrrlichtVideoPlayerLib/IrrlichtVideoPlayerLib/** (this repository)
**SomeFolder/Hello/libs/tmp/** (place ffmpeg and x264 were installed and built from the previous step)
**SomeFolder/irrlicht-code/** (place Irrlicht was downloaded -- this project uses the statically linked version, so you'll need to build Irrlicht statically)


## License

MIT
