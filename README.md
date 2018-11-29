VR-Vulkan Example
=================
This is a basic Vulkan Fragment Shader Window example, that compiles from the command line on Windows, Linux and MacOS. It's currently growing VR support out of the side.


Compiling (all OS's)
--------------------
    git clone git@github.com:burito/vr-vulkan
    cd vr-vulkan
    make


Build Environment
-----------------
### Windows

* Install [mingw-w64-install.exe](http://sourceforge.net/projects/mingw-w64/files/) 8.1.0-x86_64-posix-seh
* Add its ```bin``` directory to your path
* Install current GPU drivers
	* Nvidia 416.81

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
    ~/.steam/steam/ubuntu12_32/steam-runtime/run.sh ./vulkan
    # ~~or~~
    LD_LIBRARY_PATH=. ./vulkan
The latter will use the OpenVR library packaged in this repository, which should be enough for it to load, but I don't think OpenVR will function correctly, or at all.

### All Platforms

If you run it from a terminal, there will be lots of output. It should look something like...

    16:19:05.280853461 [INFO] Platform    : XCB
    16:19:05.281110039 [INFO] Version     : 3a52e47-dirty
    16:19:05.310715619 [INFO] GPU         : GeForce GTX 1080 Ti
    16:19:05.310730006 [INFO] GPU VRAM    : 11264Mb
    16:19:05.310734518 [INFO] queuefamily[0].flags = 15
	...
And so on and so forth.


Libraries
---------
This is not required to build, this is just how I procured the libraries present in this repo. I run this periodically to update the things.

    DEST=`pwd`/deps
    cd ..
    # grab the relevant repos
    git clone git@github.com:KhronosGroup/Vulkan-Headers
    git clone git@github.com:KhronosGroup/MoltenVK
    git clone git@github.com:ValveSoftware/openvr
    git clone git@github.com:nothings/stb
    git clone git@github.com:niswegmann/small-matrix-inverse
    # copy the files we want
    cp -r Vulkan-Headers/include/vulkan $DEST/include/
    cp MoltenVK/MoltenVK/MoltenVK/API/* $DEST/include/MoltenVK
    cp openvr/headers/* $DEST/include/
    cp openvr/bin/osx32/libopenvr_api.dylib $DEST/mac/
    cp openvr/bin/win64/openvr* $DEST/win/
    cp openvr/lib/win64/openvr* $DEST/win/
    cp openvr/bin/linux64/libopenvr* $DEST/lin/
    cp stb/stb_image.h $DEST/include/
    cp small-matrix-inverse/invert4x4_sse.h $DEST/include
    # perform needed modifications
    sed -i 's/static inline void invert4x4/void invert4x4/' $DEST/include/invert4x4_sse.h
    # return to where we started
    cd $DEST/..



### Windows & Linux
Grab them from the [LunarG Vulkan SDK](https://vulkan.lunarg.com/)

### MacOS
Check the MoltenVK Github Readme for up to date information, but as of now...

    git clone git@github.com:KhronosGroup/MoltenVK
    cd MoltenVK
    ./fetchDependencies
    xcodebuild -project MoltenVKPackaging.xcodeproj -scheme "MoltenVK Package (Release)" build

Then grab them from that directory.


Credits
=======
* ```deps/*stb*``` - Sean Barret
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
* ```src/macos.m```
	* based on the work of Dmytro Ivanov
	* https://github.com/jimon/osx_app_in_plain_c
* ```data/stanford-bunny.obj```
    * The Stanford Bunny.
    * http://graphics.stanford.edu/data/3Dscanrep/
    * Not for commercial use.

For everything else, I am to blame.