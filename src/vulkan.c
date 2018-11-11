/*
Copyright (C) 2018 Daniel Burke

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
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
3. This notice may not be removed or altered from any source distribution.
*/

// we don't want XLIB, XCB works better
// #define USE_LINUX_XLIB

#ifdef _WIN32

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
static const uint32_t vulkan_extention_count = 2;
static const char * vulkan_extension_strings[2] = {
	VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
	VK_KHR_SURFACE_EXTENSION_NAME };
extern HINSTANCE hInst;
extern HWND hWnd;

#elif defined __APPLE__

#define VK_USE_PLATFORM_MACOS_MVK
#include <vulkan/vulkan.h>
static const uint32_t vulkan_extention_count = 2;
static const char * vulkan_extension_strings[2] = {
	VK_MVK_MACOS_SURFACE_EXTENSION_NAME,
	VK_KHR_SURFACE_EXTENSION_NAME };
extern void* pView;

#elif defined USE_LINUX_XLIB

#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan.h>
static const uint32_t vulkan_extention_count = 2;
static const char * vulkan_extension_strings[2] = {
	VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
	VK_KHR_SURFACE_EXTENSION_NAME };
extern Display* display;
extern Window window;

#else

#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.h>
static const uint32_t vulkan_extention_count = 2;
static const char * vulkan_extension_strings[2] = {
	VK_KHR_XCB_SURFACE_EXTENSION_NAME,
	VK_KHR_SURFACE_EXTENSION_NAME };
extern xcb_connection_t *xcb;
extern xcb_window_t window;

#endif

#define LOG_NO_TRACE
#include "log.h"

#include <stdlib.h>
#include <math.h>
#include "vulkan_helper.h"
#include "vulkan.h"
#include "3dmaths.h"
#include "mesh.h"

#include "vert_spv.h"
#include "frag_spv.h"

#include "mesh_vert_spv.h"
#include "mesh_frag_spv.h"




extern int vid_width;
extern int vid_height;

// required by MacOS
//VkFormat pixel_format = VK_FORMAT_B8G8R8A8_SRGB;
VkFormat pixel_format = VK_FORMAT_B8G8R8A8_UNORM;
VkColorSpaceKHR color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
//VkFormat pixel_format = VK_FORMAT_R16G16B16A16_SFLOAT;
//VkFormat pixel_format = VK_FORMAT_R64G64B64A64_UINT;

struct MESH_UNIFORM_BUFFER {
	mat4x4 projection;
	mat4x4 modelview;
	float time;
};

WF_OBJ * bunny;
struct VULKAN_HANDLES vk;

int find_memory_type(VkMemoryRequirements requirements, VkMemoryPropertyFlags wanted)
{
	for(int i=0; i<vk.device_mem_props.memoryTypeCount; i++)
	{
		if( requirements.memoryTypeBits & (1<<i) )
		if( (vk.device_mem_props.memoryTypes[i].propertyFlags & wanted) == wanted )
		{
			// found the memory number we wanted
			return i;
		}
	}
	log_warning("could not find the desired memory type");
	return 0;
}

VkResult vk_buffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags flags, struct VULKAN_BUFFER *x, VkDeviceSize size, void *data)
{
	VkResult result;
	VkBufferCreateInfo buffer_crinf = {
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,	// VkStructureType        sType;
		NULL,					// const void*            pNext;
		0,					// VkBufferCreateFlags    flags;
		size,					// VkDeviceSize           size;
		usage,					// VkBufferUsageFlags     usage;
		VK_SHARING_MODE_EXCLUSIVE,		// VkSharingMode          sharingMode;
		0,					// uint32_t               queueFamilyIndexCount;
		NULL					// const uint32_t*        pQueueFamilyIndices;
	};
	result = vkCreateBuffer(vk.device, &buffer_crinf, NULL, &x->buffer);
	if( result != VK_SUCCESS )
	{
		log_fatal("vkCreateBuffer = %s", vulkan_result(result));
		goto VK_CB_BUFFER;
	}
	
	VkMemoryRequirements requirements;
	vkGetBufferMemoryRequirements(vk.device, x->buffer, &requirements);
	uint32_t memory_type = find_memory_type(requirements, flags);

	x->size = requirements.size;

	VkMemoryAllocateInfo buffer_alloc_info = {
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,	// VkStructureType    sType;
		NULL,					// const void*        pNext;
		x->size,				// VkDeviceSize       allocationSize;
		memory_type				// uint32_t           memoryTypeIndex;
	};
	result = vkAllocateMemory(vk.device, &buffer_alloc_info, NULL, &x->memory);
	if( result != VK_SUCCESS )
	{
		log_fatal("vkAllocateMemory = %s", vulkan_result(result));
		goto VK_CB_ALLOCATE;
	}

	if(data != NULL)
	{
		void *mapped;
		result = vkMapMemory(vk.device, x->memory, 0, size, 0, &mapped);
		if( result != VK_SUCCESS )
		{
			log_fatal("vkMapMemory = %s", vulkan_result(result));
			goto VK_CB_MAP;
		}
		memcpy(mapped, data, size);
		vkUnmapMemory(vk.device, x->memory);
	}

	result = vkBindBufferMemory(vk.device, x->buffer, x->memory, 0);
	if( result != VK_SUCCESS )
	{
		log_fatal("vkBindBufferMemory = %s", vulkan_result(result));
		goto VK_CB_BIND;
	}

	return VK_SUCCESS;
VK_CB_BIND:
VK_CB_MAP:
	vkFreeMemory(vk.device, x->memory, NULL);
VK_CB_ALLOCATE:
	vkDestroyBuffer(vk.device, x->buffer, NULL);
VK_CB_BUFFER:
	return result;
}

void vk_buffer_end(struct VULKAN_BUFFER *x)
{
	vkFreeMemory(vk.device, x->memory, NULL);
	vkDestroyBuffer(vk.device, x->buffer, NULL);
}


int vk_malloc_swapchain(struct VULKAN_HANDLES *vk)
{
	vk->sc_image = malloc(sizeof(VkImage) * vk->display_buffer_count);
	if( vk->sc_image == NULL )
	{
		log_fatal("malloc(vk->sc_image)");
		goto VK_MALLOC_IMAGE;
	}
	vk->sc_imageview = malloc(sizeof(VkImageView) * vk->display_buffer_count);
	if( vk->sc_imageview == NULL )
	{
		log_fatal("malloc(vk->sc_imageview)");
		goto VK_MALLOC_IMAGEVIEW;
	}
	vk->sc_framebuffer = malloc(sizeof(VkFramebuffer) * vk->display_buffer_count);
	if( vk->sc_framebuffer == NULL )
	{
		log_fatal("malloc(vk->sc_framebuffer)");
		goto VK_MALLOC_FRAMEBUFFER;
	}
	vk->sc_semaphore = malloc(sizeof(VkSemaphore) * vk->display_buffer_count);
	if( vk->sc_semaphore == NULL )
	{
		log_fatal("malloc(vk->sc_semaphore)");
		goto VK_MALLOC_SEMAPHORE;
	}
	vk->sc_commandbuffer = malloc(sizeof(VkCommandBuffer) * vk->display_buffer_count);
	if( vk->sc_commandbuffer == NULL )
	{
		log_fatal("malloc(vk->sc_commandbuffer)");
		goto VK_MALLOC_COMMANDBUFFER;
	}
	vk->sc_depth = malloc(sizeof(struct VULKAN_IMAGEBUFFER) * vk->display_buffer_count);
	if( vk->sc_depth == NULL )
	{
		log_fatal("malloc(vk->sc_depth)");
		goto VK_MALLOC_DEPTH;
	}
	return 0;
VK_MALLOC_DEPTH:
	free(vk->sc_commandbuffer);
VK_MALLOC_COMMANDBUFFER:
	free(vk->sc_semaphore);
VK_MALLOC_SEMAPHORE:
	free(vk->sc_framebuffer);
VK_MALLOC_FRAMEBUFFER:
	free(vk->sc_imageview);
VK_MALLOC_IMAGEVIEW:
	free(vk->sc_image);
VK_MALLOC_IMAGE:
	return 1;
}

