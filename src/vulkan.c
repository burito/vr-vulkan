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
#include "vulkan_helper.h"


#include "vert_spv.h"
#include "frag_spv.h"

extern int vid_width;
extern int vid_height;

// required by MacOS
//VkFormat pixel_format = VK_FORMAT_B8G8R8A8_SRGB;
VkFormat pixel_format = VK_FORMAT_B8G8R8A8_UNORM;
//VkFormat pixel_format = VK_FORMAT_R16G16B16A16_SFLOAT;
//VkFormat pixel_format = VK_FORMAT_R64G64B64A64_UINT;

static VkDevice device;
static VkSwapchainKHR swapchain;
static VkSemaphore sem_begin;
static VkSemaphore sem_end;
static uint32_t display_buffer_count;
static VkQueue queue;
static VkDeviceMemory ubo_client;
static VkCommandBuffer command_buffers[2];




int vulkan_init(void)
{
	VkResult result;
	/* Vulkan Initialisation is here! */
	VkInstanceCreateInfo vkici = {
		VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,	// VkStructureType             sType;
		NULL,					// const void*                 pNext;
		0,					// VkInstanceCreateFlags       flags;
		0,					// const VkApplicationInfo*    pApplicationInfo;
		0,					// uint32_t                    enabledLayerCount;
		0,					// const char* const*          ppEnabledLayerNames;
		vulkan_extention_count,			// uint32_t                    enabledExtensionCount;
		vulkan_extension_strings		// const char* const*          ppEnabledExtensionNames;
	};

	VkInstance vki;
	result = vkCreateInstance(&vkici, 0, &vki);
	if( result != VK_SUCCESS )
	{
		log_warning("vkCreateInstance = %s", vulkan_result(result));
	}
	unsigned int device_count = 0;
	result = vkEnumeratePhysicalDevices(vki, &device_count, NULL);
	if( result != VK_SUCCESS )
	{
		log_warning("vkEnumeratePhysicalDevices1 = %s", vulkan_result(result));
	}
	log_info("vkEnumeratePhysicalDevices = %d", device_count);
	VkPhysicalDevice *vkpd;
	vkpd = malloc(sizeof(VkPhysicalDevice) * device_count);
	result = vkEnumeratePhysicalDevices(vki, &device_count, vkpd);
	if( result != VK_SUCCESS )
	{
		log_warning("vkEnumeratePhysicalDevices2 = %s", vulkan_result(result));
	}

	int desired_device = 0;
/*
	for(int i=0; i<device_count; i++)
	{

	}
*/

//	vkGetPhysicalDeviceSurfaceSupportKHR(vkpd, )

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
	result = vkCreateDevice(vkpd[desired_device], &vkdci, 0, &device);
	if( result != VK_SUCCESS )
	{
		log_warning("vkCreateDevice = %s", vulkan_result(result));
	}

	VkSurfaceKHR vks;

#ifdef VK_USE_PLATFORM_XCB_KHR
	VkXcbSurfaceCreateInfoKHR vksi = {
		VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,	// VkStructureType               sType;
		NULL,						// const void*                   pNext;
		0,						// VkXcbSurfaceCreateFlagsKHR    flags;
		xcb,						// xcb_connection_t*             connection;
		window						// xcb_window_t                  window;
	};
	vkCreateXcbSurfaceKHR(vki, &vksi, 0, &vks);
	log_debug("vkCreateXcbSurfaceKHR");
#endif

#ifdef VK_USE_PLATFORM_XLIB_KHR
	VkXlibSurfaceCreateInfoKHR vksi = {
		VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,	// VkStructureType                sType;
		NULL,						// const void*                    pNext;
		0,						// VkXlibSurfaceCreateFlagsKHR    flags;
		display,					// Display*                       dpy;
		window						// Window                         window;
	};
	vkCreateXlibSurfaceKHR(vki, &vksi, 0, &vks);
	log_debug("vkCreateXlibSurfaceKHR");
	XFlush(display);
#endif

#ifdef VK_USE_PLATFORM_WIN32_KHR
	VkWin32SurfaceCreateInfoKHR vksi = {
		VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,// VkStructureType                 sType;
		NULL,						// const void*                     pNext;
		0,						// VkWin32SurfaceCreateFlagsKHR    flags;
		hInst,						// HINSTANCE                       hinstance;
		hWnd						// HWND                            hwnd;
	};
	vkCreateWin32SurfaceKHR(vki, &vksi, 0, &vks);
	log_debug("vkCreateWin32SurfaceKHR");
#endif

#ifdef VK_USE_PLATFORM_MACOS_MVK
	VkMacOSSurfaceCreateInfoMVK vksi = {
		VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK,// VkStructureType                 sType;
		NULL,						// const void*                     pNext;
		0,						// VkMacOSSurfaceCreateFlagsMVK    flags;
		pView						// const void*                     pView;
	};
	result = vkCreateMacOSSurfaceMVK(vki, &vksi, 0, &vks);
	if( result != VK_SUCCESS )
	{
		log_warning("vkCreateMacOSSurfaceMVK = %s", vulkan_result(result));
	}
#endif
	VkBool32 present_supported = VK_FALSE;
	result = vkGetPhysicalDeviceSurfaceSupportKHR(vkpd[desired_device], desired_queuefamily, vks, &present_supported);
	if( result != VK_SUCCESS )
	{
		log_warning("vkGetPhysicalDeviceSurfaceSupportKHR = %s", vulkan_result(result));
	}
	log_info("vkGetPhysicalDeviceSurfaceSupportKHR(%d) = %s", desired_queuefamily, present_supported?"VK_TRUE":"VK_FALSE");

	uint32_t surface_format_count = 0;
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(vkpd[desired_device], vks, &surface_format_count, NULL);
	if( result != VK_SUCCESS )
	{
		log_warning("vkGetPhysicalDeviceSurfaceFormatsKHR = %s", vulkan_result(result));
	}
	log_info("vkGetPhysicalDeviceSurfaceFormatsKHR = %d", surface_format_count);

	VkSurfaceFormatKHR * surface_formats = malloc(sizeof(VkSurfaceFormatKHR) * surface_format_count);
	if( ! surface_formats )
	{
		log_fatal("malloc(surface_formats)");
		return 1;
	}
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(vkpd[desired_device], vks, &surface_format_count, surface_formats);
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
		case VK_FORMAT_B8G8R8A8_UNORM:
			desired_format = i;
			pixel_format = surface_formats[i].format;
			break;
		default:
			continue;
		}
	}
	if(desired_format == -1)
	{
		log_fatal("Could not find a desired surface format");
		return 1;
	}

	vkGetDeviceQueue(device, desired_queuefamily, 0, &queue);
	log_debug("vkGetDeviceQueue");

	VkSemaphoreCreateInfo vksemcrinf = {
		VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,	// VkStructureType           sType;
		NULL,						// const void*               pNext;
		0						// VkSemaphoreCreateFlags    flags;
	};
	result = vkCreateSemaphore(device, &vksemcrinf, 0, &sem_begin);
	if( result != VK_SUCCESS )
	{
		log_warning("vkCreateSemaphore1 = %s", vulkan_result(result));
	}
	result = vkCreateSemaphore(device, &vksemcrinf, 0, &sem_end);
	if( result != VK_SUCCESS )
	{
		log_warning("vkCreateSemaphore2 = %s", vulkan_result(result));
	}

	// create the swapchain
	VkExtent2D vk_extent = {vid_width, vid_height};
	VkSwapchainCreateInfoKHR vkswapchaincrinf = {
		VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,	// VkStructureType                  sType;
		NULL,						// const void*                      pNext;
		0,						// VkSwapchainCreateFlagsKHR        flags;
		vks,						// VkSurfaceKHR                     surface;
		2,						// uint32_t                         minImageCount;
		surface_formats[desired_format].format,		// VkFormat                         imageFormat;
		surface_formats[desired_format].colorSpace,	// VkColorSpaceKHR                  imageColorSpace;
		vk_extent,					// VkExtent2D                       imageExtent;
		1,						// uint32_t                         imageArrayLayers;
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,		// VkImageUsageFlags                imageUsage;
		VK_SHARING_MODE_EXCLUSIVE,			// VkSharingMode                    imageSharingMode;
		0,						// uint32_t                         queueFamilyIndexCount;
		NULL,						// const uint32_t*                  pQueueFamilyIndices;
		VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,		// VkSurfaceTransformFlagBitsKHR    preTransform;
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,		// VkCompositeAlphaFlagBitsKHR      compositeAlpha;
		VK_PRESENT_MODE_FIFO_KHR,			// VkPresentModeKHR                 presentMode;
		VK_TRUE,					// VkBool32                         clipped;
		NULL						// VkSwapchainKHR                   oldSwapchain;
	};
	result = vkCreateSwapchainKHR(device, &vkswapchaincrinf, 0, &swapchain);
	if( result != VK_SUCCESS )
	{
		log_warning("vkCreateSwapchainKHR = %s", vulkan_result(result));
	}

	display_buffer_count = 2;
	VkImage vkimg[2];
	result = vkGetSwapchainImagesKHR(device, swapchain, &display_buffer_count, vkimg);
	if( result != VK_SUCCESS )
	{
		log_warning("vkGetSwapchainImagesKHR = %s", vulkan_result(result));
	}


	// create renderpass
	VkAttachmentDescription attach_desc[] = {{
		0,					// VkAttachmentDescriptionFlags    flags;
		pixel_format,				// VkFormat                        format;
		VK_SAMPLE_COUNT_1_BIT,			// VkSampleCountFlagBits           samples;
		VK_ATTACHMENT_LOAD_OP_CLEAR,		// VkAttachmentLoadOp              loadOp;
		VK_ATTACHMENT_STORE_OP_STORE,		// VkAttachmentStoreOp             storeOp;
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,	// VkAttachmentLoadOp              stencilLoadOp;
		VK_ATTACHMENT_STORE_OP_DONT_CARE,	// VkAttachmentStoreOp             stencilStoreOp;
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,	// VkImageLayout                   initialLayout;
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR		// VkImageLayout                   finalLayout;
	}};

	VkAttachmentReference color_attach_refs[] = {{
		0,						// uint32_t         attachment;
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,	// VkImageLayout    layout;
	}};

	VkSubpassDescription subpass_desc[] = {{
		0,					// VkSubpassDescriptionFlags       flags;
		VK_PIPELINE_BIND_POINT_GRAPHICS,	// VkPipelineBindPoint             pipelineBindPoint;
		0,					// uint32_t                        inputAttachmentCount;
		NULL,					// const VkAttachmentReference*    pInputAttachments;
		1,					// uint32_t                        colorAttachmentCount;
		color_attach_refs,			// const VkAttachmentReference*    pColorAttachments;
		NULL,					// const VkAttachmentReference*    pResolveAttachments;
		NULL,					// const VkAttachmentReference*    pDepthStencilAttachment;
		0,					// uint32_t                        preserveAttachmentCount;
		NULL					// const uint32_t*                 pPreserveAttachments;
	}};

	VkRenderPassCreateInfo render_pass_crinf = {
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,	// VkStructureType                   sType;
		NULL,						// const void*                       pNext;
		0,						// VkRenderPassCreateFlags           flags;
		1,						// uint32_t                          attachmentCount;
		attach_desc,					// const VkAttachmentDescription*    pAttachments;
		1,						// uint32_t                          subpassCount;
		subpass_desc,					// const VkSubpassDescription*       pSubpasses;
		0,						// uint32_t                          dependencyCount;
		NULL						// const VkSubpassDependency*        pDependencies;
	};

	VkRenderPass vkrp;
	result = vkCreateRenderPass( device, &render_pass_crinf, NULL, &vkrp );
	if( result != VK_SUCCESS )
	{
		log_warning("vkCreateRenderPass = %s", vulkan_result(result));
	}

	VkImageView img_view[2];
	VkFramebuffer vkfb[2];


	// create the framebuffers
	for( int i=0; i<display_buffer_count; i++)
	{
		VkImageViewCreateInfo image_view_crinf = {
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,	// VkStructureType            sType;
			NULL,						// const void*                pNext;
			0,						// VkImageViewCreateFlags     flags;
			vkimg[i],					// VkImage                    image;
			VK_IMAGE_VIEW_TYPE_2D,				// VkImageViewType            viewType;
			pixel_format,					// VkFormat                   format;
			{	VK_COMPONENT_SWIZZLE_IDENTITY,		// VkComponentMapping         components;
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY },	// VkImageSubresourceRange    subresourceRange;
			{ VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1 }
		};

		result = vkCreateImageView( device, &image_view_crinf, NULL, &img_view[i]);
		if( result != VK_SUCCESS )
		{
			log_warning("vkCreateImageView(%d) = %d", i+1, result);
		}

		VkFramebufferCreateInfo fb_crinf = {
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			NULL,
			0,
			vkrp, 1, &img_view[i], vid_width, vid_height, 1};

		result = vkCreateFramebuffer( device, &fb_crinf, NULL, &vkfb[i]);
		if( result != VK_SUCCESS )
		{
			log_warning("vkCreateFramebuffer(%d) = %d", i+1, result);
		}
	}

	// create the commandbuffers
	VkCommandPoolCreateInfo vk_cmdpoolcrinf = {
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,	// VkStructureType             sType;
		NULL,						// const void*                 pNext;
		0,						// VkCommandPoolCreateFlags    flags;
		0						// uint32_t                    queueFamilyIndex;
	};
	VkCommandPool vkpool;
	result = vkCreateCommandPool(device, &vk_cmdpoolcrinf, 0, &vkpool);
	if( result != VK_SUCCESS )
	{
		log_warning("vkCreateCommandPool = %s", vulkan_result(result));
	}

	VkCommandBufferAllocateInfo vk_cmdbufallocinf = {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,	// VkStructureType         sType;
		NULL,						// const void*             pNext;
		vkpool,						// VkCommandPool           commandPool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,		// VkCommandBufferLevel    level;
		2						// uint32_t                commandBufferCount;
	};
	result = vkAllocateCommandBuffers(device, &vk_cmdbufallocinf, command_buffers);
	if( result != VK_SUCCESS )
	{
		log_warning("vkAllocateCommandBuffers = %s", vulkan_result(result));
	}

	// create the shaders
	VkShaderModule shader_module_vert;
	VkShaderModule shader_module_frag;

	VkShaderModuleCreateInfo shader_vert_crinf = {
		VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,	// VkStructureType              sType;
		NULL,						// const void*                  pNext;
		0,						// VkShaderModuleCreateFlags    flags;
		build_vert_spv_len,					// size_t                       codeSize;
		(const uint32_t*)build_vert_spv			// const uint32_t*              pCode;
	};

	VkShaderModuleCreateInfo shader_frag_crinf = {
		VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,	// VkStructureType              sType;
		NULL,						// const void*                  pNext;
		0,						// VkShaderModuleCreateFlags    flags;
		build_frag_spv_len,					// size_t                       codeSize;
		(const uint32_t*)build_frag_spv			// const uint32_t*              pCode;
	};

	vkCreateShaderModule(device, &shader_vert_crinf, NULL, &shader_module_vert);
	if( result != VK_SUCCESS )
	{
		log_warning("vkCreateShaderModule(vertex) = %s", vulkan_result(result));
	}
	vkCreateShaderModule(device, &shader_frag_crinf, NULL, &shader_module_frag);
	if( result != VK_SUCCESS )
	{
		log_warning("vkCreateShaderModule(fragment) = %s", vulkan_result(result));
	}

	int ubo_buffer_size = sizeof(float);

	VkBufferCreateInfo ubo_buffer_client_crinf = {
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,	// VkStructureType        sType;
		NULL,					// const void*            pNext;
		0,					// VkBufferCreateFlags    flags;
		ubo_buffer_size,			// VkDeviceSize           size;
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |	// VkBufferUsageFlags     usage;
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_SHARING_MODE_EXCLUSIVE,		// VkSharingMode          sharingMode;
		0,					// uint32_t               queueFamilyIndexCount;
		NULL					// const uint32_t*        pQueueFamilyIndices;
	};

	VkBufferCreateInfo ubo_buffer_host_crinf = {
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,	// VkStructureType        sType;
		NULL,					// const void*            pNext;
		0,					// VkBufferCreateFlags    flags;
		ubo_buffer_size,			// VkDeviceSize           size;
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |	// VkBufferUsageFlags     usage;
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_SHARING_MODE_EXCLUSIVE,		// VkSharingMode          sharingMode;
		0,					// uint32_t               queueFamilyIndexCount;
		NULL					// const uint32_t*        pQueueFamilyIndices;
	};


	VkBuffer ubo_buffer_client;    // Client side buffer
	result = vkCreateBuffer(device, &ubo_buffer_client_crinf, NULL, &ubo_buffer_client);
	if( result != VK_SUCCESS )
	{
		log_warning("vkCreateBuffer = %s", vulkan_result(result));
	}
	int wanted = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
	VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	int client_memory_type = 0;
	VkMemoryRequirements vk_memreq;
	vkGetBufferMemoryRequirements(device, ubo_buffer_client, &vk_memreq);
	VkPhysicalDeviceMemoryProperties vkpdmp;
	vkGetPhysicalDeviceMemoryProperties(vkpd[desired_device], &vkpdmp);
	for(int i=0; i<vkpdmp.memoryTypeCount; i++)
	{
		if( vk_memreq.memoryTypeBits & (1<<i) )
		if( (vkpdmp.memoryTypes[i].propertyFlags & wanted) == wanted )
		{
			log_debug("vkGetPhysicalDeviceMemoryProperties(client) = %d", i);
			client_memory_type = i;
			break;
		}
	}

	// create the uniform buffer
	VkMemoryAllocateInfo ubo_buffer_client_alloc_info = {
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,	// VkStructureType    sType;
		NULL,					// const void*        pNext;
		ubo_buffer_size,			// VkDeviceSize       allocationSize;
		client_memory_type			// uint32_t           memoryTypeIndex;
	};
	result = vkAllocateMemory(device, &ubo_buffer_client_alloc_info, NULL, &ubo_client);
	if( result != VK_SUCCESS )
	{
		log_warning("vkAllocateMemory = %s", vulkan_result(result));
	}
	result = vkBindBufferMemory(device, ubo_buffer_client, ubo_client, 0);
	if( result != VK_SUCCESS )
	{
		log_warning("vkBindBufferMemory = %s", vulkan_result(result));
	}

	VkBuffer ubo_buffer_host;    // host side buffer
	result = vkCreateBuffer(device, &ubo_buffer_host_crinf, NULL, &ubo_buffer_host);
	if( result != VK_SUCCESS )
	{
		log_warning("vkCreateBuffer = %s", vulkan_result(result));
	}

	// for the host buffer
	wanted = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	vkGetBufferMemoryRequirements(device, ubo_buffer_host, &vk_memreq);
	int host_memory_type = 0;
	for(int i=0; i<vkpdmp.memoryTypeCount; i++)
	{
		if( vk_memreq.memoryTypeBits & (1<<i) )
		if( (vkpdmp.memoryTypes[i].propertyFlags & wanted) == wanted )
		{
			log_debug("vkGetPhysicalDeviceMemoryProperties(host) = %d", i);
			host_memory_type = i;
			break;
		}
	}

	VkMemoryAllocateInfo ubo_buffer_host_alloc_info = {
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,	// VkStructureType    sType;
		NULL,					// const void*        pNext;
		ubo_buffer_size,			// VkDeviceSize       allocationSize;
		host_memory_type			// uint32_t           memoryTypeIndex;
	};
	VkDeviceMemory ubo_buffer_host_mem;
	result = vkAllocateMemory(device, &ubo_buffer_host_alloc_info, NULL, &ubo_buffer_host_mem);
	if( result != VK_SUCCESS )
	{
		log_warning("vkAllocateMemory = %s", vulkan_result(result));
	}
	result = vkBindBufferMemory(device, ubo_buffer_host, ubo_buffer_host_mem, 0);
	if( result != VK_SUCCESS )
	{
		log_warning("vkBindBufferMemory = %s", vulkan_result(result));
	}


	VkDescriptorPoolSize desc_pool_size = {
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,	// VkDescriptorType    type;
		1					// uint32_t            descriptorCount;
	};

	VkDescriptorPoolCreateInfo desc_pool_crinf = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,	// VkStructureType                sType;
		NULL,						// const void*                    pNext;
		0,						// VkDescriptorPoolCreateFlags    flags;
		1,						// uint32_t                       maxSets;
		1,						// uint32_t                       poolSizeCount;
		&desc_pool_size					// const VkDescriptorPoolSize*    pPoolSizes;
	};

	VkDescriptorPool desc_pool;
	result = vkCreateDescriptorPool(device, &desc_pool_crinf, NULL, &desc_pool);
	if( result != VK_SUCCESS )
	{
		log_warning("vkCreateDescriptorPool = %s", vulkan_result(result));
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

	VkDescriptorSetLayout vk_ubo_layout[1];
	result = vkCreateDescriptorSetLayout(device, &descriptor_layout_crinf, NULL, &vk_ubo_layout[0]);
	if( result != VK_SUCCESS )
	{
		log_warning("vkCreateDescriptorSetLayout = %s", vulkan_result(result));
	}

	VkDescriptorSetAllocateInfo desc_set_alloc_info = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,	// VkStructureType                 sType;
		NULL,						// const void*                     pNext;
		desc_pool,					// VkDescriptorPool                descriptorPool;
		1,						// uint32_t                        descriptorSetCount;
		vk_ubo_layout					// const VkDescriptorSetLayout*    pSetLayouts;
	};

	VkDescriptorSet desc_set;
	result = vkAllocateDescriptorSets(device, &desc_set_alloc_info, &desc_set);
	if( result != VK_SUCCESS )
	{
		log_warning("vkAllocateDescriptorSets = %s", vulkan_result(result));
	}

	VkDescriptorBufferInfo desc_buf_info = {ubo_buffer_host,0,sizeof(float)};
	VkWriteDescriptorSet desc_write_set = {
		VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,	// VkStructureType                  sType;
		NULL,					// const void*                      pNext;
		desc_set,				// VkDescriptorSet                  dstSet;
		0,					// uint32_t                         dstBinding;
		0,					// uint32_t                         dstArrayElement;
		1,					// uint32_t                         descriptorCount;
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,	// VkDescriptorType                 descriptorType;
		NULL,					// const VkDescriptorImageInfo*     pImageInfo;
		&desc_buf_info,				// const VkDescriptorBufferInfo*    pBufferInfo;
		NULL					// const VkBufferView*              pTexelBufferView;
	};
	vkUpdateDescriptorSets(device, 1, &desc_write_set, 0, NULL);
	log_debug("vkUpdateDescriptorSets");

