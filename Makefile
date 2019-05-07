
COMPANY = Daniel Burke
COPYRIGHT = 2018-2019
DESCRIPTION = OpenVR Vulkan Test
BINARY_NAME = vr-vulkan
OBJS = main.o log.o global.o vr.o vr_helper.o vulkan.o vulkan_helper.o version.o 3dmaths.o text.o mesh.o image.o stb_image.o fast_atof.o
CFLAGS = -std=c11 -Ideps/include -Ideps/dpb/src
VPATH = src deps deps/dpb/src

WIN_LIBS = c:/Windows/system32/vulkan-1.dll -luser32 -lwinmm -lgdi32 -lshell32
# use this line with clang/msvc
#WIN_LIBS = -ldeps/openvr/lib/win64/openvr_api.lib -ldeps/vulkan-lib/win/vulkan-1
LIN_LIBS = ./deps/vulkan-lib/lin/libvulkan.so -lX11 -lm -lrt -rpath .
MAC_LIBS = deps/vulkan-lib/mac/libMoltenVK.dylib deps/openvr/bin/osx32/libopenvr_api.dylib -framework CoreVideo -framework QuartzCore -rpath . -framework Cocoa

_WIN_OBJS = win32.o win32.res $(OBJS)
_LIN_OBJS = linux_xlib.o $(OBJS)
_MAC_OBJS = osx.o gfx_vulkan_osx.o $(OBJS)

include deps/dpb/src/Makefile

CFLAGS += -I$(BUILD_DIR)

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


$(OUT_DIR)/vulkan.o: vulkan.c vert_spv.h frag_spv.h mesh_vert_spv.h mesh_frag_spv.h

# build the shaders - nasty hack
$(BUILD_DIR)/frag.spv : shader.frag
	$(GLSLANG) -V -H $< -o $@ > $(BUILD_DIR)/frag_spv.h.txt

$(BUILD_DIR)/vert.spv : shader.vert
	$(GLSLANG) -V -H $< -o $@ > $(BUILD_DIR)/vert_spv.h.txt

$(BUILD_DIR)/vert_spv.h : $(BUILD_DIR)/vert.spv
	(cd $(BUILD_DIR) && xxd -i vert.spv > vert_spv.h)

$(BUILD_DIR)/frag_spv.h : $(BUILD_DIR)/frag.spv
	(cd $(BUILD_DIR) && xxd -i frag.spv > frag_spv.h)

# build the mesh shaders - nasty hack continues!
$(BUILD_DIR)/mesh_frag.spv : mesh_shader.frag
	$(GLSLANG) -V -H $< -o $@ > $(BUILD_DIR)/mesh_frag_spv.h.txt

$(BUILD_DIR)/mesh_vert.spv : mesh_shader.vert
	$(GLSLANG) -V -H $< -o $@ > $(BUILD_DIR)/mesh_vert_spv.h.txt

$(BUILD_DIR)/mesh_vert_spv.h : mesh_vert.spv
	(cd $(BUILD_DIR) && xxd -i mesh_vert.spv > mesh_vert_spv.h)

$(BUILD_DIR)/mesh_frag_spv.h : mesh_frag.spv
	(cd $(BUILD_DIR) && xxd -i mesh_frag.spv > mesh_frag_spv.h)


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
