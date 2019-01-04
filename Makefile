
BINARY_NAME = vr-vulkan
OBJS = main.o log.o global.o vr.o vr_helper.o vulkan.o vulkan_helper.o version.o 3dmaths.o text.o mesh.o image.o stb_image.o fast_atof.o
CFLAGS = -std=c11 -Ideps/include -Ibuild -Ideps/dpb/src
VPATH = src build deps deps/dpb/src

WIN_LIBS = c:/Windows/system32/vulkan-1.dll -luser32 -lwinmm -lgdi32 -lshell32
# use this line with clang/msvc
#WIN_LIBS = -ldeps/openvr/lib/win64/openvr_api.lib -ldeps/vulkan-lib/win/vulkan-1
LIN_LIBS = ./deps/vulkan-lib/lin/libvulkan.so -lX11 -lm -lrt -rpath .
MAC_LIBS = deps/vulkan-lib/mac/libMoltenVK.dylib deps/openvr/bin/osx32/libopenvr_api.dylib -framework CoreVideo -framework QuartzCore -rpath . -framework Cocoa

_WIN_OBJS = win32.o win32.res $(OBJS)
_LIN_OBJS = linux_xlib.o $(OBJS)
_MAC_OBJS = macos.o $(OBJS)

include deps/dpb/Makefile

$(BINARY_NAME).exe: $(WIN_OBJS) openvr_api.dll

$(BINARY_NAME): $(LIN_OBJS) libopenvr_api.so

$(BINARY_NAME).bin: $(MAC_OBJS) libMoltenVK.dylib libopenvr_api.dylib

libMoltenVK.dylib: deps/vulkan-lib/mac/libMoltenVK.dylib
	cp $< $@

libopenvr_api.dylib: deps/openvr/bin/osx32/libopenvr_api.dylib
	cp $< $@

openvr_api.dll: deps/openvr/bin/win64/openvr_api.dll
	cp $< $@

libopenvr_api.so: deps/openvr/bin/linux64/libopenvr_api.so
	cp $< $@


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


# this has to list everything inside the app bundle
$(MAC_CONTENTS)/_CodeSignature/CodeResources : \
	$(MAC_CONTENTS)/MacOS/$(BINARY_NAME) \
	$(MAC_CONTENTS)/Resources/AppIcon.icns \
	$(MAC_CONTENTS)/Frameworks/libMoltenVK.dylib \
	$(MAC_CONTENTS)/Frameworks/libopenvr_api.dylib \
	$(MAC_CONTENTS)/Info.plist
	codesign --force --deep --sign - $(BINARY_NAME).app

$(MAC_CONTENTS)/Frameworks/libMoltenVK.dylib: deps/vulkan-lib/mac/libMoltenVK.dylib
	@mkdir -p $(MAC_CONTENTS)/Frameworks
	cp $< $@

$(MAC_CONTENTS)/Frameworks/libopenvr_api.dylib: deps/openvr/bin/osx32/libopenvr_api.dylib
	@mkdir -p $(MAC_CONTENTS)/Frameworks
	cp $< $@

# copies the binary, and tells it where to find libraries
$(MAC_CONTENTS)/MacOS/$(BINARY_NAME): $(BINARY_NAME).bin
	@mkdir -p $(MAC_CONTENTS)/MacOS
	cp $< $@
	install_name_tool -change libMoltenVK.dylib @loader_path/../Frameworks/libMoltenVK.dylib \
	-change @loader_path/libopenvr_api.dylib @loader_path/../Frameworks/libopenvr_api.dylib $@
#	install_name_tool -change libopenvr_api.dylib @loader_path/../Frameworks/libopenvr_api.dylib $@
	install_name_tool -add_rpath "@loader_path/../Frameworks" $@
# end build the App Bundle

clean:
	@rm -rf build $(BINARY_NAME) $(BINARY_NAME).exe $(BINARY_NAME).bin $(BINARY_NAME).app libMoltenVK.dylib libopenvr_api.dylib openvr_api.dll libopenvr_api.so

# create the build directory
$(shell	mkdir -p $(BUILD_DIR))
