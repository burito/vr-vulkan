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


struct VULKAN_IMAGEBUFFER {
	VkImage image;
	VkDeviceMemory memory;
	VkImageView imageview;
};

struct VR_framebuffer {
	struct VULKAN_IMAGEBUFFER color;
	struct VULKAN_IMAGEBUFFER depth;
	VkRenderPass render_pass;
	VkFramebuffer framebuffer;
	VkImageLayout color_layout;
	VkImageLayout stencil_layout;
};

struct VULKAN_BUFFER {
	VkBuffer buffer;
	VkDeviceMemory memory;
	VkDeviceSize size;
};


struct VULKAN_PIPELINE {
	// for creating the object;
	size_t shader_vertex_size;
	const uint32_t* shader_vertex_code;
	size_t shader_fragment_size;
	const uint32_t* shader_fragment_code;
	VkPipelineVertexInputStateCreateInfo *vertex_input_state_crinf;
	uint32_t ubo_size;

	// things that need to be deleted on shutdown
	VkShaderModule shader_vertex;
	VkShaderModule shader_fragment;
	VkDescriptorSetLayout descriptor_set_layout;
	VkDescriptorSet descriptor_set;
	struct VULKAN_BUFFER ubo_host;
	struct VULKAN_BUFFER ubo_device;
	VkPipelineLayout pipeline_layout;
	VkPipeline pipeline;
};


struct VULKAN_HANDLES {
	VkInstance instance;
	VkSurfaceKHR surface;
	VkDevice device;
	VkPhysicalDevice physical_device;
	VkSurfaceCapabilitiesKHR surface_caps;
	uint32_t desired_queuefamily;
	VkSwapchainKHR swapchain;
	VkRenderPass renderpass;
	VkCommandPool commandpool;
	VkPipeline pipeline;
	VkPipelineLayout pipeline_layout;
	VkQueue queue;

	VkPhysicalDeviceMemoryProperties device_mem_props;
	VkPhysicalDeviceProperties device_properties;

	VkDescriptorPool descriptor_pool;

	VkPresentModeKHR present_mode;
	uint32_t display_buffer_count;
	VkSemaphore *sc_semaphore;
	VkCommandBuffer *sc_commandbuffer;
	VkImage *sc_image;
	VkImageView *sc_imageview;
	VkFramebuffer *sc_framebuffer;
	struct VULKAN_IMAGEBUFFER *sc_depth;

	int current_image;

	struct VULKAN_PIPELINE mesh;

	int finished_initialising;
};
extern struct VULKAN_HANDLES vk;

int vk_framebuffer(int x, int y, struct VR_framebuffer *fb);
void vk_framebuffer_end(struct VR_framebuffer *fb);
VkResult vk_buffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags flags, struct VULKAN_BUFFER *x, VkDeviceSize size, void *data);
void vk_buffer_end(struct VULKAN_BUFFER *x);

VkResult vk_imagebuffer(int x, int y, VkFormat format, VkImageUsageFlags usage, VkImageLayout layout, VkImageAspectFlags aspect_mask, struct VULKAN_IMAGEBUFFER *b);
void vk_imagebuffer_end(struct VULKAN_IMAGEBUFFER *x);


#endif

