Vulkan Example
==============
This is a basic Vulkan Fragment Shader Window example, that compiles from the command line on Windows, Linux and MacOS.


Compiling (all OS's)
--------------------
    git clone git@github.com:burito/vulkan
    cd vulkan
    make


Build Environment
-----------------
### Windows

* Install [mingw-w64-install.exe](http://sourceforge.net/projects/mingw-w64/files/) 8.1.0-x86_64-posix-seh
* Add its ```bin``` directory to your path
* Install current GPU drivers

### Linux
* ```apt-get install build-essential```
* Install current GPU drivers

### MacOS

* Install XCode


Libraries
---------
This is not required to build, this is just how I procured the libraries present in this repo.

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
* ```deps/*openvr*``` - Valve Software
	* https://github.com/ValveSoftware/openvr
	* OpenVR 1.0.16 - 6aacebd1246592d9911439d5abd0c657b8948ab0
	* Fix the typo on ```openvr_capi.h:2164``` from version 1.0.11
* ```deps/include/vulkan/*``` - Khronos Group
	* https://github.com/KhronosGroup/Vulkan-Headers
	* 2fd5a24ec4a6df303b2155b3f85b6b8c1d56f6c0
* ```deps/include/MoltenVK/*``` - Khronos Group
	* https://github.com/KhronosGroup/MoltenVK/
	* 4c5f7b8b0deeb11b8f72d1af6ffef882305170c3
* ```deps/win/*``` - LunarG Vulkan SDK for Windows
	* 1.1.82.1
* ```deps/lin/*``` - LunarG Vulkan SDK for Linux
	* 1.1.82.1
* ```src/macos.m```
	* based on the work of Dmytro Ivanov
	* https://github.com/jimon/osx_app_in_plain_c

For everything else, I am to blame.