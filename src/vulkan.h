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

#define VULKAN_MAX_EXTENSIONS 10
extern uint32_t vulkan_instance_extension_count;
extern char *vulkan_instance_extension_strings[VULKAN_MAX_EXTENSIONS];

extern uint32_t vulkan_physical_device_extension_count;
extern char *vulkan_physical_device_extension_strings[VULKAN_MAX_EXTENSIONS];


#include "3dmaths.h"

struct MESH_UNIFORM_BUFFER {
	mat4x4 projection;
	mat4x4 modelview;
};

extern struct MESH_UNIFORM_BUFFER ubo_eye_left, ubo_eye_right;


struct VULKAN_IMAGEBUFFER {
	VkImage image;
	VkDeviceMemory memory;
	VkImageView imageview;
};

struct VULKAN_TEXTURE {
	VkImage image;
	VkDeviceMemory memory;
	VkImageView imageview;
	VkImageLayout image_layout;
};

struct VULKAN_FRAMEBUFFER {
	struct VULKAN_IMAGEBUFFER color;
	struct VULKAN_IMAGEBUFFER depth;
	VkFramebuffer framebuffer;
	VkImageLayout color_layout;
	VkImageLayout depth_layout;
	VkFormat format;
	int width, height;
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

struct VULKAN_VR {
	VkRenderPass renderpass;
	VkPipeline pipeline;
	VkCommandBuffer commandbuffer;
	VkDescriptorSet descriptor_set;
	struct VULKAN_BUFFER ubo_host;
	struct VULKAN_BUFFER ubo_device;
	struct VULKAN_FRAMEBUFFER fb_left;
	struct VULKAN_FRAMEBUFFER fb_right;
	int width;
	int height;
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

	VkQueue queue;
	VkSampler sampler;

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

	struct VULKAN_VR vr;
};
extern struct VULKAN_HANDLES vk;

int find_memory_type(VkMemoryRequirements requirements, VkMemoryPropertyFlags wanted);


int vk_framebuffer(int x, int y, struct VULKAN_FRAMEBUFFER *fb);
void vk_framebuffer_end(struct VULKAN_FRAMEBUFFER *fb);
VkResult vk_buffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags flags, struct VULKAN_BUFFER *x, VkDeviceSize size, void *data);
void vk_buffer_end(struct VULKAN_BUFFER *x);

VkResult vk_imagebuffer(int x, int y, VkFormat format, VkImageUsageFlags usage, VkImageLayout layout, VkImageAspectFlags aspect_mask, struct VULKAN_IMAGEBUFFER *b);
void vk_imagebuffer_end(struct VULKAN_IMAGEBUFFER *x);


void vulkan_vr_init(void);
void vulkan_vr_end(void);
void vk_commandbuffers_vr(struct VULKAN_FRAMEBUFFER *fb);



int vulkan_init(void);
int vulkan_loop(void);
void vulkan_end(void);

#endif

