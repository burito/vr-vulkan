# VR-Vulkan Example
This is a basic Vulkan based, C-API OpenVR example, that compiles from the command line on Windows, Linux, and MacOS. Its purpose is to serve as a template for other adventures.

## Quick Start
```bash
git clone --recurse-submodules git@github.com:burito/vr-vulkan
cd vr-vulkan
make -j8          # Build it using 8 threads
vr-vulkan.exe     # Windows
./vr-vulkan       # Linux
./vr-vulkan.bin   # MacOSX
```

If you have Steam and SteamVR installed (SteamVR is listed in Steam's "Tools" menu), then press `F9`. If you don't have a VR headset that works with SteamVR, you can use the [null driver](https://developer.valvesoftware.com/wiki/SteamVR/steamvr.vrsettings).

## Usage
 * `ESC` - quit
 * `F9` - toggle VR
 * `F11` - toggle fullscreen

On Linux, to get the full Steam environment, one should use the command
```bash
~/.steam/steam/ubuntu12_32/steam-runtime/run.sh ./vr-vulkan
```
This may not be necessary anymore.

## Build Environment
### Windows
* Install [mingw-w64-install.exe](http://sourceforge.net/projects/mingw-w64/files/) 8.1.0-x86_64-posix-seh
* Add its ```bin``` directory to your path
* Install current GPU drivers
	* Nvidia 430.39
* Install [ImageMagick](http://www.imagemagick.org/script/download.php#windows)

### Linux
* Install current GPU drivers and compiler
```bash
add-apt-repository ppa:graphics-drivers/ppa
apt update
apt install nvidia-410 vulkan-utils build-essential clang imagemagick
```

### MacOS
* Install XCode

## Libraries
They are all in submodules now.
```bash
git submodule init
git submodule update --remote
```

## Submodules / Credits
* [```deps/stb```](https://github.com/nothings/stb) - [Sean Barrett](http://nothings.org/)
* [```deps/fast_atof.c```](http://www.leapsecond.com/tools/fast_atof.c) - [Tom Van Baak](http://www.leapsecond.com/)
* [```deps/small-matrix-inverse```](https://github.com/niswegmann/small-matrix-inverse) - Nis Wegmann
* [```deps/openvr```](https://github.com/ValveSoftware/openvr) - Valve Software
* [```deps/Vulkan-Headers```](https://github.com/KhronosGroup/Vulkan-Headers) - Khronos Group
* [```deps/MoltenVK```](https://github.com/KhronosGroup/MoltenVK/) - [The Brenwill Workshop Ltd](http://brenwill.com/)
* [```deps/vulkan-lib```](https://github.com/burito/vulkan-lib) - [LunarG Vulkan SDK](https://www.lunarg.com/vulkan-sdk/)
* [```deps/models```](https://github.com/burito/models) - [Morgan McGuire's Computer Graphics Archive](https://casual-effects.com/data)

For everything else, I am to blame.