VkResult vk_imagebuffer(int x, int y, VkFormat format, VkImageUsageFlags usage, VkImageLayout layout, VkImageAspectFlags aspect_mask, struct VULKAN_IMAGEBUFFER *b)
{
	VkResult result;
	VkImageCreateInfo image_crinf = {
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,	// VkStructureType          sType;
		NULL,					// const void*              pNext;
		0,					// VkImageCreateFlags       flags;
		VK_IMAGE_TYPE_2D,			// VkImageType              imageType;
		format,					// VkFormat                 format;
		{ x, y, 1},				// VkExtent3D               extent;
		1,					// uint32_t                 mipLevels;
		1,					// uint32_t                 arrayLayers;
		VK_SAMPLE_COUNT_1_BIT,			// VkSampleCountFlagBits    samples;
		VK_IMAGE_TILING_OPTIMAL,		// VkImageTiling            tiling;
		usage,					// VkImageUsageFlags        usage;
		VK_SHARING_MODE_EXCLUSIVE,		// VkSharingMode            sharingMode;
		0,					// uint32_t                 queueFamilyIndexCount;
		NULL,					// const uint32_t*          pQueueFamilyIndices;
		layout					// VkImageLayout            initialLayout;
	};

	result = vkCreateImage(vk.device, &image_crinf, NULL, &b->image);
	if( result != VK_SUCCESS )
	{
		log_warning("vkCreateImage = %s", vulkan_result(result));
		goto VK_IB_IMAGE;
	}
	VkMemoryRequirements requirements;
	vkGetImageMemoryRequirements(vk.device, b->image, &requirements);
	uint32_t memory_type = find_memory_type(requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkMemoryAllocateInfo alloc_info = {
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,	// VkStructureType    sType;
		NULL,					// const void*        pNext;
		requirements.size,			// VkDeviceSize       allocationSize;
		memory_type				// uint32_t           memoryTypeIndex;
	};
	result = vkAllocateMemory(vk.device, &alloc_info, NULL, &b->memory);
	if( result != VK_SUCCESS )
	{
		log_warning("vkAllocateMemory = %s", vulkan_result(result));
		goto VK_IB_ALLOC;
	}
	result = vkBindImageMemory(vk.device, b->image, b->memory, 0);
	if( result != VK_SUCCESS )
	{
		log_warning("vkBindImageMemory = %s", vulkan_result(result));
		goto VK_IB_BIND;
	}

	VkImageViewCreateInfo image_view_crinf = {
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,	// VkStructureType            sType;
		NULL,						// const void*                pNext;
		0,						// VkImageViewCreateFlags     flags;
		b->image,					// VkImage                    image;
		VK_IMAGE_VIEW_TYPE_2D,				// VkImageViewType            viewType;
		format,						// VkFormat                   format;
		{	VK_COMPONENT_SWIZZLE_IDENTITY,		// VkComponentMapping         components;
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY		// VkImageSubresourceRange    subresourceRange;
		},{ 	aspect_mask,			// VkImageAspectFlags    aspectMask;
			0,				// uint32_t              baseMipLevel;
			1,				// uint32_t              levelCount;
			0,				// uint32_t              baseArrayLayer;
			1				// uint32_t              layerCount;
		}
	};

	VkImageView imageview;
	result = vkCreateImageView(vk.device, &image_view_crinf, NULL, &b->imageview);
	if( result != VK_SUCCESS )
	{
		log_warning("vkCreateImageView = %s", vulkan_result(result));
		goto VK_IB_IMAGEVIEW;
	}
	return VK_SUCCESS;

	vkDestroyImageView(vk.device, b->imageview, NULL);
VK_IB_IMAGEVIEW:
VK_IB_BIND:
	vkFreeMemory(vk.device, b->memory, NULL);
VK_IB_ALLOC:
	vkDestroyImage(vk.device, b->image, NULL);
VK_IB_IMAGE:
	return result;
}

void vk_imagebuffer_end(struct VULKAN_IMAGEBUFFER *x)
{
	vkDestroyImageView(vk.device, x->imageview, NULL);
	vkFreeMemory(vk.device, x->memory, NULL);
	vkDestroyImage(vk.device, x->image, NULL);
}

int vk_framebuffer(int x, int y, struct VR_framebuffer *fb)
{
	VkResult result;

	result = vk_imagebuffer(x, y,
		pixel_format,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_ASPECT_COLOR_BIT,
		&fb->color
	);
	if( result != VK_SUCCESS )
	{
		log_warning("vk_imagebuffer = %s", vulkan_result(result));
		goto VK_FB_COLORBUFFER;
	}

	result = vk_imagebuffer(x, y,
		VK_FORMAT_D32_SFLOAT,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		&fb->depth
	);
	if( result != VK_SUCCESS )
	{
		log_warning("vk_imagebuffer = %s", vulkan_result(result));
		goto VK_FB_DEPTHBUFFER;
	}

	// create renderpass
	VkAttachmentDescription attach_desc[] = {{
			0,						// VkAttachmentDescriptionFlags    flags;
			pixel_format,					// VkFormat                        format;
			VK_SAMPLE_COUNT_1_BIT,				// VkSampleCountFlagBits           samples;
			VK_ATTACHMENT_LOAD_OP_CLEAR,			// VkAttachmentLoadOp              loadOp;
			VK_ATTACHMENT_STORE_OP_STORE,			// VkAttachmentStoreOp             storeOp;
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,		// VkAttachmentLoadOp              stencilLoadOp;
			VK_ATTACHMENT_STORE_OP_DONT_CARE,		// VkAttachmentStoreOp             stencilStoreOp;
			VK_IMAGE_LAYOUT_UNDEFINED,	// VkImageLayout                   initialLayout;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL	// VkImageLayout                   finalLayout;
		},{
			0,							// VkAttachmentDescriptionFlags    flags;
			VK_FORMAT_D32_SFLOAT,					// VkFormat                        format;
			VK_SAMPLE_COUNT_1_BIT,					// VkSampleCountFlagBits           samples;
			VK_ATTACHMENT_LOAD_OP_CLEAR,				// VkAttachmentLoadOp              loadOp;
			VK_ATTACHMENT_STORE_OP_STORE,				// VkAttachmentStoreOp             storeOp;
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,			// VkAttachmentLoadOp              stencilLoadOp;
			VK_ATTACHMENT_STORE_OP_DONT_CARE,			// VkAttachmentStoreOp             stencilStoreOp;
			VK_IMAGE_LAYOUT_UNDEFINED,	// VkImageLayout                   initialLayout;
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL	// VkImageLayout                   finalLayout;
		}
	};

	VkAttachmentReference attach_refs[] = {{
			0,						// uint32_t         attachment;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL	// VkImageLayout    layout;
		}, {
			1,						// uint32_t         attachment;
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL// VkImageLayout    layout;
		}
	};

	VkSubpassDescription subpass_desc[] = {{
		0,					// VkSubpassDescriptionFlags       flags;
		VK_PIPELINE_BIND_POINT_GRAPHICS,	// VkPipelineBindPoint             pipelineBindPoint;
		0,					// uint32_t                        inputAttachmentCount;
		NULL,					// const VkAttachmentReference*    pInputAttachments;
		1,					// uint32_t                        colorAttachmentCount;
		&attach_refs[0],			// const VkAttachmentReference*    pColorAttachments;
		NULL,					// const VkAttachmentReference*    pResolveAttachments;
		&attach_refs[1],			// const VkAttachmentReference*    pDepthStencilAttachment;
		0,					// uint32_t                        preserveAttachmentCount;
		NULL					// const uint32_t*                 pPreserveAttachments;
	}};

	VkRenderPassCreateInfo render_pass_crinf = {
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,	// VkStructureType                   sType;
		NULL,						// const void*                       pNext;
		0,						// VkRenderPassCreateFlags           flags;
		2,						// uint32_t                          attachmentCount;
		&attach_desc[0],				// const VkAttachmentDescription*    pAttachments;
		1,						// uint32_t                          subpassCount;
		subpass_desc,					// const VkSubpassDescription*       pSubpasses;
		0,						// uint32_t                          dependencyCount;
		NULL						// const VkSubpassDependency*        pDependencies;
	};

	result = vkCreateRenderPass(vk.device, &render_pass_crinf, NULL, &fb->render_pass );
	if( result != VK_SUCCESS )
	{
		log_warning("vkCreateRenderPass = %s", vulkan_result(result));
		goto VK_FB_RENDERPASS;
	}

	VkImageView attachments[2] = { fb->color.imageview, fb->depth.imageview };
	VkFramebufferCreateInfo fb_crinf = {
		VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,	// VkStructureType             sType;
		NULL,						// const void*                 pNext;
		0,						// VkFramebufferCreateFlags    flags;
		fb->render_pass,				// VkRenderPass                renderPass;
		2,						// uint32_t                    attachmentCount;
		&attachments[0],				// const VkImageView*          pAttachments;
		x,						// uint32_t                    width;
		y,						// uint32_t                    height;
		1						// uint32_t                    layers;
	};

	result = vkCreateFramebuffer(vk.device, &fb_crinf, NULL, &fb->framebuffer);
	if( result != VK_SUCCESS )
	{
		log_warning("vkCreateFramebuffer = %s", vulkan_result(result));
		goto VK_FB_FRAMEBUFFER;
	}

	fb->color_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	fb->stencil_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	return 0;

	vkDestroyFramebuffer(vk.device, fb->framebuffer, NULL);
VK_FB_FRAMEBUFFER:
	vkDestroyRenderPass(vk.device, fb->render_pass, NULL);
VK_FB_RENDERPASS:
	vk_imagebuffer_end(&fb->depth);
VK_FB_DEPTHBUFFER:
	vk_imagebuffer_end(&fb->color);
VK_FB_COLORBUFFER:	
	return result;
}

void vk_framebuffer_end(struct VR_framebuffer *fb)
{
	vkDestroyFramebuffer(vk.device, fb->framebuffer, NULL);
	vkDestroyRenderPass(vk.device, fb->render_pass, NULL);

	vk_imagebuffer_end(&fb->depth);
	vk_imagebuffer_end(&fb->color);
}


void vk_mesh_pipeline(void)
{
	VkResult result;
	// create the shaders

	VkShaderModuleCreateInfo shader_vert_crinf = {
		VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,	// VkStructureType              sType;
		NULL,						// const void*                  pNext;
		0,						// VkShaderModuleCreateFlags    flags;
		build_mesh_vert_spv_len,			// size_t                       codeSize;
		(const uint32_t*)build_mesh_vert_spv		// const uint32_t*              pCode;
	};

	VkShaderModuleCreateInfo shader_frag_crinf = {
		VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,	// VkStructureType              sType;
		NULL,						// const void*                  pNext;
		0,						// VkShaderModuleCreateFlags    flags;
		build_mesh_frag_spv_len,			// size_t                       codeSize;
		(const uint32_t*)build_mesh_frag_spv		// const uint32_t*              pCode;
	};

	result = vkCreateShaderModule(vk.device, &shader_vert_crinf, NULL, &vk.mesh_shader_vert);
	if( result != VK_SUCCESS )
	{
		log_fatal("vkCreateShaderModule(vertex) = %s", vulkan_result(result));
//		goto VK_INIT_CREATE_SHADER_VERT;
	}
	result = vkCreateShaderModule(vk.device, &shader_frag_crinf, NULL, &vk.mesh_shader_frag);
	if( result != VK_SUCCESS )
	{
		log_fatal("vkCreateShaderModule(fragment) = %s", vulkan_result(result));
//		goto VK_INIT_CREATE_SHADER_FRAG;
	}

	int ubo_buffer_size = sizeof(struct MESH_UNIFORM_BUFFER);

	vk_buffer( VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&vk.mesh_ubo_host, ubo_buffer_size, NULL );

	vk_buffer( VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&vk.mesh_ubo_device, vk.mesh_ubo_host.size, NULL );

	VkDescriptorSetLayoutBinding descriptor_layout_binding = {
		0,					// uint32_t              binding;
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,	// VkDescriptorType      descriptorType;
		1,					// uint32_t              descriptorCount;
		VK_SHADER_STAGE_VERTEX_BIT |		// VkShaderStageFlags    stageFlags;
		VK_SHADER_STAGE_FRAGMENT_BIT,
		NULL					// const VkSampler*      pImmutableSamplers;
	};

	VkDescriptorSetLayoutCreateInfo descriptor_layout_crinf = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,	// VkStructureType                        sType;
		NULL,							// const void*                            pNext;
		0,							// VkDescriptorSetLayoutCreateFlags       flags;
		1,							// uint32_t                               bindingCount;
		&descriptor_layout_binding				// const VkDescriptorSetLayoutBinding*    pBindings;
	};

	result = vkCreateDescriptorSetLayout(vk.device, &descriptor_layout_crinf, NULL, &vk.mesh_ubo_layout);
	if( result != VK_SUCCESS )
	{
		log_fatal("vkCreateDescriptorSetLayout = %s", vulkan_result(result));
//		goto VK_INIT_DESCRIPTOR_SET_LAYOUT;
	}

	VkDescriptorSetAllocateInfo desc_set_alloc_info = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,	// VkStructureType                 sType;
		NULL,						// const void*                     pNext;
		vk.descriptor_pool,				// VkDescriptorPool                descriptorPool;
		1,						// uint32_t                        descriptorSetCount;
		&vk.mesh_ubo_layout				// const VkDescriptorSetLayout*    pSetLayouts;
	};

	result = vkAllocateDescriptorSets(vk.device, &desc_set_alloc_info, &vk.mesh_descriptor_set);
	if( result != VK_SUCCESS )
	{
		log_warning("vkAllocateDescriptorSets = %s", vulkan_result(result));
	}

	VkDescriptorBufferInfo desc_buf_info = {
		vk.mesh_ubo_host.buffer,// VkBuffer        buffer;
		0,			// VkDeviceSize    offset;
		sizeof(struct MESH_UNIFORM_BUFFER)	// VkDeviceSize    range;
	};
	VkWriteDescriptorSet desc_write_set = {
		VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,	// VkStructureType                  sType;
		NULL,					// const void*                      pNext;
		vk.mesh_descriptor_set,			// VkDescriptorSet                  dstSet;
		0,					// uint32_t                         dstBinding;
		0,					// uint32_t                         dstArrayElement;
		1,					// uint32_t                         descriptorCount;
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,	// VkDescriptorType                 descriptorType;
		NULL,					// const VkDescriptorImageInfo*     pImageInfo;
		&desc_buf_info,				// const VkDescriptorBufferInfo*    pBufferInfo;
		NULL					// const VkBufferView*              pTexelBufferView;
	};
	vkUpdateDescriptorSets(vk.device, 1, &desc_write_set, 0, NULL);