// create the pipeline
	// shader and vertex input
	VkPipelineShaderStageCreateInfo shader_stage_crinf[2] = {
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,	// VkStructureType                     sType;
			NULL,							// const void*                         pNext;
			0,							// VkPipelineShaderStageCreateFlags    flags;
			VK_SHADER_STAGE_VERTEX_BIT,				// VkShaderStageFlagBits               stage;
			shader_module_vert,					// VkShaderModule                      module;
			"main",							// const char*                         pName;
			NULL							// const VkSpecializationInfo*         pSpecializationInfo;
		},
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,	// VkStructureType                     sType;
			NULL,							// const void*                         pNext;
			0,							// VkPipelineShaderStageCreateFlags    flags;
			VK_SHADER_STAGE_FRAGMENT_BIT,				// VkShaderStageFlagBits               stage;
			shader_module_frag,					// VkShaderModule                      module;
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
		vk_ubo_layout,					// const VkDescriptorSetLayout*    pSetLayouts;
		0,						// uint32_t                        pushConstantRangeCount;
		NULL						// const VkPushConstantRange*      pPushConstantRanges;
	};
	VkPipelineLayout pipeline_layout;
	result = vkCreatePipelineLayout( device, &layout_crinf, NULL, &pipeline_layout);
	if( result != VK_SUCCESS )
	{
		log_warning("vkCreatePipelineLayout = %s", vulkan_result(result));
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
		NULL,							// const VkPipelineDepthStencilStateCreateInfo*     pDepthStencilState;
		&color_blend_state_crinf,				// const VkPipelineColorBlendStateCreateInfo*       pColorBlendState;
		NULL,							// const VkPipelineDynamicStateCreateInfo*          pDynamicState;
		pipeline_layout,					// VkPipelineLayout                                 layout;
		vkrp,							// VkRenderPass                                     renderPass;
		0,							// uint32_t                                         subpass;
		VK_NULL_HANDLE,						// VkPipeline                                       basePipelineHandle;
		-1							// int32_t                                          basePipelineIndex;
	};
	VkPipeline vkpipe;
	result = vkCreateGraphicsPipelines( device, VK_NULL_HANDLE, 1, &pipeline_crinf, NULL, &vkpipe);
	if( result != VK_SUCCESS )
	{
		log_warning("vkCreateGraphicsPipelines = %s", vulkan_result(result));
	}

	// more commandbuffer
	VkCommandBufferBeginInfo vk_cmdbegin = {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// VkStructureType                          sType;
		NULL,						// const void*                              pNext;
		VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,	// VkCommandBufferUsageFlags                flags;
		NULL						// const VkCommandBufferInheritanceInfo*    pInheritanceInfo;
	};

	VkClearValue clear_color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	VkImageSubresourceRange image_subresource_range = {
		VK_IMAGE_ASPECT_COLOR_BIT,	// VkImageAspectFlags    aspectMask;
		0,				// uint32_t              baseMipLevel;
		1,				// uint32_t              levelCount;
		0,				// uint32_t              baseArrayLayer;
		1				// uint32_t              layerCount;
	};

	log_debug("display_buffer_count = %d", display_buffer_count);

	for(int i=0; i<display_buffer_count; i++)
	{
		VkImageMemoryBarrier vkb_present_to_draw = {
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,	// VkStructureType            sType;
			NULL,					// const void*                pNext;
			VK_ACCESS_MEMORY_READ_BIT,		// VkAccessFlags              srcAccessMask;
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,	// VkAccessFlags              dstAccessMask;
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,	// VkImageLayout              oldLayout;
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,	// VkImageLayout              newLayout;
			0,					// uint32_t                   srcQueueFamilyIndex;
			0,					// uint32_t                   dstQueueFamilyIndex;
			vkimg[i],				// VkImage                    image;
			image_subresource_range			// VkImageSubresourceRange    subresourceRange;
		};

		VkRenderPassBeginInfo render_pass_begin_info = {
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,	// VkStructureType        sType;
			NULL,						// const void*            pNext;
			vkrp,						// VkRenderPass           renderPass;
			vkfb[i],					// VkFramebuffer          framebuffer;
			{{0,0},{vid_width,vid_height}},				// VkRect2D               renderArea;
			1,						// uint32_t               clearValueCount;
			&clear_color					// const VkClearValue*    pClearValues;
		};

		VkImageMemoryBarrier vkb_draw_to_present = {
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,	// VkStructureType            sType;
			NULL,					// const void*                pNext;
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,	// VkAccessFlags              srcAccessMask;
			VK_ACCESS_MEMORY_READ_BIT,		// VkAccessFlags              dstAccessMask;
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,	// VkImageLayout              oldLayout;
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,	// VkImageLayout              newLayout;
			0,					// uint32_t                   srcQueueFamilyIndex;
			0,					// uint32_t                   dstQueueFamilyIndex;
			vkimg[i],				// VkImage                    image;
			image_subresource_range			// VkImageSubresourceRange    subresourceRange;
		};

		VkBufferCopy buffer_copy[1] = {
			0,		// VkDeviceSize    srcOffset;
			0,		// VkDeviceSize    dstOffset;
			ubo_buffer_size	// VkDeviceSize    size;
		};


		// fill the command buffer with commands
		result = vkBeginCommandBuffer(command_buffers[i],&vk_cmdbegin);
		if( result != VK_SUCCESS ) { log_warning("vkBeginCommandBuffer = %s", vulkan_result(result)); }
		
		vkCmdCopyBuffer( command_buffers[i], ubo_buffer_client, ubo_buffer_host, 1, &buffer_copy[0]);

		vkCmdPipelineBarrier(command_buffers[i],
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			0, 0, NULL, 0, NULL, 1, &vkb_present_to_draw);

		vkCmdBeginRenderPass( command_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline( command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vkpipe);
		vkCmdBindDescriptorSets( command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipeline_layout, 0, 1, &desc_set, 0, NULL);




		vkCmdDraw( command_buffers[i], 4, 1, 0, 0 );

		vkCmdEndRenderPass( command_buffers[i] );

		vkCmdPipelineBarrier(command_buffers[i],VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,0,0,NULL,0,NULL,1,&vkb_draw_to_present);

		result = vkEndCommandBuffer( command_buffers[i] );
		if( result != VK_SUCCESS ) { log_warning("vkEndCommandBuffer = %s", vulkan_result(result)); }
//		log_debug("command buffer %d filled", i+1);
	}
	log_debug("Vulkan Initialisation complete");
	return 0;
}


