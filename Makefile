
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
# MacOS
BUILD_DIR = $(MAC_DIR)
GLSLANG = deps/mac/glslangValidator
CC = clang -g
default: vulkan.app

else
# Windows & Linux need ImageMagick, lets check for it
ifeq (magick,$(findstring magick, $(shell which magick 2>&1))) # current ImageMagick looks like this
MAGICK = magick convert
else
	ifeq (convert,$(findstring convert, $(shell which convert 2>&1))) # Ubuntu ships a very old ImageMagick that looks like this
MAGICK = convert
	else
$(error Can't find ImageMagick installation, try...)
		ifeq ($(UNAME), Linux)
			$(error		apt-get install imagemagick)
		else
			$(error		https://www.imagemagick.org/script/download.php)
		endif
	endif
endif # ImageMagick check done!

ifeq ($(UNAME), Linux)
# Linux
BUILD_DIR = $(LIN_DIR)
GLSLANG = deps/lin/glslangValidator
CC = clang -g
default: vulkan

else
# Windows
BUILD_DIR = $(WIN_DIR)
GLSLANG = deps/win/glslangValidator.exe
WINDRES = windres
CC = gcc -g
default: vulkan.exe
$(shell	cp deps/win/openvr_api.dll .)
endif
endif

OBJS = main.o log.o vr.o vr_helper.o vulkan.o vulkan_helper.o version.o 3dmaths.o text.o mesh.o image.o stb_image.o fast_atof.o
CFLAGS = -std=c11 -Ideps/include -Ibuild
VPATH = src build deps

WIN_LIBS = -luser32 -lwinmm -lgdi32 -lshell32
# use this line with gcc
WIN_LIBS += c:/Windows/system32/vulkan-1.dll openvr_api.dll 
# use this line with clang/msvc
#WIN_LIBS += -Ldeps/win -lopenvr_api.lib -lvulkan-1
LIN_LIBS = ./deps/lin/libopenvr_api.so -Ldeps/lin -lvulkan -lX11 -lm -lrt
#LIN_LIBS = -Ldeps/lin -lvulkan -lxcb -lm -lopenvr_api -lrt
MAC_LIBS = -Ldeps/mac -lMoltenVK -lopenvr_api -framework CoreVideo -framework QuartzCore -rpath . -framework Cocoa

WIN_DIR = build/win
LIN_DIR = build/lin
MAC_DIR = build/mac

_WIN_OBJS = win32.o win32.res $(OBJS)
_LIN_OBJS = linux_xlib.o $(OBJS)
_MAC_OBJS = macos.o $(OBJS)

WIN_OBJS = $(patsubst %,$(WIN_DIR)/%,$(_WIN_OBJS))
LIN_OBJS = $(patsubst %,$(LIN_DIR)/%,$(_LIN_OBJS))
MAC_OBJS = $(patsubst %,$(MAC_DIR)/%,$(_MAC_OBJS))

MAC_BUNDLE = vulkan

$(WIN_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(LIN_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(MAC_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(MAC_DIR)/%.o: %.m
	$(CC) $(CFLAGS) -c $< -o $@

libMoltenVK.dylib: deps/mac/libMoltenVK.dylib
	cp $< $@

libopenvr_api.dylib: deps/mac/libopenvr_api.dylib
	cp $< $@

openvr_api.dll: deps/win/openvr_api.dll
	cp $< $@

vulkan.exe: $(WIN_OBJS)
	$(CC) $^ $(WIN_LIBS) -o $@

vulkan: $(LIN_OBJS)
	$(CC) $(CFLAGS) $^ $(LIN_LIBS) -o $@

vulkan.bin: $(MAC_OBJS) libMoltenVK.dylib libopenvr_api.dylib
	$(CC) $(CFLAGS) $(MAC_LIBS) $^ -o $@


$(BUILD_DIR)/vulkan.o: vulkan.c build/vert_spv.h build/frag_spv.h build/mesh_vert_spv.h build/mesh_frag_spv.h


# build the shaders - nasty hack
build/frag.spv : shader.frag
	$(GLSLANG) -V -H $< -o $@ > build/frag_spv.h.txt

build/vert.spv : shader.vert
	$(GLSLANG) -V -H $< -o $@ > build/vert_spv.h.txt

build/vert_spv.h : build/vert.spv
	xxd -i $< > $@ 

build/frag_spv.h : build/frag.spv
	xxd -i $< > $@

# build the mesh shaders - nasty hack continues!
build/mesh_frag.spv : mesh_shader.frag
	$(GLSLANG) -V -H $< -o $@ > build/mesh_frag_spv.h.txt

build/mesh_vert.spv : mesh_shader.vert
	$(GLSLANG) -V -H $< -o $@ > build/mesh_vert_spv.h.txt

build/mesh_vert_spv.h : build/mesh_vert.spv
	xxd -i $< > $@ 

build/mesh_frag_spv.h : build/mesh_frag.spv
	xxd -i $< > $@

# start build the win32 Resource File (contains the icon)
$(WIN_DIR)/Icon.ico: Icon.png
	$(MAGICK) -resize 256x256 $< $@
$(WIN_DIR)/win32.res: win32.rc $(WIN_DIR)/Icon.ico
	$(WINDRES) -I $(WIN_DIR) -O coff $< -o $@
# end build the win32 Resource File

# start build the App Bundle (apple)
# generate the Apple Icon file from src/Icon.png
$(MAC_DIR)/AppIcon.iconset:
	mkdir $@
$(MAC_DIR)/AppIcon.iconset/icon_512x512@2x.png: Icon.png $(MAC_DIR)/AppIcon.iconset
	cp $< $@
$(MAC_DIR)/AppIcon.iconset/icon_512x512.png: Icon.png $(MAC_DIR)/AppIcon.iconset
	sips -Z 512 $< --out $@ 1>/dev/null
$(MAC_DIR)/AppIcon.icns: $(MAC_DIR)/AppIcon.iconset/icon_512x512@2x.png $(MAC_DIR)/AppIcon.iconset/icon_512x512.png
	iconutil -c icns $(MAC_DIR)/AppIcon.iconset

MAC_CONTENTS = $(MAC_BUNDLE).app/Contents

.PHONY: $(MAC_BUNDLE).app
$(MAC_BUNDLE).app : $(MAC_CONTENTS)/_CodeSignature/CodeResources

# this has to list everything inside the app bundle
$(MAC_CONTENTS)/_CodeSignature/CodeResources : \
	$(MAC_CONTENTS)/MacOS/$(MAC_BUNDLE) \
	$(MAC_CONTENTS)/Resources/AppIcon.icns \
	$(MAC_CONTENTS)/Frameworks/libMoltenVK.dylib \
	$(MAC_CONTENTS)/Frameworks/libopenvr_api.dylib \
	$(MAC_CONTENTS)/Info.plist
	codesign --force --deep --sign - $(MAC_BUNDLE).app

$(MAC_CONTENTS)/Info.plist: src/Info.plist
	@mkdir -p $(MAC_CONTENTS)
	cp $< $@

$(MAC_CONTENTS)/Resources/AppIcon.icns: $(MAC_DIR)/AppIcon.icns
	@mkdir -p $(MAC_CONTENTS)/Resources
	cp $< $@

$(MAC_CONTENTS)/Frameworks/libMoltenVK.dylib: deps/mac/libMoltenVK.dylib
	@mkdir -p $(MAC_CONTENTS)/Frameworks
	cp $< $@

$(MAC_CONTENTS)/Frameworks/libopenvr_api.dylib: deps/mac/libopenvr_api.dylib
	@mkdir -p $(MAC_CONTENTS)/Frameworks
	cp $< $@

# copies the binary, and tells it where to find libraries
$(MAC_CONTENTS)/MacOS/$(MAC_BUNDLE): $(MAC_BUNDLE).bin
	@mkdir -p $(MAC_CONTENTS)/MacOS
	cp $< $@
	install_name_tool -change libMoltenVK.dylib @loader_path/../Frameworks/libMoltenVK.dylib \
	-change @loader_path/libopenvr_api.dylib @loader_path/../Frameworks/libopenvr_api.dylib $@
#	install_name_tool -change libopenvr_api.dylib @loader_path/../Frameworks/libopenvr_api.dylib $@
	install_name_tool -add_rpath "@loader_path/../Frameworks" $@

.DELETE_ON_ERROR :
# end build the App Bundle

clean:
	@rm -rf build vulkan vulkan.exe vulkan.bin vulkan.app libMoltenVK.dylib libopenvr_api.dylib openvr_api.dll libopenvr_api.so

# create the build directory
$(shell	mkdir -p $(BUILD_DIR))

# create the version info
GIT_VERSION:=$(shell git describe --dirty --always --tags)
VERSION:=const char git_version[] = "$(GIT_VERSION)";
SRC_VERSION:=$(shell cat build/version.c 2>/dev/null)
ifneq ($(SRC_VERSION),$(VERSION))
$(shell echo '$(VERSION)' > build/version.c)
endif
