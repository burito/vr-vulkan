VR-Vulkan Example
=================
This is a basic Vulkan Fragment Shader Window example, that compiles from the command line on Windows, Linux and MacOS. It's currently growing VR support out of the side.


Quick Start (all OS's)
--------------------
    git clone --recurse-submodules git@github.com:burito/vr-vulkan
    cd vr-vulkan
    make -j8                # adjust number to suit your CPU
    vulkan.exe                              # windows
    ./vulkan                                # linux
    ./vulkan.bin                            # MacOS
    ./vulkan.app/Contents/MacOS/vulkan      # MacOS with keyboard support

If you have Steam and SteamVR installed (SteamVR is listed in Steam's "Tools" menu), then press F9. If you don't have a VR headset that works with SteamVR, you can use the [null driver](https://developer.valvesoftware.com/wiki/SteamVR/steamvr.vrsettings).

Build Environment
-----------------
### Windows

* Install [mingw-w64-install.exe](http://sourceforge.net/projects/mingw-w64/files/) 8.1.0-x86_64-posix-seh
* Add its ```bin``` directory to your path
* Install current GPU drivers
	* Nvidia 417.35

### Linux
* Install current GPU drivers and compiler
	* ```add-apt-repository ppa:graphics-drivers/ppa```
	* ```apt-get update```
	* ```apt-get install nvidia-410 vulkan-utils build-essential clang imagemagick```

### MacOS

* Install XCode

Executing
---------
### Windows & MacOS
Run it however you normally would. On Mac, if you want to run the naked executable (the one without the icon), be aware that the keyboard will not work. That's a feature of MacOS.

### Linux
    ./vulkan
    # ~~or~~
    ~/.steam/steam/ubuntu12_32/steam-runtime/run.sh ./vulkan
The latter is the canonical way to run things in the Steam Environment. Or at least it was... seems to work great now without it. It is using the libopenvr_api.so in the deps/lin directory.

### All Platforms

If you run it from a terminal, there will be lots of output. It should look something like...

    16:19:05.280853461 [INFO] Platform    : XCB
    16:19:05.281110039 [INFO] Version     : 3a52e47-dirty
    16:19:05.310715619 [INFO] GPU         : GeForce GTX 1080 Ti
    16:19:05.310730006 [INFO] GPU VRAM    : 11264Mb
    16:19:05.310734518 [INFO] queuefamily[0].flags = 15
	...
And so on and so forth.

Usage
-----
 * F9 - restart window
 * F9 - toggle VR
 * F11 - toggle fullscreen

Libraries
---------
They are all in submodules now.
    git submodule init
    git submodule update --remote


Credits
=======
* ```deps/stb``` - Sean Barret
	* https://github.com/nothings/stb
	* stb_image 2.19
* ```deps/fast_atof.c``` - Tom Van Baak
	* http://www.leapsecond.com/
* ```deps/include/invert4x4_sse.h```
	* https://github.com/niswegmann/small-matrix-inverse
	* 6eac02b84ad06870692abaf828638a391548502c
* ```deps/*openvr*``` - Valve Software
	* https://github.com/ValveSoftware/openvr
	* OpenVR 1.1.3b - 64fc05966a109543a1e191a45e1ab3a25a651211
* ```deps/include/vulkan/*``` - Khronos Group
	* https://github.com/KhronosGroup/Vulkan-Headers
	* 369e6ea7f9b8cf0155b183da7e5be1b39ef6138d
* ```deps/include/MoltenVK/*``` - Khronos Group
	* https://github.com/KhronosGroup/MoltenVK/
	* 9517c58dbdf1b4c269bd700b346361a5dc01f1c0
* ```deps/win/*``` - LunarG Vulkan SDK for Windows
	* 1.1.82.1
* ```deps/lin/*``` - LunarG Vulkan SDK for Linux
	* 1.1.82.1
* ```data/stanford-bunny.obj```
    * The Stanford Bunny.
    * http://graphics.stanford.edu/data/3Dscanrep/
    * Not for commercial use.

For everything else, I am to blame.