//	log_debug("vkUpdateDescriptorSets");

// create the pipeline
	// shader and vertex input
	VkPipelineShaderStageCreateInfo shader_stage_crinf[2] = {
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,	// VkStructureType                     sType;
			NULL,							// const void*                         pNext;
			0,							// VkPipelineShaderStageCreateFlags    flags;
			VK_SHADER_STAGE_VERTEX_BIT,				// VkShaderStageFlagBits               stage;
			vk.mesh_shader_vert,					// VkShaderModule                      module;
			"main",							// const char*                         pName;
			NULL							// const VkSpecializationInfo*         pSpecializationInfo;
		},
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,	// VkStructureType                     sType;
			NULL,							// const void*                         pNext;
			0,							// VkPipelineShaderStageCreateFlags    flags;
			VK_SHADER_STAGE_FRAGMENT_BIT,				// VkShaderStageFlagBits               stage;
			vk.mesh_shader_frag,					// VkShaderModule                      module;
			"main",							// const char*                         pName;
			NULL							// const VkSpecializationInfo*         pSpecializationInfo;
		}
	};

	VkVertexInputBindingDescription vertex_binding_description = {
		0,				// uint32_t             binding;
		32,				// uint32_t             stride;
		VK_VERTEX_INPUT_RATE_VERTEX	// VkVertexInputRate    inputRate;
	};


	VkVertexInputAttributeDescription attribute_binding_description[] = {{
			0,				// uint32_t    location;
			0,				// uint32_t    binding;
			VK_FORMAT_R32G32B32_SFLOAT,	// VkFormat    format;
			0				// uint32_t    offset;
		},{
			1,				// uint32_t    location;
			0,				// uint32_t    binding;
			VK_FORMAT_R32G32B32_SFLOAT,	// VkFormat    format;
			sizeof(float)*3			// uint32_t    offset;
		},{
			2,				// uint32_t    location;
			0,				// uint32_t    binding;
			VK_FORMAT_R32G32_SFLOAT,	// VkFormat    format;
			sizeof(float)*6			// uint32_t    offset;
		}
	};


	VkPipelineVertexInputStateCreateInfo vertex_input_state_crinf = {
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,	// VkStructureType                             sType;
		NULL,								// const void*                                 pNext;
		0,								// VkPipelineVertexInputStateCreateFlags       flags;
		1,								// uint32_t                                    vertexBindingDescriptionCount;
		&vertex_binding_description,					// const VkVertexInputBindingDescription*      pVertexBindingDescriptions;
		3,								// uint32_t                                    vertexAttributeDescriptionCount;
		attribute_binding_description					// const VkVertexInputAttributeDescription*    pVertexAttributeDescriptions;
	};

	// input assembly, viewport
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state_crinf = {
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,	// VkStructureType                            sType;
		NULL,								// const void*                                pNext;
		0,								// VkPipelineInputAssemblyStateCreateFlags    flags;
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,				// VkPrimitiveTopology                        topology;
		VK_FALSE							// VkBool32                                   primitiveRestartEnable;		
	};

	VkViewport viewport = {0.0f,0.0f,(float)vid_width,(float)vid_height,0.0f,1.0f};
	VkRect2D scissor = {{0,0},{vid_width,vid_height}};
	VkPipelineViewportStateCreateInfo viewport_state_crinf = {
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,	// VkStructureType                       sType;
		NULL,							// const void*                           pNext;
		0,							// VkPipelineViewportStateCreateFlags    flags;
		1,							// uint32_t                              viewportCount;
		&viewport,						// const VkViewport*                     pViewports;
		1,							// uint32_t                              scissorCount;
		&scissor						// const VkRect2D*                       pScissors;
	};

	// Rasterisation, multisampling
	VkPipelineRasterizationStateCreateInfo rasterization_state_crinf = {
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,	// VkStructureType                            sType;
		NULL,								// const void*                                pNext;
		0,								// VkPipelineRasterizationStateCreateFlags    flags;
		VK_FALSE,							// VkBool32                                   depthClampEnable;
		VK_FALSE,							// VkBool32                                   rasterizerDiscardEnable;
		VK_POLYGON_MODE_FILL,						// VkPolygonMode                              polygonMode;
		VK_CULL_MODE_BACK_BIT,						// VkCullModeFlags                            cullMode;
		VK_FRONT_FACE_COUNTER_CLOCKWISE,				// VkFrontFace                                frontFace;
		VK_FALSE,							// VkBool32                                   depthBiasEnable;
		0.0f,								// float                                      depthBiasConstantFactor;
		0.0f,								// float                                      depthBiasClamp;
		0.0f,								// float                                      depthBiasSlopeFactor;
		1.0f								// float                                      lineWidth;		
	};

	VkSampleMask sample_mask = 0xFFFFFFFF;
	VkPipelineMultisampleStateCreateInfo multisample_state_crinf = {
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,	// VkStructureType                          sType;
		NULL,								// const void*                              pNext;
		0,								// VkPipelineMultisampleStateCreateFlags    flags;
		VK_SAMPLE_COUNT_1_BIT,						// VkSampleCountFlagBits                    rasterizationSamples;
		VK_FALSE,							// VkBool32                                 sampleShadingEnable;
		0.0f,								// float                                    minSampleShading;
		&sample_mask,							// const VkSampleMask*                      pSampleMask;
		VK_FALSE,							// VkBool32                                 alphaToCoverageEnable;
		VK_FALSE							// VkBool32                                 alphaToOneEnable;
	};


	VkPipelineDepthStencilStateCreateInfo depth_stencil_state_crinf = {
		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,	// VkStructureType                           sType;
		NULL,								// const void*                               pNext;
		0,								// VkPipelineDepthStencilStateCreateFlags    flags;
		VK_TRUE,							// VkBool32                                  depthTestEnable;
		VK_TRUE,							// VkBool32                                  depthWriteEnable;
		VK_COMPARE_OP_LESS_OR_EQUAL,					// VkCompareOp                               depthCompareOp;
		VK_FALSE,							// VkBool32                                  depthBoundsTestEnable;
		VK_FALSE,							// VkBool32                                  stencilTestEnable;
		{0,0,0,VK_COMPARE_OP_ALWAYS,0,0,0				// VkStencilOpState                          front;
							// VkStencilOp    failOp;
							// VkStencilOp    passOp;
							// VkStencilOp    depthFailOp;
							// VkCompareOp    compareOp;
							// uint32_t       compareMask;
							// uint32_t       writeMask;
							// uint32_t       reference;
		}, {0,0,0,VK_COMPARE_OP_ALWAYS,0,0,0				// VkStencilOpState                          back;
		},
		0.0f,								// float                                     minDepthBounds;
		0.0f								// float                                     maxDepthBounds;
	};

	// colour blending
	VkPipelineColorBlendAttachmentState color_blend_attachment_state = {
		VK_FALSE,			// VkBool32                 blendEnable;
		VK_BLEND_FACTOR_ONE,		// VkBlendFactor            srcColorBlendFactor;
		VK_BLEND_FACTOR_ZERO,		// VkBlendFactor            dstColorBlendFactor;
		VK_BLEND_OP_ADD,		// VkBlendOp                colorBlendOp;
		VK_BLEND_FACTOR_ONE,		// VkBlendFactor            srcAlphaBlendFactor;
		VK_BLEND_FACTOR_ZERO,		// VkBlendFactor            dstAlphaBlendFactor;
		VK_BLEND_OP_ADD,		// VkBlendOp                alphaBlendOp;
		VK_COLOR_COMPONENT_R_BIT |	// VkColorComponentFlags    colorWriteMask;
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT
	};

	VkPipelineColorBlendStateCreateInfo color_blend_state_crinf = {
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,	// VkStructureType                               sType;
		NULL,								// const void*                                   pNext;
		0,								// VkPipelineColorBlendStateCreateFlags          flags;
		VK_FALSE,							// VkBool32                                      logicOpEnable;
		VK_LOGIC_OP_COPY,						// VkLogicOp                                     logicOp;
		1,								// uint32_t                                      attachmentCount;
		&color_blend_attachment_state,					// const VkPipelineColorBlendAttachmentState*    pAttachments;
		{0.0f,0.0f,0.0f,0.0f}						// float                                         blendConstants[4];
	};

	// Pipeline creation
	VkPipelineLayoutCreateInfo layout_crinf = {
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,	// VkStructureType                 sType;
		NULL,						// const void*                     pNext;
		0,						// VkPipelineLayoutCreateFlags     flags;
		1,						// uint32_t                        setLayoutCount;
		&vk.mesh_ubo_layout,				// const VkDescriptorSetLayout*    pSetLayouts;
		0,						// uint32_t                        pushConstantRangeCount;
		NULL						// const VkPushConstantRange*      pPushConstantRanges;
	};
	result = vkCreatePipelineLayout(vk.device, &layout_crinf, NULL, &vk.mesh_pipeline_layout);
	if( result != VK_SUCCESS )
	{
		log_fatal("vkCreatePipelineLayout = %s", vulkan_result(result));
//		goto VK_INIT_PIPELINE_LAYOUT;
	}

	VkGraphicsPipelineCreateInfo pipeline_crinf = {
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,	// VkStructureType                                  sType;
		NULL,							// const void*                                      pNext;
		0,							// VkPipelineCreateFlags                            flags;
		2,							// uint32_t                                         stageCount;
		&shader_stage_crinf[0],					// const VkPipelineShaderStageCreateInfo*           pStages;
		&vertex_input_state_crinf,				// const VkPipelineVertexInputStateCreateInfo*      pVertexInputState;
		&input_assembly_state_crinf,				// const VkPipelineInputAssemblyStateCreateInfo*    pInputAssemblyState;
		NULL,							// const VkPipelineTessellationStateCreateInfo*     pTessellationState;
		&viewport_state_crinf,					// const VkPipelineViewportStateCreateInfo*         pViewportState;
		&rasterization_state_crinf,				// const VkPipelineRasterizationStateCreateInfo*    pRasterizationState;
		&multisample_state_crinf,				// const VkPipelineMultisampleStateCreateInfo*      pMultisampleState;
		&depth_stencil_state_crinf,				// const VkPipelineDepthStencilStateCreateInfo*     pDepthStencilState;
		&color_blend_state_crinf,				// const VkPipelineColorBlendStateCreateInfo*       pColorBlendState;
		NULL,							// const VkPipelineDynamicStateCreateInfo*          pDynamicState;
		vk.mesh_pipeline_layout,				// VkPipelineLayout                                 layout;
		vk.renderpass,					// VkRenderPass                                     renderPass;
		0,							// uint32_t                                         subpass;
		VK_NULL_HANDLE,						// VkPipeline                                       basePipelineHandle;
		-1							// int32_t                                          basePipelineIndex;
	};

	result = vkCreateGraphicsPipelines(vk.device, VK_NULL_HANDLE, 1, &pipeline_crinf, NULL, &vk.mesh_pipeline);
	if( result != VK_SUCCESS )
	{
		log_fatal("vkCreateGraphicsPipelines = %s", vulkan_result(result));
//		goto VK_INIT_PIPELINE;
	}


	VkAttachmentReference attachment_reference[] = {
		{
			0,						// uint32_t         attachment;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL	// VkImageLayout    layout;
		}, {
			1,							// uint32_t         attachment;
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL	// VkImageLayout    layout;
		}
	};

	VkAttachmentDescription attachment_description[] = {
		{
			0,						// VkAttachmentDescriptionFlags    flags;
			pixel_format,					// VkFormat                        format;
			VK_SAMPLE_COUNT_1_BIT,				// VkSampleCountFlagBits           samples;
			VK_ATTACHMENT_LOAD_OP_CLEAR,			// VkAttachmentLoadOp              loadOp;
			VK_ATTACHMENT_STORE_OP_STORE,			// VkAttachmentStoreOp             storeOp;
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,		// VkAttachmentLoadOp              stencilLoadOp;
			VK_ATTACHMENT_STORE_OP_DONT_CARE,		// VkAttachmentStoreOp             stencilStoreOp;
			VK_IMAGE_LAYOUT_UNDEFINED,	// VkImageLayout                   initialLayout;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL	// VkImageLayout                   finalLayout;
		},{
			0,						// VkAttachmentDescriptionFlags    flags;
			VK_FORMAT_D32_SFLOAT,					// VkFormat                        format;
			VK_SAMPLE_COUNT_1_BIT,					// VkSampleCountFlagBits           samples;
			VK_ATTACHMENT_LOAD_OP_CLEAR,				// VkAttachmentLoadOp              loadOp;
			VK_ATTACHMENT_STORE_OP_STORE,				// VkAttachmentStoreOp             storeOp;
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,			// VkAttachmentLoadOp              stencilLoadOp;
			VK_ATTACHMENT_STORE_OP_DONT_CARE,			// VkAttachmentStoreOp             stencilStoreOp;
			VK_IMAGE_LAYOUT_UNDEFINED,	// VkImageLayout                   initialLayout;
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL	// VkImageLayout                   finalLayout;
		}
	};

	VkSubpassDescription subpass_description = {
		0,					// VkSubpassDescriptionFlags       flags;
		VK_PIPELINE_BIND_POINT_GRAPHICS,	// VkPipelineBindPoint             pipelineBindPoint;
		0,					// uint32_t                        inputAttachmentCount;
		NULL,					// const VkAttachmentReference*    pInputAttachments;
		1,					// uint32_t                        colorAttachmentCount;
		&attachment_reference[0],		// const VkAttachmentReference*    pColorAttachments;
		NULL,					// const VkAttachmentReference*    pResolveAttachments;
		&attachment_reference[1],		// const VkAttachmentReference*    pDepthStencilAttachment;
		0,					// uint32_t                        preserveAttachmentCount;
		NULL					// const uint32_t*                 pPreserveAttachments;
	};

	VkRenderPassCreateInfo renderpass_crinf = {
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,	// VkStructureType                   sType;
		NULL,						// const void*                       pNext;
		0,						// VkRenderPassCreateFlags           flags;
		2,						// uint32_t                          attachmentCount;
		attachment_description,				// const VkAttachmentDescription*    pAttachments;
		1,						// uint32_t                          subpassCount;
		&subpass_description,				// const VkSubpassDescription*       pSubpasses;
		0,						// uint32_t                          dependencyCount;
		NULL						// const VkSubpassDependency*        pDependencies;
	};
	// TODO: do we need this?
	result = vkCreateRenderPass(vk.device, &renderpass_crinf, NULL, &vk.mesh_renderpass);
	if( result != VK_SUCCESS )
	{
		log_fatal("vkCreateRenderPass = %s", vulkan_result(result));
//		goto VK_INIT_PIPELINE;
	}



}