int vulkan_loop(float current_time)
{
	log_trace("frame time = %f", current_time);
	uint32_t next_image = 0;
	VkResult result;
	result = vkAcquireNextImageKHR(device, swapchain, 10000000, sem_begin, VK_NULL_HANDLE, &next_image);
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
	float * data;
	result = vkMapMemory(device, ubo_client, 0, sizeof(float), 0, (void**)&data);
	if(result != VK_SUCCESS)
	{
		log_warning("vkMapMemory = %s", vulkan_result(result));
	}
	data[0] = current_time;
	vkUnmapMemory(device, ubo_client);

	VkPipelineStageFlags vkflags = VK_PIPELINE_STAGE_TRANSFER_BIT;
	VkSubmitInfo submit_info = {
		VK_STRUCTURE_TYPE_SUBMIT_INFO,		// VkStructureType                sType;
		NULL,					// const void*                    pNext;
		1,					// uint32_t                       waitSemaphoreCount;
		&sem_begin,				// const VkSemaphore*             pWaitSemaphores;
		&vkflags,				// const VkPipelineStageFlags*    pWaitDstStageMask;
		1,					// uint32_t                       commandBufferCount;
		&command_buffers[next_image],		// const VkCommandBuffer*         pCommandBuffers;
		1,					// uint32_t                       signalSemaphoreCount;
		&sem_end				// const VkSemaphore*             pSignalSemaphores;
	};
	result = vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
	if(result != VK_SUCCESS)
	{
		log_warning("vkQueueSubmit = %s", vulkan_result(result));
	}

	VkPresentInfoKHR present_info = {
		VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,	// VkStructureType          sType;
		NULL,					// const void*              pNext;
		1,					// uint32_t                 waitSemaphoreCount;
		&sem_end,				// const VkSemaphore*       pWaitSemaphores;
		1,					// uint32_t                 swapchainCount;
		&swapchain,				// const VkSwapchainKHR*    pSwapchains;
		&next_image,				// const uint32_t*          pImageIndices;
		NULL					// VkResult*                pResults;
	};
	result = vkQueuePresentKHR(queue, &present_info);
	if(result != VK_SUCCESS)
	{
		log_warning("vkQueuePresentKHR = %s", vulkan_result(result));
	}
	return 0;
//	log_trace("frame finished");
}

