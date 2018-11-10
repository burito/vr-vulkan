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

#ifndef _VULKAN_H_
#define _VULKAN_H_

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

struct VULKAN_BUFFER {
	VkBuffer buffer;
	VkDeviceMemory memory;
	VkDeviceSize size;
};

struct VULKAN_HANDLES {
	VkInstance instance;
	VkSurfaceKHR surface;
	VkDevice device;
	VkSwapchainKHR swapchain;
	VkRenderPass renderpass;
	VkCommandPool commandpool;
	VkPipeline pipeline;
	VkPipelineLayout pipeline_layout;
	VkQueue queue;
	VkDeviceMemory ubo_client_mem;
	VkDeviceMemory ubo_host_mem;
	VkBuffer ubo_client_buffer;
	VkBuffer ubo_host_buffer;

	VkPhysicalDeviceMemoryProperties device_mem_props;
	VkPhysicalDeviceProperties device_properties;

	VkDescriptorPool descriptor_pool;
	VkDescriptorSetLayout layout_ubo;
	VkShaderModule shader_module_vert;
	VkShaderModule shader_module_frag;

	uint32_t display_buffer_count;
	VkSemaphore *sc_semaphore;
	VkCommandBuffer *sc_commandbuffer;
	VkImage *sc_image;
	VkImageView *sc_imageview;
	VkFramebuffer *sc_framebuffer;

	int current_image;
};
extern struct VULKAN_HANDLES vk;

int vk_framebuffer(int x, int y, struct VR_framebuffer *fb);
void vk_framebuffer_free(struct VR_framebuffer *fb);
int vk_create_buffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags flags, struct VULKAN_BUFFER *x, VkDeviceSize size, void *data);

#endif