void vk_mesh_pipeline_end(void)
{
	vkDestroyRenderPass(vk.device, vk.mesh_renderpass, NULL);
	vkDestroyPipeline(vk.device, vk.mesh_pipeline, NULL);
	vkDestroyPipelineLayout(vk.device, vk.mesh_pipeline_layout, NULL);
	vkDestroyDescriptorSetLayout(vk.device, vk.mesh_ubo_layout, NULL);
	vkFreeMemory(vk.device, vk.mesh_ubo_device.memory, NULL);
	vkDestroyBuffer(vk.device, vk.mesh_ubo_device.buffer, NULL);
	vkFreeMemory(vk.device, vk.mesh_ubo_host.memory, NULL);
	vkDestroyBuffer(vk.device, vk.mesh_ubo_host.buffer, NULL);
	vkDestroyShaderModule(vk.device, vk.mesh_shader_vert, NULL);
	vkDestroyShaderModule(vk.device, vk.mesh_shader_frag, NULL);
}


int vulkan_init(void)
{
	VkResult result;

	int layer_count = 1;
	const char *layer_names[] = {
		"VK_LAYER_LUNARG_standard_validation",
	};

	/* Vulkan Initialisation is here! */
	VkInstanceCreateInfo vkici = {
		VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,	// VkStructureType             sType;
		NULL,					// const void*                 pNext;
		0,					// VkInstanceCreateFlags       flags;
		0,					// const VkApplicationInfo*    pApplicationInfo;
		layer_count,					// uint32_t                    enabledLayerCount;
		layer_names,					// const char* const*          ppEnabledLayerNames;
		vulkan_extention_count,			// uint32_t                    enabledExtensionCount;
		vulkan_extension_strings		// const char* const*          ppEnabledExtensionNames;
	};

	result = vkCreateInstance(&vkici, 0, &vk.instance);
	if( result != VK_SUCCESS )
	{
		log_fatal("vkCreateInstance = %s", vulkan_result(result));
		goto VK_INIT_INSTANCE;
	}
	unsigned int device_count = 0;
	result = vkEnumeratePhysicalDevices(vk.instance, &device_count, NULL);
	if( result != VK_SUCCESS )
	{
		log_fatal("vkEnumeratePhysicalDevices1 = %s", vulkan_result(result));
		goto VK_INIT_ENUMERATE;
	}

	VkPhysicalDevice *vkpd = NULL;
	vkpd = malloc(sizeof(VkPhysicalDevice) * device_count);
	result = vkEnumeratePhysicalDevices(vk.instance, &device_count, vkpd);
	if( result != VK_SUCCESS )
	{
		log_fatal("vkEnumeratePhysicalDevices2 = %s", vulkan_result(result));
		goto VK_INIT_ENUMERATE;
	}

	int desired_device = 0;
	int desired_device_vram = 0;
	VkPhysicalDeviceProperties device_properties;

	for(int i=0; i<device_count; i++)
	{
		vkGetPhysicalDeviceProperties(vkpd[i], &device_properties);
		VkPhysicalDeviceMemoryProperties pd_mem;
		vkGetPhysicalDeviceMemoryProperties(vkpd[i], &pd_mem);

		if( device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU )
		{
			// might be a good device, let's check VRAM amount
			for(int j=0; j<pd_mem.memoryHeapCount; j++)
			if( pd_mem.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
			{
				int vram = (pd_mem.memoryHeaps[j].size / (1024*1024));
				if( desired_device_vram < vram )
				{
					// it has more VRAM than the last device we checked
					desired_device_vram = vram;
					desired_device = i;
				}
			}
		}
	}

	// we just want this information, to put in a log
	vkGetPhysicalDeviceProperties(vkpd[desired_device], &vk.device_properties);
	// we need this information for later.
	vkGetPhysicalDeviceMemoryProperties(vkpd[desired_device], &vk.device_mem_props);

	log_info("GPU         : %s", vk.device_properties.deviceName);
	log_info("GPU VRAM    : %dMb", desired_device_vram);

	uint32_t queuefamily_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(vkpd[desired_device], &queuefamily_count, NULL);
	VkQueueFamilyProperties *queuefamily_properties = malloc(sizeof(VkQueueFamilyProperties) * queuefamily_count);
	vkGetPhysicalDeviceQueueFamilyProperties(vkpd[desired_device], &queuefamily_count, queuefamily_properties);
	uint32_t desired_queuefamily = UINT32_MAX;
	for(int i=0; i<queuefamily_count; i++)
	{
		log_info("queuefamily[%d].flags = %d", i,
			queuefamily_properties[i].queueFlags);
		vulkan_queueflags(queuefamily_properties[i].queueFlags);

		VkQueueFlags wanted = 
			VK_QUEUE_GRAPHICS_BIT |
			VK_QUEUE_COMPUTE_BIT |
			VK_QUEUE_TRANSFER_BIT;

		if( (queuefamily_properties[i].queueFlags & wanted) == wanted )
		{
			desired_queuefamily = i;
		}
	}
	if(desired_queuefamily == UINT32_MAX)
	{
		log_fatal("Could not find a desired QueueFamily");
		return 1;
	}

	free(queuefamily_properties);

	float queue_priority = 1.0f;
	VkDeviceQueueCreateInfo vkqci = {
		VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,	// VkStructureType             sType;
		NULL,						// const void*                 pNext;
		0,						// VkDeviceQueueCreateFlags    flags;
		desired_queuefamily,				// uint32_t                    queueFamilyIndex;
		1,						// uint32_t                    queueCount;
		&queue_priority					// const float*                pQueuePriorities;
	};

	const char * vk_dext_str[1] = {"VK_KHR_swapchain"};
	VkDeviceCreateInfo vkdci = {
		VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,	// VkStructureType                    sType;
		NULL,					// const void*                        pNext;
		0,					// VkDeviceCreateFlags                flags;
		1,					// uint32_t                           queueCreateInfoCount;
		&vkqci,					// const VkDeviceQueueCreateInfo*     pQueueCreateInfos;
		0,					// uint32_t                           enabledLayerCount;
		0,					// const char* const*                 ppEnabledLayerNames;
		1,					// uint32_t                           enabledExtensionCount;
		vk_dext_str,				// const char* const*                 ppEnabledExtensionNames;
		0					// const VkPhysicalDeviceFeatures*    pEnabledFeatures;
	};
	result = vkCreateDevice(vkpd[desired_device], &vkdci, 0, &vk.device);
	if( result != VK_SUCCESS )
	{
		log_fatal("vkCreateDevice = %s", vulkan_result(result));
		goto VK_INIT_CREATEDEVICE;
	}

#ifdef VK_USE_PLATFORM_XCB_KHR
	VkXcbSurfaceCreateInfoKHR vksi = {
		VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,	// VkStructureType               sType;
		NULL,						// const void*                   pNext;
		0,						// VkXcbSurfaceCreateFlagsKHR    flags;
		xcb,						// xcb_connection_t*             connection;
		window						// xcb_window_t                  window;
	};
	result = vkCreateXcbSurfaceKHR(vk.instance, &vksi, NULL, &vk.surface);
	char os_surface_func[] = "Xcb";
#endif

#ifdef VK_USE_PLATFORM_XLIB_KHR
	VkXlibSurfaceCreateInfoKHR vksi = {
		VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,	// VkStructureType                sType;
		NULL,						// const void*                    pNext;
		0,						// VkXlibSurfaceCreateFlagsKHR    flags;
		display,					// Display*                       dpy;
		window						// Window                         window;
	};
	result = vkCreateXlibSurfaceKHR(vk.instance, &vksi, NULL, &vk.surface);
	char os_surface_func[] ="Xlib";
#endif

#ifdef VK_USE_PLATFORM_WIN32_KHR
	VkWin32SurfaceCreateInfoKHR vksi = {
		VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,// VkStructureType                 sType;
		NULL,						// const void*                     pNext;
		0,						// VkWin32SurfaceCreateFlagsKHR    flags;
		hInst,						// HINSTANCE                       hinstance;
		hWnd						// HWND                            hwnd;
	};
	result = vkCreateWin32SurfaceKHR(vk.instance, &vksi, NULL, &vk.surface);
	char os_surface_func[] ="Win32";
#endif

#ifdef VK_USE_PLATFORM_MACOS_MVK
	VkMacOSSurfaceCreateInfoMVK vksi = {
		VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK,// VkStructureType                 sType;
		NULL,						// const void*                     pNext;
		0,						// VkMacOSSurfaceCreateFlagsMVK    flags;
		pView						// const void*                     pView;
	};
	result = vkCreateMacOSSurfaceMVK(vk.instance, &vksi, NULL, &vk.surface);
	char os_surface_func[] = "MacOS";
#endif
	if( result != VK_SUCCESS )
	{
		log_fatal("vkCreate%sSurfaceMVK = %s", os_surface_func, vulkan_result(result));
		goto VK_INIT_CREATESURFACE;
	}

	VkBool32 present_supported = VK_FALSE;
	result = vkGetPhysicalDeviceSurfaceSupportKHR(vkpd[desired_device], desired_queuefamily, vk.surface, &present_supported);
	if( result != VK_SUCCESS )
	{
		log_warning("vkGetPhysicalDeviceSurfaceSupportKHR = %s", vulkan_result(result));
	}
	log_info("vkGetPhysicalDeviceSurfaceSupportKHR(%d) = %s", desired_queuefamily, present_supported?"VK_TRUE":"VK_FALSE");

	uint32_t surface_format_count = 0;
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(vkpd[desired_device], vk.surface, &surface_format_count, NULL);
	if( result != VK_SUCCESS )
	{
		log_warning("vkGetPhysicalDeviceSurfaceFormatsKHR(NULL) = %s", vulkan_result(result));
	}
	log_info("vkGetPhysicalDeviceSurfaceFormatsKHR = %d", surface_format_count);

	VkSurfaceFormatKHR * surface_formats = malloc(sizeof(VkSurfaceFormatKHR) * surface_format_count);
	if( surface_formats == NULL )
	{
		log_fatal("malloc(surface_formats)");
		free(surface_formats);
		goto VK_INIT_SURFACEFORMATS;
	}
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(vkpd[desired_device], vk.surface, &surface_format_count, surface_formats);
	if( result != VK_SUCCESS )
	{
		log_warning("vkGetPhysicalDeviceSurfaceFormatsKHR = %s", vulkan_result(result));
	}

	int desired_format = -1;
	for(int i = 0; i< surface_format_count; i++)
	{
		log_info( "cs=%s, f=%s", vulkan_colorspace(surface_formats[i].colorSpace),
						vulkan_format(surface_formats[i].format) );
		switch( surface_formats[i].format ){
		case VK_FORMAT_B8G8R8A8_SRGB:
			desired_format = i;
			pixel_format = surface_formats[i].format;
			color_space = surface_formats[i].colorSpace;
			break;
		default:
			continue;
		}
	}
	free(surface_formats);
	if(desired_format == -1)
	{
		log_fatal("Could not find a desired surface format");
		return 1;
	}

	// get the surface capabilities
	VkSurfaceCapabilitiesKHR surface_caps;
	result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkpd[desired_device], vk.surface, &surface_caps);
	if( result != VK_SUCCESS )
	{
		log_warning("vkGetPhysicalDeviceSurfaceCapabilitiesKHR = %s", vulkan_result(result));
	}

	// and the present modes
	uint32_t present_mode_count = 0;
	result = vkGetPhysicalDeviceSurfacePresentModesKHR(vkpd[desired_device], vk.surface, &present_mode_count, NULL);
	if( result != VK_SUCCESS )
	{
		log_warning("vkGetPhysicalDeviceSurfacePresentModesKHR(NULL) = %s", vulkan_result(result));
	}

	VkPresentModeKHR *present_modes = malloc(sizeof(VkPresentModeKHR) * present_mode_count);
	if( present_modes == NULL )
	{
		log_fatal("malloc(VkPresentModeKHR)");
		goto VK_INIT_PRESENTMODES;
	}
	result = vkGetPhysicalDeviceSurfacePresentModesKHR(vkpd[desired_device], vk.surface, &present_mode_count, present_modes);
	if( result != VK_SUCCESS )
	{
		log_warning("vkGetPhysicalDeviceSurfacePresentModesKHR = %s", vulkan_result(result));
	}

	free(vkpd);	// don't need this anymore
	vkpd = NULL;

	log_info("present_mode_count = %d", present_mode_count);
	for(int i=0; i<present_mode_count; i++)
	{
		log_info("%d:%s", i, vulkan_presentmode(present_modes[i]));
	}

	// TODO: make this logic better
	// now determine what present mode we want (vsync setting)
	VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;	// vsync
	for( int i=0; i<present_mode_count; i++)
	{	// hunting for types of no-vsync, in order of preference
		if( present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR )
		{
			swapchain_present_mode = present_modes[i];
			break;
		}
		else if( present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR )
		{
			swapchain_present_mode = present_modes[i];
		}
		else if( (swapchain_present_mode != VK_PRESENT_MODE_MAILBOX_KHR) &&
			 (present_modes[i] == VK_PRESENT_MODE_FIFO_RELAXED_KHR) )
		{
			swapchain_present_mode = present_modes[i];
		}
	}
	free(present_modes); // don't need this anymore


	// how many swapchain images do we want?
	int wanted_image_count = 3;
	vk.display_buffer_count = surface_caps.minImageCount;
	if( wanted_image_count > surface_caps.maxImageCount)
		wanted_image_count = surface_caps.maxImageCount;
	vk.display_buffer_count = wanted_image_count;

	vkGetDeviceQueue(vk.device, desired_queuefamily, 0, &vk.queue);
	log_debug("vkGetDeviceQueue");

	// create the swapchain
	VkExtent2D vk_extent = {vid_width, vid_height};
	VkSwapchainCreateInfoKHR vkswapchaincrinf = {
		VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,	// VkStructureType                  sType;
		NULL,						// const void*                      pNext;
		0,						// VkSwapchainCreateFlagsKHR        flags;
		vk.surface,					// VkSurfaceKHR                     surface;
		vk.display_buffer_count,			// uint32_t                         minImageCount;
		pixel_format,					// VkFormat                         imageFormat;
		color_space,					// VkColorSpaceKHR                  imageColorSpace;
		vk_extent,					// VkExtent2D                       imageExtent;
		1,						// uint32_t                         imageArrayLayers;
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,		// VkImageUsageFlags                imageUsage;
		VK_SHARING_MODE_EXCLUSIVE,			// VkSharingMode                    imageSharingMode;
		0,						// uint32_t                         queueFamilyIndexCount;
		NULL,						// const uint32_t*                  pQueueFamilyIndices;
		VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,		// VkSurfaceTransformFlagBitsKHR    preTransform;
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,		// VkCompositeAlphaFlagBitsKHR      compositeAlpha;
		swapchain_present_mode,				// VkPresentModeKHR                 presentMode;
		VK_TRUE,					// VkBool32                         clipped;
		NULL						// VkSwapchainKHR                   oldSwapchain;
	};
	result = vkCreateSwapchainKHR(vk.device, &vkswapchaincrinf, 0, &vk.swapchain);
	if( result != VK_SUCCESS )
	{
		log_fatal("vkCreateSwapchainKHR = %s", vulkan_result(result));
		goto VK_INIT_SWAPCHAIN;
	}

	// create renderpass
	VkAttachmentDescription attach_desc[] = {{
			0,						// VkAttachmentDescriptionFlags    flags;
			pixel_format,					// VkFormat                        format;
			VK_SAMPLE_COUNT_1_BIT,				// VkSampleCountFlagBits           samples;
			VK_ATTACHMENT_LOAD_OP_CLEAR,			// VkAttachmentLoadOp              loadOp;
			VK_ATTACHMENT_STORE_OP_STORE,			// VkAttachmentStoreOp             storeOp;
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,		// VkAttachmentLoadOp              stencilLoadOp;
			VK_ATTACHMENT_STORE_OP_DONT_CARE,		// VkAttachmentStoreOp             stencilStoreOp;
			VK_IMAGE_LAYOUT_UNDEFINED,			// VkImageLayout                   initialLayout;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL	// VkImageLayout                   finalLayout;
		},{
		0,								// VkAttachmentDescriptionFlags    flags;
			VK_FORMAT_D32_SFLOAT,					// VkFormat                        format;
			VK_SAMPLE_COUNT_1_BIT,					// VkSampleCountFlagBits           samples;
			VK_ATTACHMENT_LOAD_OP_CLEAR,				// VkAttachmentLoadOp              loadOp;
			VK_ATTACHMENT_STORE_OP_STORE,				// VkAttachmentStoreOp             storeOp;
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,			// VkAttachmentLoadOp              stencilLoadOp;
			VK_ATTACHMENT_STORE_OP_DONT_CARE,			// VkAttachmentStoreOp             stencilStoreOp;
			VK_IMAGE_LAYOUT_UNDEFINED,				// VkImageLayout                   initialLayout;
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL	// VkImageLayout                   finalLayout;
		}
	};

	VkAttachmentReference attach_refs[] = {{
			0,						// uint32_t         attachment;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL	// VkImageLayout    layout;
		}, {
			1,						// uint32_t         attachment;
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL// VkImageLayout    layout;
		}
	};

	VkSubpassDescription subpass_desc[] = {{
		0,					// VkSubpassDescriptionFlags       flags;
		VK_PIPELINE_BIND_POINT_GRAPHICS,	// VkPipelineBindPoint             pipelineBindPoint;
		0,					// uint32_t                        inputAttachmentCount;
		NULL,					// const VkAttachmentReference*    pInputAttachments;
		1,					// uint32_t                        colorAttachmentCount;
		&attach_refs[0],			// const VkAttachmentReference*    pColorAttachments;
		NULL,					// const VkAttachmentReference*    pResolveAttachments;
		&attach_refs[1],			// const VkAttachmentReference*    pDepthStencilAttachment;
		0,					// uint32_t                        preserveAttachmentCount;
		NULL					// const uint32_t*                 pPreserveAttachments;
	}};

	VkRenderPassCreateInfo render_pass_crinf = {
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,	// VkStructureType                   sType;
		NULL,						// const void*                       pNext;
		0,						// VkRenderPassCreateFlags           flags;
		2,						// uint32_t                          attachmentCount;
		attach_desc,					// const VkAttachmentDescription*    pAttachments;
		1,						// uint32_t                          subpassCount;
		subpass_desc,					// const VkSubpassDescription*       pSubpasses;
		0,						// uint32_t                          dependencyCount;
		NULL						// const VkSubpassDependency*        pDependencies;
	};

	result = vkCreateRenderPass(vk.device, &render_pass_crinf, NULL, &vk.renderpass );
	if( result != VK_SUCCESS )
	{
		log_fatal("vkCreateRenderPass = %s", vulkan_result(result));
		goto VK_INIT_RENDERPASS;
	}


	// create the shaders

	VkShaderModuleCreateInfo shader_vert_crinf = {
		VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,	// VkStructureType              sType;
		NULL,						// const void*                  pNext;
		0,						// VkShaderModuleCreateFlags    flags;
		build_vert_spv_len,				// size_t                       codeSize;
		(const uint32_t*)build_vert_spv			// const uint32_t*              pCode;
	};

	VkShaderModuleCreateInfo shader_frag_crinf = {
		VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,	// VkStructureType              sType;
		NULL,						// const void*                  pNext;
		0,						// VkShaderModuleCreateFlags    flags;
		build_frag_spv_len,				// size_t                       codeSize;
		(const uint32_t*)build_frag_spv			// const uint32_t*              pCode;
	};

	vkCreateShaderModule(vk.device, &shader_vert_crinf, NULL, &vk.shader_vert);
	if( result != VK_SUCCESS )
	{
		log_fatal("vkCreateShaderModule(vertex) = %s", vulkan_result(result));
		goto VK_INIT_CREATE_SHADER_VERT;
	}
	vkCreateShaderModule(vk.device, &shader_frag_crinf, NULL, &vk.shader_frag);
	if( result != VK_SUCCESS )
	{
		log_fatal("vkCreateShaderModule(fragment) = %s", vulkan_result(result));
		goto VK_INIT_CREATE_SHADER_FRAG;
	}

	// TODO: fancier shader
	int ubo_buffer_size = sizeof(float);

	result = vk_buffer( VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&vk.ubo_host, ubo_buffer_size, NULL );
	if(result != VK_SUCCESS)
	{
		log_fatal("vk_buffer = %s", vulkan_result(result));
		goto VK_INIT_UBO_HOST;
	}

	result = vk_buffer( VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&vk.ubo_device, vk.ubo_host.size, NULL );
	if(result != VK_SUCCESS)
	{
		log_fatal("vk_buffer = %s", vulkan_result(result));
		goto VK_INIT_UBO_DEVICE;
	}

	VkDescriptorPoolSize desc_pool_size = {
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,	// VkDescriptorType    type;
		2					// uint32_t            descriptorCount;
	};

	VkDescriptorPoolCreateInfo desc_pool_crinf = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,	// VkStructureType                sType;
		NULL,						// const void*                    pNext;
		0,						// VkDescriptorPoolCreateFlags    flags;
		2,						// uint32_t                       maxSets;
		1,						// uint32_t                       poolSizeCount;
		&desc_pool_size					// const VkDescriptorPoolSize*    pPoolSizes;
	};

	result = vkCreateDescriptorPool(vk.device, &desc_pool_crinf, NULL, &vk.descriptor_pool);
	if( result != VK_SUCCESS )
	{
		log_fatal("vkCreateDescriptorPool = %s", vulkan_result(result));
		goto VK_INIT_DESCRIPTOR_POOL;
	}

	VkDescriptorSetLayoutBinding descriptor_layout_binding = {
		0,					// uint32_t              binding;
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,	// VkDescriptorType      descriptorType;
		1,					// uint32_t              descriptorCount;
		VK_SHADER_STAGE_FRAGMENT_BIT,		// VkShaderStageFlags    stageFlags;
		NULL					// const VkSampler*      pImmutableSamplers;
	};

	VkDescriptorSetLayoutCreateInfo descriptor_layout_crinf = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,	// VkStructureType                        sType;
		NULL,							// const void*                            pNext;
		0,							// VkDescriptorSetLayoutCreateFlags       flags;
		1,							// uint32_t                               bindingCount;
		&descriptor_layout_binding				// const VkDescriptorSetLayoutBinding*    pBindings;
	};


	result = vkCreateDescriptorSetLayout(vk.device, &descriptor_layout_crinf, NULL, &vk.layout_ubo);
	if( result != VK_SUCCESS )
	{
		log_fatal("vkCreateDescriptorSetLayout = %s", vulkan_result(result));
		goto VK_INIT_DESCRIPTOR_SET_LAYOUT;
	}

	VkDescriptorSetAllocateInfo desc_set_alloc_info = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,	// VkStructureType                 sType;
		NULL,						// const void*                     pNext;
		vk.descriptor_pool,				// VkDescriptorPool                descriptorPool;
		1,						// uint32_t                        descriptorSetCount;
		&vk.layout_ubo					// const VkDescriptorSetLayout*    pSetLayouts;
	};

	result = vkAllocateDescriptorSets(vk.device, &desc_set_alloc_info, &vk.descriptor_set);
	if( result != VK_SUCCESS )
	{
		log_warning("vkAllocateDescriptorSets = %s", vulkan_result(result));
	}

	VkDescriptorBufferInfo desc_buf_info = {
		vk.ubo_host.buffer,	// VkBuffer        buffer;
		0,			// VkDeviceSize    offset;
		sizeof(float)		// VkDeviceSize    range;
	};
	VkWriteDescriptorSet desc_write_set = {
		VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,	// VkStructureType                  sType;
		NULL,					// const void*                      pNext;
		vk.descriptor_set,			// VkDescriptorSet                  dstSet;
		0,					// uint32_t                         dstBinding;
		0,					// uint32_t                         dstArrayElement;
		1,					// uint32_t                         descriptorCount;
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,	// VkDescriptorType                 descriptorType;
		NULL,					// const VkDescriptorImageInfo*     pImageInfo;
		&desc_buf_info,				// const VkDescriptorBufferInfo*    pBufferInfo;
		NULL					// const VkBufferView*              pTexelBufferView;
	};
	vkUpdateDescriptorSets(vk.device, 1, &desc_write_set, 0, NULL);
	log_debug("vkUpdateDescriptorSets");

