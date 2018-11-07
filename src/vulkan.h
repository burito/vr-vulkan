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

#include <vulkan/vulkan.h>

struct VR_framebuffer {
	VkImage color_image;
	VkImageLayout color_layout;
	VkDeviceMemory color_memory;
	VkImageView color_view;
	VkImage stencil_image;
	VkImageLayout stencil_layout;
	VkDeviceMemory stencil_memory;
	VkImageView stencil_view;
	VkRenderPass render_pass;
	VkFramebuffer framebuffer;
};

int vk_framebuffer(int x, int y, struct VR_framebuffer *fb);
void vk_framebuffer_free(struct VR_framebuffer *fb);



