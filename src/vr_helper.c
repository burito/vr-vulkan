/*
Copyright (c) 2018 Daniel Burke

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
*/

#include <stdlib.h>
#include <openvr/openvr_capi.h>

char* vrc_error(EVRCompositorError x)
{
	switch(x) {
	case EVRCompositorError_VRCompositorError_None:
		return "EVRCompositorError_VRCompositorError_None";
	case EVRCompositorError_VRCompositorError_RequestFailed:
		return "EVRCompositorError_VRCompositorError_RequestFailed";
	case EVRCompositorError_VRCompositorError_IncompatibleVersion:
		return "EVRCompositorError_VRCompositorError_IncompatibleVersion";
	case EVRCompositorError_VRCompositorError_DoNotHaveFocus:
		return "EVRCompositorError_VRCompositorError_DoNotHaveFocus";
	case EVRCompositorError_VRCompositorError_InvalidTexture:
		return "EVRCompositorError_VRCompositorError_InvalidTexture";
	case EVRCompositorError_VRCompositorError_IsNotSceneApplication:
		return "EVRCompositorError_VRCompositorError_IsNotSceneApplication";
	case EVRCompositorError_VRCompositorError_TextureIsOnWrongDevice:
		return "EVRCompositorError_VRCompositorError_TextureIsOnWrongDevice";
	case EVRCompositorError_VRCompositorError_TextureUsesUnsupportedFormat:
		return "EVRCompositorError_VRCompositorError_TextureUsesUnsupportedFormat";
	case EVRCompositorError_VRCompositorError_SharedTexturesNotSupported:
		return "EVRCompositorError_VRCompositorError_SharedTexturesNotSupported";
	case EVRCompositorError_VRCompositorError_IndexOutOfRange:
		return "EVRCompositorError_VRCompositorError_IndexOutOfRange";
	case EVRCompositorError_VRCompositorError_AlreadySubmitted:
		return "EVRCompositorError_VRCompositorError_AlreadySubmitted";
	case EVRCompositorError_VRCompositorError_InvalidBounds:
		return "EVRCompositorError_VRCompositorError_InvalidBounds";
	default:
		return NULL;
	}
}