// create the pipeline
	// shader and vertex input
	VkPipelineShaderStageCreateInfo shader_stage_crinf[2] = {
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,	// VkStructureType                     sType;
			NULL,							// const void*                         pNext;
			0,							// VkPipelineShaderStageCreateFlags    flags;
			VK_SHADER_STAGE_VERTEX_BIT,				// VkShaderStageFlagBits               stage;
			vk.shader_vert,						// VkShaderModule                      module;
			"main",							// const char*                         pName;
			NULL							// const VkSpecializationInfo*         pSpecializationInfo;
		},
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,	// VkStructureType                     sType;
			NULL,							// const void*                         pNext;
			0,							// VkPipelineShaderStageCreateFlags    flags;
			VK_SHADER_STAGE_FRAGMENT_BIT,				// VkShaderStageFlagBits               stage;
			vk.shader_frag,						// VkShaderModule                      module;
			"main",							// const char*                         pName;
			NULL							// const VkSpecializationInfo*         pSpecializationInfo;
		}
	};

	VkPipelineVertexInputStateCreateInfo vertex_input_state_crinf = {
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,	// VkStructureType                             sType;
		NULL,								// const void*                                 pNext;
		0,								// VkPipelineVertexInputStateCreateFlags       flags;
		0,								// uint32_t                                    vertexBindingDescriptionCount;
		NULL,								// const VkVertexInputBindingDescription*      pVertexBindingDescriptions;
		0,								// uint32_t                                    vertexAttributeDescriptionCount;
		NULL								// const VkVertexInputAttributeDescription*    pVertexAttributeDescriptions;
	};

	// input assembly, viewport
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state_crinf = {
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,	// VkStructureType                            sType;
		NULL,								// const void*                                pNext;
		0,								// VkPipelineInputAssemblyStateCreateFlags    flags;
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,				// VkPrimitiveTopology                        topology;
		VK_FALSE							// VkBool32                                   primitiveRestartEnable;		
	};

	VkViewport viewport = {0.0f,0.0f,(float)vid_width,(float)vid_height,0.0f,1.0f};
	VkRect2D scissor = {{0,0},{vid_width,vid_height}};
	VkPipelineViewportStateCreateInfo viewport_state_crinf = {
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,	// VkStructureType                       sType;
		NULL,							// const void*                           pNext;
		0,							// VkPipelineViewportStateCreateFlags    flags;
		1,							// uint32_t                              viewportCount;
		&viewport,						// const VkViewport*                     pViewports;
		1,							// uint32_t                              scissorCount;
		&scissor						// const VkRect2D*                       pScissors;
	};

	// Rasterisation, multisampling
	VkPipelineRasterizationStateCreateInfo rasterization_state_crinf = {
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,	// VkStructureType                            sType;
		NULL,								// const void*                                pNext;
		0,								// VkPipelineRasterizationStateCreateFlags    flags;
		VK_FALSE,							// VkBool32                                   depthClampEnable;
		VK_FALSE,							// VkBool32                                   rasterizerDiscardEnable;
		VK_POLYGON_MODE_FILL,						// VkPolygonMode                              polygonMode;
		VK_CULL_MODE_BACK_BIT,						// VkCullModeFlags                            cullMode;
		VK_FRONT_FACE_COUNTER_CLOCKWISE,				// VkFrontFace                                frontFace;
		VK_FALSE,							// VkBool32                                   depthBiasEnable;
		0.0f,								// float                                      depthBiasConstantFactor;
		0.0f,								// float                                      depthBiasClamp;
		0.0f,								// float                                      depthBiasSlopeFactor;
		1.0f								// float                                      lineWidth;		
	};

	VkPipelineMultisampleStateCreateInfo multisample_state_crinf = {
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,	// VkStructureType                          sType;
		NULL,								// const void*                              pNext;
		0,								// VkPipelineMultisampleStateCreateFlags    flags;
		VK_SAMPLE_COUNT_1_BIT,						// VkSampleCountFlagBits                    rasterizationSamples;
		VK_FALSE,							// VkBool32                                 sampleShadingEnable;
		1.0f,								// float                                    minSampleShading;
		NULL,								// const VkSampleMask*                      pSampleMask;
		VK_FALSE,							// VkBool32                                 alphaToCoverageEnable;
		VK_FALSE							// VkBool32                                 alphaToOneEnable;
	};

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state_crinf = {
		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,	// VkStructureType                           sType;
		NULL,								// const void*                               pNext;
		0,								// VkPipelineDepthStencilStateCreateFlags    flags;
		VK_TRUE,							// VkBool32                                  depthTestEnable;
		VK_TRUE,							// VkBool32                                  depthWriteEnable;
		VK_COMPARE_OP_LESS_OR_EQUAL,					// VkCompareOp                               depthCompareOp;
		VK_FALSE,							// VkBool32                                  depthBoundsTestEnable;
		VK_FALSE,							// VkBool32                                  stencilTestEnable;
		{0,0,0,VK_COMPARE_OP_ALWAYS,0,0,0				// VkStencilOpState                          front;
							// VkStencilOp    failOp;
							// VkStencilOp    passOp;
							// VkStencilOp    depthFailOp;
							// VkCompareOp    compareOp;
							// uint32_t       compareMask;
							// uint32_t       writeMask;
							// uint32_t       reference;
		}, {0,0,0,VK_COMPARE_OP_ALWAYS,0,0,0				// VkStencilOpState                          back;
		},
		0.0f,								// float                                     minDepthBounds;
		0.0f								// float                                     maxDepthBounds;
	};

	// colour blending
	VkPipelineColorBlendAttachmentState color_blend_attachment_state = {
		VK_FALSE,			// VkBool32                 blendEnable;
		VK_BLEND_FACTOR_ONE,		// VkBlendFactor            srcColorBlendFactor;
		VK_BLEND_FACTOR_ZERO,		// VkBlendFactor            dstColorBlendFactor;
		VK_BLEND_OP_ADD,		// VkBlendOp                colorBlendOp;
		VK_BLEND_FACTOR_ONE,		// VkBlendFactor            srcAlphaBlendFactor;
		VK_BLEND_FACTOR_ZERO,		// VkBlendFactor            dstAlphaBlendFactor;
		VK_BLEND_OP_ADD,		// VkBlendOp                alphaBlendOp;
		VK_COLOR_COMPONENT_R_BIT |	// VkColorComponentFlags    colorWriteMask;
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT
	};

	VkPipelineColorBlendStateCreateInfo color_blend_state_crinf = {
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,	// VkStructureType                               sType;
		NULL,								// const void*                                   pNext;
		0,								// VkPipelineColorBlendStateCreateFlags          flags;
		VK_FALSE,							// VkBool32                                      logicOpEnable;
		VK_LOGIC_OP_COPY,						// VkLogicOp                                     logicOp;
		1,								// uint32_t                                      attachmentCount;
		&color_blend_attachment_state,					// const VkPipelineColorBlendAttachmentState*    pAttachments;
		{0.0f,0.0f,0.0f,0.0f}						// float                                         blendConstants[4];
	};

	// Pipeline creation
	VkPipelineLayoutCreateInfo layout_crinf = {
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,	// VkStructureType                 sType;
		NULL,						// const void*                     pNext;
		0,						// VkPipelineLayoutCreateFlags     flags;
		1,						// uint32_t                        setLayoutCount;
		&vk.layout_ubo,					// const VkDescriptorSetLayout*    pSetLayouts;
		0,						// uint32_t                        pushConstantRangeCount;
		NULL						// const VkPushConstantRange*      pPushConstantRanges;
	};
	result = vkCreatePipelineLayout(vk.device, &layout_crinf, NULL, &vk.pipeline_layout);
	if( result != VK_SUCCESS )
	{
		log_fatal("vkCreatePipelineLayout = %s", vulkan_result(result));
		goto VK_INIT_PIPELINE_LAYOUT;
	}

	VkGraphicsPipelineCreateInfo pipeline_crinf = {
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,	// VkStructureType                                  sType;
		NULL,							// const void*                                      pNext;
		0,							// VkPipelineCreateFlags                            flags;
		2,							// uint32_t                                         stageCount;
		&shader_stage_crinf[0],					// const VkPipelineShaderStageCreateInfo*           pStages;
		&vertex_input_state_crinf,				// const VkPipelineVertexInputStateCreateInfo*      pVertexInputState;
		&input_assembly_state_crinf,				// const VkPipelineInputAssemblyStateCreateInfo*    pInputAssemblyState;
		NULL,							// const VkPipelineTessellationStateCreateInfo*     pTessellationState;
		&viewport_state_crinf,					// const VkPipelineViewportStateCreateInfo*         pViewportState;
		&rasterization_state_crinf,				// const VkPipelineRasterizationStateCreateInfo*    pRasterizationState;
		&multisample_state_crinf,				// const VkPipelineMultisampleStateCreateInfo*      pMultisampleState;
		&depth_stencil_state_crinf,				// const VkPipelineDepthStencilStateCreateInfo*     pDepthStencilState;
		&color_blend_state_crinf,				// const VkPipelineColorBlendStateCreateInfo*       pColorBlendState;
		NULL,							// const VkPipelineDynamicStateCreateInfo*          pDynamicState;
		vk.pipeline_layout,					// VkPipelineLayout                                 layout;
		vk.renderpass,							// VkRenderPass                                     renderPass;
		0,							// uint32_t                                         subpass;
		VK_NULL_HANDLE,						// VkPipeline                                       basePipelineHandle;
		-1							// int32_t                                          basePipelineIndex;
	};

	result = vkCreateGraphicsPipelines(vk.device, VK_NULL_HANDLE, 1, &pipeline_crinf, NULL, &vk.pipeline);
	if( result != VK_SUCCESS )
	{
		log_fatal("vkCreateGraphicsPipelines = %s", vulkan_result(result));
		goto VK_INIT_PIPELINE;
	}

	// more commandbuffer
	VkCommandBufferBeginInfo vk_cmdbegin = {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// VkStructureType                          sType;
		NULL,						// const void*                              pNext;
		VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,	// VkCommandBufferUsageFlags                flags;
		NULL						// const VkCommandBufferInheritanceInfo*    pInheritanceInfo;
	};

	VkImageSubresourceRange image_subresource_range = {
		VK_IMAGE_ASPECT_COLOR_BIT,	// VkImageAspectFlags    aspectMask;
		0,				// uint32_t              baseMipLevel;
		1,				// uint32_t              levelCount;
		0,				// uint32_t              baseArrayLayer;
		1				// uint32_t              layerCount;
	};

	// create the swapchain framebuffers
	// TODO
	if(vk_malloc_swapchain(&vk))
	{
		log_fatal("vk_malloc_swapchain() failed");
		goto VK_INIT_MALLOC_SWAPCHAIN;
	}

	// create the commandpool for the commandbuffers
	VkCommandPoolCreateInfo vk_cmdpoolcrinf = {
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,	// VkStructureType             sType;
		NULL,						// const void*                 pNext;
		0,						// VkCommandPoolCreateFlags    flags;
		0						// uint32_t                    queueFamilyIndex;
	};

	result = vkCreateCommandPool(vk.device, &vk_cmdpoolcrinf, 0, &vk.commandpool);
	if( result != VK_SUCCESS )
	{
		log_fatal("vkCreateCommandPool = %s", vulkan_result(result));
		goto VK_INIT_COMMAND_POOL;
	}

	VkCommandBufferAllocateInfo vk_cmdbufallocinf = {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,	// VkStructureType         sType;
		NULL,						// const void*             pNext;
		vk.commandpool,					// VkCommandPool           commandPool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,		// VkCommandBufferLevel    level;
		vk.display_buffer_count				// uint32_t                commandBufferCount;
	};
	result = vkAllocateCommandBuffers(vk.device, &vk_cmdbufallocinf, vk.sc_commandbuffer);
	if( result != VK_SUCCESS )
	{
		log_fatal("vkAllocateCommandBuffers = %s", vulkan_result(result));
		goto VK_INIT_ALLOC_COMMAND_BUFFERS;
	}

	result = vkGetSwapchainImagesKHR(vk.device, vk.swapchain, &vk.display_buffer_count, vk.sc_image);
	if( result != VK_SUCCESS )
	{
		log_fatal("vkGetSwapchainImagesKHR = %s", vulkan_result(result));
		goto VK_INIT_SWAPCHAIN_IMAGES;
	}

	// incase this fails, we can free the correct amount
	int allocated_imageviews = 0;
	int allocated_framebuffers = 0;
	int allocated_semaphores = 0;
	int allocated_depthbuffer = 0;

//	log_debug("this one");

	vk_mesh_pipeline();
	bunny = wf_load("data/stanford-bunny.obj");


	for( int i=0; i<vk.display_buffer_count; i++)
	{
		VkImageViewCreateInfo image_view_crinf = {
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,	// VkStructureType            sType;
			NULL,						// const void*                pNext;
			0,						// VkImageViewCreateFlags     flags;
			vk.sc_image[i],					// VkImage                    image;
			VK_IMAGE_VIEW_TYPE_2D,				// VkImageViewType            viewType;
			pixel_format,					// VkFormat                   format;
			{	VK_COMPONENT_SWIZZLE_IDENTITY,		// VkComponentMapping         components;
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY },	// VkImageSubresourceRange    subresourceRange;
			{ VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1 }
		};

		result = vkCreateImageView(vk.device, &image_view_crinf, NULL, &vk.sc_imageview[i]);
		if( result != VK_SUCCESS )
		{
			log_fatal("vkCreateImageView(%d) = %s", i+1, vulkan_result(result));
			goto VK_INIT_SWAPCHAIN_ITEMS;
		}
		allocated_imageviews++;

		result = vk_imagebuffer( vid_width, vid_height,
			VK_FORMAT_D32_SFLOAT,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_ASPECT_DEPTH_BIT,
			&vk.sc_depth[i]);
		allocated_depthbuffer++;

		VkImageView attachments[] = { vk.sc_imageview[i], vk.sc_depth[i].imageview };

		VkFramebufferCreateInfo fb_crinf = {
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,	// VkStructureType             sType;
			NULL,						// const void*                 pNext;
			0,						// VkFramebufferCreateFlags    flags;
			vk.renderpass,				// VkRenderPass                renderPass;
			2,						// uint32_t                    attachmentCount;
			attachments,					// const VkImageView*          pAttachments;
			vid_width,					// uint32_t                    width;
			vid_height,					// uint32_t                    height;
			1						// uint32_t                    layers;
		};

		result = vkCreateFramebuffer(vk.device, &fb_crinf, NULL, &vk.sc_framebuffer[i]);
		if( result != VK_SUCCESS )
		{
			log_fatal("vkCreateFramebuffer(%d) = %s", i+1, vulkan_result(result));
			goto VK_INIT_SWAPCHAIN_ITEMS;
		}
		allocated_framebuffers++;

		// the semaphore for this swapchain image
		VkSemaphoreCreateInfo vksemcrinf = {
			VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,	// VkStructureType           sType;
			NULL,						// const void*               pNext;
			0						// VkSemaphoreCreateFlags    flags;
		};
		result = vkCreateSemaphore(vk.device, &vksemcrinf, NULL, &vk.sc_semaphore[i]);
		if( result != VK_SUCCESS )
		{
			log_fatal("vkCreateSemaphore(%d) = %s", vulkan_result(result));
			goto VK_INIT_SWAPCHAIN_ITEMS;
		}
		allocated_semaphores++;

		VkImageMemoryBarrier vkb_present_to_draw = {
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,	// VkStructureType            sType;
			NULL,					// const void*                pNext;
			VK_ACCESS_MEMORY_READ_BIT,		// VkAccessFlags              srcAccessMask;
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,	// VkAccessFlags              dstAccessMask;
			VK_IMAGE_LAYOUT_UNDEFINED,		// VkImageLayout              oldLayout;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,	// VkImageLayout              newLayout;
			0,					// uint32_t                   srcQueueFamilyIndex;
			0,					// uint32_t                   dstQueueFamilyIndex;
			vk.sc_image[i],				// VkImage                    image;
			image_subresource_range			// VkImageSubresourceRange    subresourceRange;
		};

		VkClearValue clear_color[] = { { 0.2f, 0.0f, 0.0f, 1.0f }, { 1000.0f, 0.0f, 0.0f, 0.0f } };
		VkRenderPassBeginInfo render_pass_begin_info = {
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,	// VkStructureType        sType;
			NULL,						// const void*            pNext;
			vk.renderpass,				// VkRenderPass           renderPass;
			vk.sc_framebuffer[i],				// VkFramebuffer          framebuffer;
			{{0,0},{vid_width,vid_height}},			// VkRect2D               renderArea;
			2,						// uint32_t               clearValueCount;
			clear_color					// const VkClearValue*    pClearValues;
		};

		VkImageMemoryBarrier vkb_draw_to_present = {
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,	// VkStructureType            sType;
			NULL,					// const void*                pNext;
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,	// VkAccessFlags              srcAccessMask;
			VK_ACCESS_MEMORY_READ_BIT,		// VkAccessFlags              dstAccessMask;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,		// VkImageLayout              oldLayout;
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,	// VkImageLayout              newLayout;
			0,					// uint32_t                   srcQueueFamilyIndex;
			0,					// uint32_t                   dstQueueFamilyIndex;
			vk.sc_image[i],				// VkImage                    image;
			image_subresource_range			// VkImageSubresourceRange    subresourceRange;
		};

		VkBufferCopy buffer_copy[1] = {
			0,		// VkDeviceSize    srcOffset;
			0,		// VkDeviceSize    dstOffset;
			ubo_buffer_size	// VkDeviceSize    size;
		};
		VkBufferCopy mesh_buffer_copy[1] = {
			0,		// VkDeviceSize    srcOffset;
			0,		// VkDeviceSize    dstOffset;
			vk.mesh_ubo_host.size	// VkDeviceSize    size;
		};

		// fill the command buffer with commands
		result = vkBeginCommandBuffer(vk.sc_commandbuffer[i], &vk_cmdbegin);
		if( result != VK_SUCCESS ) { log_warning("vkBeginCommandBuffer = %s", vulkan_result(result)); }
		
//		vkCmdCopyBuffer( vk.sc_commandbuffer[i], vk.ubo_host.buffer, vk.ubo_device.buffer, 1, &buffer_copy[0]);

		vkCmdCopyBuffer( vk.sc_commandbuffer[i], vk.mesh_ubo_host.buffer, vk.mesh_ubo_device.buffer, 1, &mesh_buffer_copy[0]);



		vkCmdPipelineBarrier(vk.sc_commandbuffer[i],
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			0, 0, NULL, 0, NULL, 1, &vkb_present_to_draw);

		vkCmdBeginRenderPass( vk.sc_commandbuffer[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline( vk.sc_commandbuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vk.mesh_pipeline);
		vkCmdBindDescriptorSets( vk.sc_commandbuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
			vk.mesh_pipeline_layout, 0, 1, &vk.mesh_descriptor_set, 0, NULL);

		VkDeviceSize poffset = 0;
		vkCmdBindVertexBuffers( vk.sc_commandbuffer[i], 0, 1, &bunny->vertex_buffer.buffer, &poffset );
		vkCmdBindIndexBuffer( vk.sc_commandbuffer[i], bunny->index_buffer.buffer, 0, bunny->index_type );
		vkCmdDrawIndexed( vk.sc_commandbuffer[i], bunny->nf*3, 1, 0, 0, 0);

//		vkCmdDraw( vk.sc_commandbuffer[i], 4, 1, 0, 0 );

		vkCmdEndRenderPass( vk.sc_commandbuffer[i] );

		vkCmdPipelineBarrier(vk.sc_commandbuffer[i],VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,0,0,NULL,0,NULL,1,&vkb_draw_to_present);

		result = vkEndCommandBuffer( vk.sc_commandbuffer[i] );
		if( result != VK_SUCCESS ) { log_warning("vkEndCommandBuffer = %s", vulkan_result(result)); }
//		log_debug("command buffer %d filled", i+1);
	}
	vk.current_image = vk.display_buffer_count - 1;
	log_debug("Vulkan Initialisation complete");
	return 0;

VK_INIT_SWAPCHAIN_ITEMS:
	for(int i=0; i<allocated_semaphores; i++)
		vkDestroySemaphore(vk.device, vk.sc_semaphore[i], NULL);
	for(int i=0; i<allocated_framebuffers; i++)
		vkDestroyFramebuffer(vk.device, vk.sc_framebuffer[i], NULL);
	for(int i=0; i<allocated_imageviews; i++)
		vkDestroyImageView(vk.device, vk.sc_imageview[i], NULL);
	for(int i=0; i<allocated_depthbuffer; i++)
		vk_imagebuffer_end(&vk.sc_depth[i]);
	free(vk.sc_framebuffer);
	free(vk.sc_imageview);
	free(vk.sc_semaphore);
	free(vk.sc_commandbuffer);
	free(vk.sc_image);

VK_INIT_SWAPCHAIN_IMAGES:
VK_INIT_ALLOC_COMMAND_BUFFERS:
	vkDestroyCommandPool(vk.device, vk.commandpool, NULL);
VK_INIT_COMMAND_POOL:
VK_INIT_MALLOC_SWAPCHAIN:
	vkDestroyPipeline(vk.device, vk.pipeline, NULL);
VK_INIT_PIPELINE:
	vkDestroyPipelineLayout(vk.device, vk.pipeline_layout, NULL);
VK_INIT_PIPELINE_LAYOUT:
	vkDestroyDescriptorSetLayout(vk.device, vk.layout_ubo, NULL);
VK_INIT_DESCRIPTOR_SET_LAYOUT:
	vkDestroyDescriptorPool(vk.device, vk.descriptor_pool, NULL);
VK_INIT_DESCRIPTOR_POOL:
	vkFreeMemory(vk.device, vk.ubo_device.memory, NULL);
	vkDestroyBuffer(vk.device, vk.ubo_device.buffer, NULL);
VK_INIT_UBO_DEVICE:
	vkFreeMemory(vk.device, vk.ubo_host.memory, NULL);
	vkDestroyBuffer(vk.device, vk.ubo_host.buffer, NULL);
VK_INIT_UBO_HOST:
	vkDestroyShaderModule(vk.device, vk.shader_frag, NULL);
VK_INIT_CREATE_SHADER_FRAG:
	vkDestroyShaderModule(vk.device, vk.shader_vert, NULL);
VK_INIT_CREATE_SHADER_VERT:
	vkDestroyRenderPass(vk.device, vk.renderpass, NULL);
VK_INIT_RENDERPASS:
	vkDestroySwapchainKHR(vk.device, vk.swapchain, NULL);
VK_INIT_SWAPCHAIN:
VK_INIT_PRESENTMODES:
VK_INIT_SURFACEFORMATS:
VK_INIT_SURFACESUPPORT:
	vkDestroySurfaceKHR(vk.instance, vk.surface, NULL);
VK_INIT_CREATESURFACE:
	vkDestroyDevice(vk.device, NULL);
VK_INIT_CREATEDEVICE:
VK_INIT_ENUMERATE:
	if(vkpd != NULL)free(vkpd);
	vkDestroyInstance(vk.instance, NULL);
VK_INIT_INSTANCE:
	return 1;
}

void vulkan_end(void)
{
	VkResult result;
	result = vkQueueWaitIdle(vk.queue);

	vk_mesh_pipeline_end();
	wf_free(bunny);


	if( result != VK_SUCCESS )
	{
		log_warning("vkQueueWaitIdle = %s", vulkan_result(result));
//		goto TODO_ERROR_HANDLING;
	}
	for(int i=0; i<vk.display_buffer_count; i++)
	{
		vkDestroySemaphore(vk.device, vk.sc_semaphore[i], NULL);
		vkDestroyFramebuffer(vk.device, vk.sc_framebuffer[i], NULL);
		vkDestroyImageView(vk.device, vk.sc_imageview[i], NULL);
		vk_imagebuffer_end(&vk.sc_depth[i]);
	}
	free(vk.sc_framebuffer);
	free(vk.sc_imageview);
	free(vk.sc_semaphore);
	free(vk.sc_commandbuffer);
	free(vk.sc_image);

	vkDestroyCommandPool(vk.device, vk.commandpool, NULL);
	vkDestroyPipeline(vk.device, vk.pipeline, NULL);
	vkDestroyPipelineLayout(vk.device, vk.pipeline_layout, NULL);
	vkDestroyDescriptorSetLayout(vk.device, vk.layout_ubo, NULL);
	vkDestroyDescriptorPool(vk.device, vk.descriptor_pool, NULL);
	vkFreeMemory(vk.device, vk.ubo_device.memory, NULL);
	vkDestroyBuffer(vk.device, vk.ubo_device.buffer, NULL);
	vkFreeMemory(vk.device, vk.ubo_host.memory, NULL);
	vkDestroyBuffer(vk.device, vk.ubo_host.buffer, NULL);
	vkDestroyShaderModule(vk.device, vk.shader_vert, NULL);
	vkDestroyShaderModule(vk.device, vk.shader_frag, NULL);
	vkDestroyRenderPass(vk.device, vk.renderpass, NULL);
	vkDestroySwapchainKHR(vk.device, vk.swapchain, NULL);
	vkDestroySurfaceKHR(vk.instance, vk.surface, NULL);
	vkDestroyDevice(vk.device, NULL);
	vkDestroyInstance(vk.instance, NULL);
}

int vulkan_loop(float current_time)
{
	log_trace("frame time = %f", current_time);
	uint32_t next_image = 0;
	VkResult result;
	result = vkAcquireNextImageKHR(vk.device, vk.swapchain, 10000000, vk.sc_semaphore[vk.current_image], VK_NULL_HANDLE, &next_image);
	if(result != VK_SUCCESS)
	{
		log_warning("vkAcquireNextImageKHR = %s", vulkan_result(result));
		switch(result) {
		case VK_ERROR_OUT_OF_DATE_KHR:
		case VK_ERROR_DEVICE_LOST:
			return 1;
		default:
			break;
		}
	}
//	log_debug("current = %d, next = %d", vk.current_image, next_image);

	void * data;
	result = vkMapMemory(vk.device, vk.mesh_ubo_host.memory, 0, vk.mesh_ubo_host.size, 0, &data);
	if(result != VK_SUCCESS)
	{
		log_warning("vkMapMemory = %s", vulkan_result(result));
	}

	mat4x4 proj = mat4x4_identity();
	proj = mat4x4_perspective(1, 30, 1, (float)vid_height / (float)vid_width);
//	proj = mat4x4_orthographic(0.1, 30, 1, (float)vid_height / (float)vid_width);

	mat4x4 m = mat4x4_identity();


	m = mat4x4_rot_y(current_time);		// rotate the bunny
	m = mul(m, mat4x4_translate_float(-0.5, -0.5, -0.5)); // around it's own origin
	m = mul(mat4x4_translate_float( 0, 0, -2), m);	// move it 2 metres infront of the origin
//	m = mul( mat4x4_rot_z(3.141), m);

	struct MESH_UNIFORM_BUFFER *ubo = data;

	ubo->modelview = m;
	ubo->projection = proj;
	ubo->time = current_time;

	vkUnmapMemory(vk.device, vk.mesh_ubo_host.memory);

	VkPipelineStageFlags vkflags = VK_PIPELINE_STAGE_TRANSFER_BIT;
	VkSubmitInfo submit_info = {
		VK_STRUCTURE_TYPE_SUBMIT_INFO,		// VkStructureType                sType;
		NULL,					// const void*                    pNext;
		1,					// uint32_t                       waitSemaphoreCount;
		&vk.sc_semaphore[vk.current_image],	// const VkSemaphore*             pWaitSemaphores;
		&vkflags,				// const VkPipelineStageFlags*    pWaitDstStageMask;
		1,					// uint32_t                       commandBufferCount;
		&vk.sc_commandbuffer[next_image],	// const VkCommandBuffer*         pCommandBuffers;
		1,					// uint32_t                       signalSemaphoreCount;
		&vk.sc_semaphore[next_image]		// const VkSemaphore*             pSignalSemaphores;
	};
	result = vkQueueSubmit(vk.queue, 1, &submit_info, VK_NULL_HANDLE);
	if(result != VK_SUCCESS)
	{
		log_warning("vkQueueSubmit = %s", vulkan_result(result));
	}

	VkPresentInfoKHR present_info = {
		VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,	// VkStructureType          sType;
		NULL,					// const void*              pNext;
		1,					// uint32_t                 waitSemaphoreCount;
		&vk.sc_semaphore[next_image],		// const VkSemaphore*       pWaitSemaphores;
		1,					// uint32_t                 swapchainCount;
		&vk.swapchain,				// const VkSwapchainKHR*    pSwapchains;
		&next_image,				// const uint32_t*          pImageIndices;
		NULL					// VkResult*                pResults;
	};
	result = vkQueuePresentKHR(vk.queue, &present_info);
	if(result != VK_SUCCESS)
	{
		log_warning("vkQueuePresentKHR = %s", vulkan_result(result));
	}

	vk.current_image = (vk.current_image + 1) % vk.display_buffer_count;
	return 0;
}

