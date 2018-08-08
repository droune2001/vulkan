/* This file is part of volk library; see volk.h for version/license details */
#define VK_USE_PLATFORM_WIN32_KHR
#define VK_NO_PROTOTYPES
#include "volk.h"

#ifdef _WIN32
#	include <windows.h>
#else
#	include <dlfcn.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

static void volkGenLoadLoader(void* context, PFN_vkVoidFunction (*load)(void*, const char*));
static void volkGenLoadInstance(void* context, PFN_vkVoidFunction (*load)(void*, const char*));
static void volkGenLoadDevice(void* context, PFN_vkVoidFunction (*load)(void*, const char*));
static void volkGenLoadDeviceTable(struct VolkDeviceTable* table, void* context, PFN_vkVoidFunction (*load)(void*, const char*));

static PFN_vkVoidFunction vkGetInstanceProcAddrStub(void* context, const char* name)
{
	return vkGetInstanceProcAddr((VkInstance)context, name);
}

static PFN_vkVoidFunction vkGetDeviceProcAddrStub(void* context, const char* name)
{
	return vkGetDeviceProcAddr((VkDevice)context, name);
}

VkResult volkInitialize()
{
#ifdef _WIN32
	HMODULE module = LoadLibraryA("vulkan-1.dll");
	if (!module)
		return VK_ERROR_INITIALIZATION_FAILED;

	vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress(module, "vkGetInstanceProcAddr");
#else
	void* module = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
	if (!module)
		module = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
	if (!module)
		return VK_ERROR_INITIALIZATION_FAILED;

	vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(module, "vkGetInstanceProcAddr");
#endif

	volkGenLoadLoader(NULL, vkGetInstanceProcAddrStub);

	return VK_SUCCESS;
}

void volkInitializeCustom(PFN_vkGetInstanceProcAddr handler)
{
	vkGetInstanceProcAddr = handler;

	volkGenLoadLoader(NULL, vkGetInstanceProcAddrStub);
}

uint32_t volkGetInstanceVersion()
{
#if defined(VK_VERSION_1_1)
	uint32_t apiVersion = 0;
	if (vkEnumerateInstanceVersion && vkEnumerateInstanceVersion(&apiVersion) == VK_SUCCESS)
		return apiVersion;
#endif

	if (vkCreateInstance)
		return VK_API_VERSION_1_0;

	return 0;
}

void volkLoadInstance(VkInstance instance)
{
	volkGenLoadInstance(instance, vkGetInstanceProcAddrStub);
	volkGenLoadDevice(instance, vkGetInstanceProcAddrStub);
}

void volkLoadDevice(VkDevice device)
{
	volkGenLoadDevice(device, vkGetDeviceProcAddrStub);
}

void volkLoadDeviceTable(struct VolkDeviceTable* table, VkDevice device)
{
	volkGenLoadDeviceTable(table, device, vkGetDeviceProcAddrStub);
}

static void volkGenLoadLoader(void* context, PFN_vkVoidFunction (*load)(void*, const char*))
{
	/* VOLK_GENERATE_LOAD_LOADER */
#if defined(VK_VERSION_1_0)
	vkCreateInstance = (PFN_vkCreateInstance)load(context, "vkCreateInstance");
	vkEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties)load(context, "vkEnumerateInstanceExtensionProperties");
	vkEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties)load(context, "vkEnumerateInstanceLayerProperties");
#endif /* defined(VK_VERSION_1_0) */
#if defined(VK_VERSION_1_1)
	vkEnumerateInstanceVersion = (PFN_vkEnumerateInstanceVersion)load(context, "vkEnumerateInstanceVersion");
#endif /* defined(VK_VERSION_1_1) */
	/* VOLK_GENERATE_LOAD_LOADER */
}

static void volkGenLoadInstance(void* context, PFN_vkVoidFunction (*load)(void*, const char*))
{
	/* VOLK_GENERATE_LOAD_INSTANCE */
#if defined(VK_VERSION_1_0)
	vkCreateDevice = (PFN_vkCreateDevice)load(context, "vkCreateDevice");
	vkDestroyInstance = (PFN_vkDestroyInstance)load(context, "vkDestroyInstance");
	vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties)load(context, "vkEnumerateDeviceExtensionProperties");
	vkEnumerateDeviceLayerProperties = (PFN_vkEnumerateDeviceLayerProperties)load(context, "vkEnumerateDeviceLayerProperties");
	vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)load(context, "vkEnumeratePhysicalDevices");
	vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)load(context, "vkGetDeviceProcAddr");
	vkGetPhysicalDeviceFeatures = (PFN_vkGetPhysicalDeviceFeatures)load(context, "vkGetPhysicalDeviceFeatures");
	vkGetPhysicalDeviceFormatProperties = (PFN_vkGetPhysicalDeviceFormatProperties)load(context, "vkGetPhysicalDeviceFormatProperties");
	vkGetPhysicalDeviceImageFormatProperties = (PFN_vkGetPhysicalDeviceImageFormatProperties)load(context, "vkGetPhysicalDeviceImageFormatProperties");
	vkGetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties)load(context, "vkGetPhysicalDeviceMemoryProperties");
	vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)load(context, "vkGetPhysicalDeviceProperties");
	vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)load(context, "vkGetPhysicalDeviceQueueFamilyProperties");
	vkGetPhysicalDeviceSparseImageFormatProperties = (PFN_vkGetPhysicalDeviceSparseImageFormatProperties)load(context, "vkGetPhysicalDeviceSparseImageFormatProperties");
#endif /* defined(VK_VERSION_1_0) */
#if defined(VK_VERSION_1_1)
	vkEnumeratePhysicalDeviceGroups = (PFN_vkEnumeratePhysicalDeviceGroups)load(context, "vkEnumeratePhysicalDeviceGroups");
	vkGetPhysicalDeviceExternalBufferProperties = (PFN_vkGetPhysicalDeviceExternalBufferProperties)load(context, "vkGetPhysicalDeviceExternalBufferProperties");
	vkGetPhysicalDeviceExternalFenceProperties = (PFN_vkGetPhysicalDeviceExternalFenceProperties)load(context, "vkGetPhysicalDeviceExternalFenceProperties");
	vkGetPhysicalDeviceExternalSemaphoreProperties = (PFN_vkGetPhysicalDeviceExternalSemaphoreProperties)load(context, "vkGetPhysicalDeviceExternalSemaphoreProperties");
	vkGetPhysicalDeviceFeatures2 = (PFN_vkGetPhysicalDeviceFeatures2)load(context, "vkGetPhysicalDeviceFeatures2");
	vkGetPhysicalDeviceFormatProperties2 = (PFN_vkGetPhysicalDeviceFormatProperties2)load(context, "vkGetPhysicalDeviceFormatProperties2");
	vkGetPhysicalDeviceImageFormatProperties2 = (PFN_vkGetPhysicalDeviceImageFormatProperties2)load(context, "vkGetPhysicalDeviceImageFormatProperties2");
	vkGetPhysicalDeviceMemoryProperties2 = (PFN_vkGetPhysicalDeviceMemoryProperties2)load(context, "vkGetPhysicalDeviceMemoryProperties2");
	vkGetPhysicalDeviceProperties2 = (PFN_vkGetPhysicalDeviceProperties2)load(context, "vkGetPhysicalDeviceProperties2");
	vkGetPhysicalDeviceQueueFamilyProperties2 = (PFN_vkGetPhysicalDeviceQueueFamilyProperties2)load(context, "vkGetPhysicalDeviceQueueFamilyProperties2");
	vkGetPhysicalDeviceSparseImageFormatProperties2 = (PFN_vkGetPhysicalDeviceSparseImageFormatProperties2)load(context, "vkGetPhysicalDeviceSparseImageFormatProperties2");
#endif /* defined(VK_VERSION_1_1) */
#if defined(VK_EXT_acquire_xlib_display)
	vkAcquireXlibDisplayEXT = (PFN_vkAcquireXlibDisplayEXT)load(context, "vkAcquireXlibDisplayEXT");
	vkGetRandROutputDisplayEXT = (PFN_vkGetRandROutputDisplayEXT)load(context, "vkGetRandROutputDisplayEXT");
#endif /* defined(VK_EXT_acquire_xlib_display) */
#if defined(VK_EXT_debug_report)
	vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)load(context, "vkCreateDebugReportCallbackEXT");
	vkDebugReportMessageEXT = (PFN_vkDebugReportMessageEXT)load(context, "vkDebugReportMessageEXT");
	vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)load(context, "vkDestroyDebugReportCallbackEXT");
#endif /* defined(VK_EXT_debug_report) */
#if defined(VK_EXT_debug_utils)
	vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)load(context, "vkCreateDebugUtilsMessengerEXT");
	vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)load(context, "vkDestroyDebugUtilsMessengerEXT");
	vkSubmitDebugUtilsMessageEXT = (PFN_vkSubmitDebugUtilsMessageEXT)load(context, "vkSubmitDebugUtilsMessageEXT");
#endif /* defined(VK_EXT_debug_utils) */
#if defined(VK_EXT_direct_mode_display)
	vkReleaseDisplayEXT = (PFN_vkReleaseDisplayEXT)load(context, "vkReleaseDisplayEXT");
#endif /* defined(VK_EXT_direct_mode_display) */
#if defined(VK_EXT_display_surface_counter)
	vkGetPhysicalDeviceSurfaceCapabilities2EXT = (PFN_vkGetPhysicalDeviceSurfaceCapabilities2EXT)load(context, "vkGetPhysicalDeviceSurfaceCapabilities2EXT");
#endif /* defined(VK_EXT_display_surface_counter) */
#if defined(VK_EXT_sample_locations)
	vkGetPhysicalDeviceMultisamplePropertiesEXT = (PFN_vkGetPhysicalDeviceMultisamplePropertiesEXT)load(context, "vkGetPhysicalDeviceMultisamplePropertiesEXT");
#endif /* defined(VK_EXT_sample_locations) */
#if defined(VK_KHR_android_surface)
	vkCreateAndroidSurfaceKHR = (PFN_vkCreateAndroidSurfaceKHR)load(context, "vkCreateAndroidSurfaceKHR");
#endif /* defined(VK_KHR_android_surface) */
#if defined(VK_KHR_device_group_creation)
	vkEnumeratePhysicalDeviceGroupsKHR = (PFN_vkEnumeratePhysicalDeviceGroupsKHR)load(context, "vkEnumeratePhysicalDeviceGroupsKHR");
#endif /* defined(VK_KHR_device_group_creation) */
#if defined(VK_KHR_display)
	vkCreateDisplayModeKHR = (PFN_vkCreateDisplayModeKHR)load(context, "vkCreateDisplayModeKHR");
	vkCreateDisplayPlaneSurfaceKHR = (PFN_vkCreateDisplayPlaneSurfaceKHR)load(context, "vkCreateDisplayPlaneSurfaceKHR");
	vkGetDisplayModePropertiesKHR = (PFN_vkGetDisplayModePropertiesKHR)load(context, "vkGetDisplayModePropertiesKHR");
	vkGetDisplayPlaneCapabilitiesKHR = (PFN_vkGetDisplayPlaneCapabilitiesKHR)load(context, "vkGetDisplayPlaneCapabilitiesKHR");
	vkGetDisplayPlaneSupportedDisplaysKHR = (PFN_vkGetDisplayPlaneSupportedDisplaysKHR)load(context, "vkGetDisplayPlaneSupportedDisplaysKHR");
	vkGetPhysicalDeviceDisplayPlanePropertiesKHR = (PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR)load(context, "vkGetPhysicalDeviceDisplayPlanePropertiesKHR");
	vkGetPhysicalDeviceDisplayPropertiesKHR = (PFN_vkGetPhysicalDeviceDisplayPropertiesKHR)load(context, "vkGetPhysicalDeviceDisplayPropertiesKHR");
#endif /* defined(VK_KHR_display) */
#if defined(VK_KHR_external_fence_capabilities)
	vkGetPhysicalDeviceExternalFencePropertiesKHR = (PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR)load(context, "vkGetPhysicalDeviceExternalFencePropertiesKHR");
#endif /* defined(VK_KHR_external_fence_capabilities) */
#if defined(VK_KHR_external_memory_capabilities)
	vkGetPhysicalDeviceExternalBufferPropertiesKHR = (PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR)load(context, "vkGetPhysicalDeviceExternalBufferPropertiesKHR");
#endif /* defined(VK_KHR_external_memory_capabilities) */
#if defined(VK_KHR_external_semaphore_capabilities)
	vkGetPhysicalDeviceExternalSemaphorePropertiesKHR = (PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR)load(context, "vkGetPhysicalDeviceExternalSemaphorePropertiesKHR");
#endif /* defined(VK_KHR_external_semaphore_capabilities) */
#if defined(VK_KHR_get_display_properties2)
	vkGetDisplayModeProperties2KHR = (PFN_vkGetDisplayModeProperties2KHR)load(context, "vkGetDisplayModeProperties2KHR");
	vkGetDisplayPlaneCapabilities2KHR = (PFN_vkGetDisplayPlaneCapabilities2KHR)load(context, "vkGetDisplayPlaneCapabilities2KHR");
	vkGetPhysicalDeviceDisplayPlaneProperties2KHR = (PFN_vkGetPhysicalDeviceDisplayPlaneProperties2KHR)load(context, "vkGetPhysicalDeviceDisplayPlaneProperties2KHR");
	vkGetPhysicalDeviceDisplayProperties2KHR = (PFN_vkGetPhysicalDeviceDisplayProperties2KHR)load(context, "vkGetPhysicalDeviceDisplayProperties2KHR");
#endif /* defined(VK_KHR_get_display_properties2) */
#if defined(VK_KHR_get_physical_device_properties2)
	vkGetPhysicalDeviceFeatures2KHR = (PFN_vkGetPhysicalDeviceFeatures2KHR)load(context, "vkGetPhysicalDeviceFeatures2KHR");
	vkGetPhysicalDeviceFormatProperties2KHR = (PFN_vkGetPhysicalDeviceFormatProperties2KHR)load(context, "vkGetPhysicalDeviceFormatProperties2KHR");
	vkGetPhysicalDeviceImageFormatProperties2KHR = (PFN_vkGetPhysicalDeviceImageFormatProperties2KHR)load(context, "vkGetPhysicalDeviceImageFormatProperties2KHR");
	vkGetPhysicalDeviceMemoryProperties2KHR = (PFN_vkGetPhysicalDeviceMemoryProperties2KHR)load(context, "vkGetPhysicalDeviceMemoryProperties2KHR");
	vkGetPhysicalDeviceProperties2KHR = (PFN_vkGetPhysicalDeviceProperties2KHR)load(context, "vkGetPhysicalDeviceProperties2KHR");
	vkGetPhysicalDeviceQueueFamilyProperties2KHR = (PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR)load(context, "vkGetPhysicalDeviceQueueFamilyProperties2KHR");
	vkGetPhysicalDeviceSparseImageFormatProperties2KHR = (PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR)load(context, "vkGetPhysicalDeviceSparseImageFormatProperties2KHR");
#endif /* defined(VK_KHR_get_physical_device_properties2) */
#if defined(VK_KHR_get_surface_capabilities2)
	vkGetPhysicalDeviceSurfaceCapabilities2KHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR)load(context, "vkGetPhysicalDeviceSurfaceCapabilities2KHR");
	vkGetPhysicalDeviceSurfaceFormats2KHR = (PFN_vkGetPhysicalDeviceSurfaceFormats2KHR)load(context, "vkGetPhysicalDeviceSurfaceFormats2KHR");
#endif /* defined(VK_KHR_get_surface_capabilities2) */
#if defined(VK_KHR_mir_surface)
	vkCreateMirSurfaceKHR = (PFN_vkCreateMirSurfaceKHR)load(context, "vkCreateMirSurfaceKHR");
	vkGetPhysicalDeviceMirPresentationSupportKHR = (PFN_vkGetPhysicalDeviceMirPresentationSupportKHR)load(context, "vkGetPhysicalDeviceMirPresentationSupportKHR");
#endif /* defined(VK_KHR_mir_surface) */
#if defined(VK_KHR_surface)
	vkDestroySurfaceKHR = (PFN_vkDestroySurfaceKHR)load(context, "vkDestroySurfaceKHR");
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)load(context, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
	vkGetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)load(context, "vkGetPhysicalDeviceSurfaceFormatsKHR");
	vkGetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)load(context, "vkGetPhysicalDeviceSurfacePresentModesKHR");
	vkGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)load(context, "vkGetPhysicalDeviceSurfaceSupportKHR");
#endif /* defined(VK_KHR_surface) */
#if defined(VK_KHR_wayland_surface)
	vkCreateWaylandSurfaceKHR = (PFN_vkCreateWaylandSurfaceKHR)load(context, "vkCreateWaylandSurfaceKHR");
	vkGetPhysicalDeviceWaylandPresentationSupportKHR = (PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR)load(context, "vkGetPhysicalDeviceWaylandPresentationSupportKHR");
#endif /* defined(VK_KHR_wayland_surface) */
#if defined(VK_KHR_win32_surface)
	vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)load(context, "vkCreateWin32SurfaceKHR");
	vkGetPhysicalDeviceWin32PresentationSupportKHR = (PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR)load(context, "vkGetPhysicalDeviceWin32PresentationSupportKHR");
#endif /* defined(VK_KHR_win32_surface) */
#if defined(VK_KHR_xcb_surface)
	vkCreateXcbSurfaceKHR = (PFN_vkCreateXcbSurfaceKHR)load(context, "vkCreateXcbSurfaceKHR");
	vkGetPhysicalDeviceXcbPresentationSupportKHR = (PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR)load(context, "vkGetPhysicalDeviceXcbPresentationSupportKHR");
#endif /* defined(VK_KHR_xcb_surface) */
#if defined(VK_KHR_xlib_surface)
	vkCreateXlibSurfaceKHR = (PFN_vkCreateXlibSurfaceKHR)load(context, "vkCreateXlibSurfaceKHR");
	vkGetPhysicalDeviceXlibPresentationSupportKHR = (PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR)load(context, "vkGetPhysicalDeviceXlibPresentationSupportKHR");
#endif /* defined(VK_KHR_xlib_surface) */
#if defined(VK_MVK_ios_surface)
	vkCreateIOSSurfaceMVK = (PFN_vkCreateIOSSurfaceMVK)load(context, "vkCreateIOSSurfaceMVK");
#endif /* defined(VK_MVK_ios_surface) */
#if defined(VK_MVK_macos_surface)
	vkCreateMacOSSurfaceMVK = (PFN_vkCreateMacOSSurfaceMVK)load(context, "vkCreateMacOSSurfaceMVK");
#endif /* defined(VK_MVK_macos_surface) */
#if defined(VK_NN_vi_surface)
	vkCreateViSurfaceNN = (PFN_vkCreateViSurfaceNN)load(context, "vkCreateViSurfaceNN");
#endif /* defined(VK_NN_vi_surface) */
#if defined(VK_NVX_device_generated_commands)
	vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX = (PFN_vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX)load(context, "vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX");
#endif /* defined(VK_NVX_device_generated_commands) */
#if defined(VK_NV_external_memory_capabilities)
	vkGetPhysicalDeviceExternalImageFormatPropertiesNV = (PFN_vkGetPhysicalDeviceExternalImageFormatPropertiesNV)load(context, "vkGetPhysicalDeviceExternalImageFormatPropertiesNV");
#endif /* defined(VK_NV_external_memory_capabilities) */
#if (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
	vkGetPhysicalDevicePresentRectanglesKHR = (PFN_vkGetPhysicalDevicePresentRectanglesKHR)load(context, "vkGetPhysicalDevicePresentRectanglesKHR");
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */
	/* VOLK_GENERATE_LOAD_INSTANCE */
}

static void volkGenLoadDevice(void* context, PFN_vkVoidFunction (*load)(void*, const char*))
{
	/* VOLK_GENERATE_LOAD_DEVICE */
#if defined(VK_VERSION_1_0)
	vkAllocateCommandBuffers = (PFN_vkAllocateCommandBuffers)load(context, "vkAllocateCommandBuffers");
	vkAllocateDescriptorSets = (PFN_vkAllocateDescriptorSets)load(context, "vkAllocateDescriptorSets");
	vkAllocateMemory = (PFN_vkAllocateMemory)load(context, "vkAllocateMemory");
	vkBeginCommandBuffer = (PFN_vkBeginCommandBuffer)load(context, "vkBeginCommandBuffer");
	vkBindBufferMemory = (PFN_vkBindBufferMemory)load(context, "vkBindBufferMemory");
	vkBindImageMemory = (PFN_vkBindImageMemory)load(context, "vkBindImageMemory");
	vkCmdBeginQuery = (PFN_vkCmdBeginQuery)load(context, "vkCmdBeginQuery");
	vkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)load(context, "vkCmdBeginRenderPass");
	vkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets)load(context, "vkCmdBindDescriptorSets");
	vkCmdBindIndexBuffer = (PFN_vkCmdBindIndexBuffer)load(context, "vkCmdBindIndexBuffer");
	vkCmdBindPipeline = (PFN_vkCmdBindPipeline)load(context, "vkCmdBindPipeline");
	vkCmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers)load(context, "vkCmdBindVertexBuffers");
	vkCmdBlitImage = (PFN_vkCmdBlitImage)load(context, "vkCmdBlitImage");
	vkCmdClearAttachments = (PFN_vkCmdClearAttachments)load(context, "vkCmdClearAttachments");
	vkCmdClearColorImage = (PFN_vkCmdClearColorImage)load(context, "vkCmdClearColorImage");
	vkCmdClearDepthStencilImage = (PFN_vkCmdClearDepthStencilImage)load(context, "vkCmdClearDepthStencilImage");
	vkCmdCopyBuffer = (PFN_vkCmdCopyBuffer)load(context, "vkCmdCopyBuffer");
	vkCmdCopyBufferToImage = (PFN_vkCmdCopyBufferToImage)load(context, "vkCmdCopyBufferToImage");
	vkCmdCopyImage = (PFN_vkCmdCopyImage)load(context, "vkCmdCopyImage");
	vkCmdCopyImageToBuffer = (PFN_vkCmdCopyImageToBuffer)load(context, "vkCmdCopyImageToBuffer");
	vkCmdCopyQueryPoolResults = (PFN_vkCmdCopyQueryPoolResults)load(context, "vkCmdCopyQueryPoolResults");
	vkCmdDispatch = (PFN_vkCmdDispatch)load(context, "vkCmdDispatch");
	vkCmdDispatchIndirect = (PFN_vkCmdDispatchIndirect)load(context, "vkCmdDispatchIndirect");
	vkCmdDraw = (PFN_vkCmdDraw)load(context, "vkCmdDraw");
	vkCmdDrawIndexed = (PFN_vkCmdDrawIndexed)load(context, "vkCmdDrawIndexed");
	vkCmdDrawIndexedIndirect = (PFN_vkCmdDrawIndexedIndirect)load(context, "vkCmdDrawIndexedIndirect");
	vkCmdDrawIndirect = (PFN_vkCmdDrawIndirect)load(context, "vkCmdDrawIndirect");
	vkCmdEndQuery = (PFN_vkCmdEndQuery)load(context, "vkCmdEndQuery");
	vkCmdEndRenderPass = (PFN_vkCmdEndRenderPass)load(context, "vkCmdEndRenderPass");
	vkCmdExecuteCommands = (PFN_vkCmdExecuteCommands)load(context, "vkCmdExecuteCommands");
	vkCmdFillBuffer = (PFN_vkCmdFillBuffer)load(context, "vkCmdFillBuffer");
	vkCmdNextSubpass = (PFN_vkCmdNextSubpass)load(context, "vkCmdNextSubpass");
	vkCmdPipelineBarrier = (PFN_vkCmdPipelineBarrier)load(context, "vkCmdPipelineBarrier");
	vkCmdPushConstants = (PFN_vkCmdPushConstants)load(context, "vkCmdPushConstants");
	vkCmdResetEvent = (PFN_vkCmdResetEvent)load(context, "vkCmdResetEvent");
	vkCmdResetQueryPool = (PFN_vkCmdResetQueryPool)load(context, "vkCmdResetQueryPool");
	vkCmdResolveImage = (PFN_vkCmdResolveImage)load(context, "vkCmdResolveImage");
	vkCmdSetBlendConstants = (PFN_vkCmdSetBlendConstants)load(context, "vkCmdSetBlendConstants");
	vkCmdSetDepthBias = (PFN_vkCmdSetDepthBias)load(context, "vkCmdSetDepthBias");
	vkCmdSetDepthBounds = (PFN_vkCmdSetDepthBounds)load(context, "vkCmdSetDepthBounds");
	vkCmdSetEvent = (PFN_vkCmdSetEvent)load(context, "vkCmdSetEvent");
	vkCmdSetLineWidth = (PFN_vkCmdSetLineWidth)load(context, "vkCmdSetLineWidth");
	vkCmdSetScissor = (PFN_vkCmdSetScissor)load(context, "vkCmdSetScissor");
	vkCmdSetStencilCompareMask = (PFN_vkCmdSetStencilCompareMask)load(context, "vkCmdSetStencilCompareMask");
	vkCmdSetStencilReference = (PFN_vkCmdSetStencilReference)load(context, "vkCmdSetStencilReference");
	vkCmdSetStencilWriteMask = (PFN_vkCmdSetStencilWriteMask)load(context, "vkCmdSetStencilWriteMask");
	vkCmdSetViewport = (PFN_vkCmdSetViewport)load(context, "vkCmdSetViewport");
	vkCmdUpdateBuffer = (PFN_vkCmdUpdateBuffer)load(context, "vkCmdUpdateBuffer");
	vkCmdWaitEvents = (PFN_vkCmdWaitEvents)load(context, "vkCmdWaitEvents");
	vkCmdWriteTimestamp = (PFN_vkCmdWriteTimestamp)load(context, "vkCmdWriteTimestamp");
	vkCreateBuffer = (PFN_vkCreateBuffer)load(context, "vkCreateBuffer");
	vkCreateBufferView = (PFN_vkCreateBufferView)load(context, "vkCreateBufferView");
	vkCreateCommandPool = (PFN_vkCreateCommandPool)load(context, "vkCreateCommandPool");
	vkCreateComputePipelines = (PFN_vkCreateComputePipelines)load(context, "vkCreateComputePipelines");
	vkCreateDescriptorPool = (PFN_vkCreateDescriptorPool)load(context, "vkCreateDescriptorPool");
	vkCreateDescriptorSetLayout = (PFN_vkCreateDescriptorSetLayout)load(context, "vkCreateDescriptorSetLayout");
	vkCreateEvent = (PFN_vkCreateEvent)load(context, "vkCreateEvent");
	vkCreateFence = (PFN_vkCreateFence)load(context, "vkCreateFence");
	vkCreateFramebuffer = (PFN_vkCreateFramebuffer)load(context, "vkCreateFramebuffer");
	vkCreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines)load(context, "vkCreateGraphicsPipelines");
	vkCreateImage = (PFN_vkCreateImage)load(context, "vkCreateImage");
	vkCreateImageView = (PFN_vkCreateImageView)load(context, "vkCreateImageView");
	vkCreatePipelineCache = (PFN_vkCreatePipelineCache)load(context, "vkCreatePipelineCache");
	vkCreatePipelineLayout = (PFN_vkCreatePipelineLayout)load(context, "vkCreatePipelineLayout");
	vkCreateQueryPool = (PFN_vkCreateQueryPool)load(context, "vkCreateQueryPool");
	vkCreateRenderPass = (PFN_vkCreateRenderPass)load(context, "vkCreateRenderPass");
	vkCreateSampler = (PFN_vkCreateSampler)load(context, "vkCreateSampler");
	vkCreateSemaphore = (PFN_vkCreateSemaphore)load(context, "vkCreateSemaphore");
	vkCreateShaderModule = (PFN_vkCreateShaderModule)load(context, "vkCreateShaderModule");
	vkDestroyBuffer = (PFN_vkDestroyBuffer)load(context, "vkDestroyBuffer");
	vkDestroyBufferView = (PFN_vkDestroyBufferView)load(context, "vkDestroyBufferView");
	vkDestroyCommandPool = (PFN_vkDestroyCommandPool)load(context, "vkDestroyCommandPool");
	vkDestroyDescriptorPool = (PFN_vkDestroyDescriptorPool)load(context, "vkDestroyDescriptorPool");
	vkDestroyDescriptorSetLayout = (PFN_vkDestroyDescriptorSetLayout)load(context, "vkDestroyDescriptorSetLayout");
	vkDestroyDevice = (PFN_vkDestroyDevice)load(context, "vkDestroyDevice");
	vkDestroyEvent = (PFN_vkDestroyEvent)load(context, "vkDestroyEvent");
	vkDestroyFence = (PFN_vkDestroyFence)load(context, "vkDestroyFence");
	vkDestroyFramebuffer = (PFN_vkDestroyFramebuffer)load(context, "vkDestroyFramebuffer");
	vkDestroyImage = (PFN_vkDestroyImage)load(context, "vkDestroyImage");
	vkDestroyImageView = (PFN_vkDestroyImageView)load(context, "vkDestroyImageView");
	vkDestroyPipeline = (PFN_vkDestroyPipeline)load(context, "vkDestroyPipeline");
	vkDestroyPipelineCache = (PFN_vkDestroyPipelineCache)load(context, "vkDestroyPipelineCache");
	vkDestroyPipelineLayout = (PFN_vkDestroyPipelineLayout)load(context, "vkDestroyPipelineLayout");
	vkDestroyQueryPool = (PFN_vkDestroyQueryPool)load(context, "vkDestroyQueryPool");
	vkDestroyRenderPass = (PFN_vkDestroyRenderPass)load(context, "vkDestroyRenderPass");
	vkDestroySampler = (PFN_vkDestroySampler)load(context, "vkDestroySampler");
	vkDestroySemaphore = (PFN_vkDestroySemaphore)load(context, "vkDestroySemaphore");
	vkDestroyShaderModule = (PFN_vkDestroyShaderModule)load(context, "vkDestroyShaderModule");
	vkDeviceWaitIdle = (PFN_vkDeviceWaitIdle)load(context, "vkDeviceWaitIdle");
	vkEndCommandBuffer = (PFN_vkEndCommandBuffer)load(context, "vkEndCommandBuffer");
	vkFlushMappedMemoryRanges = (PFN_vkFlushMappedMemoryRanges)load(context, "vkFlushMappedMemoryRanges");
	vkFreeCommandBuffers = (PFN_vkFreeCommandBuffers)load(context, "vkFreeCommandBuffers");
	vkFreeDescriptorSets = (PFN_vkFreeDescriptorSets)load(context, "vkFreeDescriptorSets");
	vkFreeMemory = (PFN_vkFreeMemory)load(context, "vkFreeMemory");
	vkGetBufferMemoryRequirements = (PFN_vkGetBufferMemoryRequirements)load(context, "vkGetBufferMemoryRequirements");
	vkGetDeviceMemoryCommitment = (PFN_vkGetDeviceMemoryCommitment)load(context, "vkGetDeviceMemoryCommitment");
	vkGetDeviceQueue = (PFN_vkGetDeviceQueue)load(context, "vkGetDeviceQueue");
	vkGetEventStatus = (PFN_vkGetEventStatus)load(context, "vkGetEventStatus");
	vkGetFenceStatus = (PFN_vkGetFenceStatus)load(context, "vkGetFenceStatus");
	vkGetImageMemoryRequirements = (PFN_vkGetImageMemoryRequirements)load(context, "vkGetImageMemoryRequirements");
	vkGetImageSparseMemoryRequirements = (PFN_vkGetImageSparseMemoryRequirements)load(context, "vkGetImageSparseMemoryRequirements");
	vkGetImageSubresourceLayout = (PFN_vkGetImageSubresourceLayout)load(context, "vkGetImageSubresourceLayout");
	vkGetPipelineCacheData = (PFN_vkGetPipelineCacheData)load(context, "vkGetPipelineCacheData");
	vkGetQueryPoolResults = (PFN_vkGetQueryPoolResults)load(context, "vkGetQueryPoolResults");
	vkGetRenderAreaGranularity = (PFN_vkGetRenderAreaGranularity)load(context, "vkGetRenderAreaGranularity");
	vkInvalidateMappedMemoryRanges = (PFN_vkInvalidateMappedMemoryRanges)load(context, "vkInvalidateMappedMemoryRanges");
	vkMapMemory = (PFN_vkMapMemory)load(context, "vkMapMemory");
	vkMergePipelineCaches = (PFN_vkMergePipelineCaches)load(context, "vkMergePipelineCaches");
	vkQueueBindSparse = (PFN_vkQueueBindSparse)load(context, "vkQueueBindSparse");
	vkQueueSubmit = (PFN_vkQueueSubmit)load(context, "vkQueueSubmit");
	vkQueueWaitIdle = (PFN_vkQueueWaitIdle)load(context, "vkQueueWaitIdle");
	vkResetCommandBuffer = (PFN_vkResetCommandBuffer)load(context, "vkResetCommandBuffer");
	vkResetCommandPool = (PFN_vkResetCommandPool)load(context, "vkResetCommandPool");
	vkResetDescriptorPool = (PFN_vkResetDescriptorPool)load(context, "vkResetDescriptorPool");
	vkResetEvent = (PFN_vkResetEvent)load(context, "vkResetEvent");
	vkResetFences = (PFN_vkResetFences)load(context, "vkResetFences");
	vkSetEvent = (PFN_vkSetEvent)load(context, "vkSetEvent");
	vkUnmapMemory = (PFN_vkUnmapMemory)load(context, "vkUnmapMemory");
	vkUpdateDescriptorSets = (PFN_vkUpdateDescriptorSets)load(context, "vkUpdateDescriptorSets");
	vkWaitForFences = (PFN_vkWaitForFences)load(context, "vkWaitForFences");
#endif /* defined(VK_VERSION_1_0) */
#if defined(VK_VERSION_1_1)
	vkBindBufferMemory2 = (PFN_vkBindBufferMemory2)load(context, "vkBindBufferMemory2");
	vkBindImageMemory2 = (PFN_vkBindImageMemory2)load(context, "vkBindImageMemory2");
	vkCmdDispatchBase = (PFN_vkCmdDispatchBase)load(context, "vkCmdDispatchBase");
	vkCmdSetDeviceMask = (PFN_vkCmdSetDeviceMask)load(context, "vkCmdSetDeviceMask");
	vkCreateDescriptorUpdateTemplate = (PFN_vkCreateDescriptorUpdateTemplate)load(context, "vkCreateDescriptorUpdateTemplate");
	vkCreateSamplerYcbcrConversion = (PFN_vkCreateSamplerYcbcrConversion)load(context, "vkCreateSamplerYcbcrConversion");
	vkDestroyDescriptorUpdateTemplate = (PFN_vkDestroyDescriptorUpdateTemplate)load(context, "vkDestroyDescriptorUpdateTemplate");
	vkDestroySamplerYcbcrConversion = (PFN_vkDestroySamplerYcbcrConversion)load(context, "vkDestroySamplerYcbcrConversion");
	vkGetBufferMemoryRequirements2 = (PFN_vkGetBufferMemoryRequirements2)load(context, "vkGetBufferMemoryRequirements2");
	vkGetDescriptorSetLayoutSupport = (PFN_vkGetDescriptorSetLayoutSupport)load(context, "vkGetDescriptorSetLayoutSupport");
	vkGetDeviceGroupPeerMemoryFeatures = (PFN_vkGetDeviceGroupPeerMemoryFeatures)load(context, "vkGetDeviceGroupPeerMemoryFeatures");
	vkGetDeviceQueue2 = (PFN_vkGetDeviceQueue2)load(context, "vkGetDeviceQueue2");
	vkGetImageMemoryRequirements2 = (PFN_vkGetImageMemoryRequirements2)load(context, "vkGetImageMemoryRequirements2");
	vkGetImageSparseMemoryRequirements2 = (PFN_vkGetImageSparseMemoryRequirements2)load(context, "vkGetImageSparseMemoryRequirements2");
	vkTrimCommandPool = (PFN_vkTrimCommandPool)load(context, "vkTrimCommandPool");
	vkUpdateDescriptorSetWithTemplate = (PFN_vkUpdateDescriptorSetWithTemplate)load(context, "vkUpdateDescriptorSetWithTemplate");
#endif /* defined(VK_VERSION_1_1) */
#if defined(VK_AMD_buffer_marker)
	vkCmdWriteBufferMarkerAMD = (PFN_vkCmdWriteBufferMarkerAMD)load(context, "vkCmdWriteBufferMarkerAMD");
#endif /* defined(VK_AMD_buffer_marker) */
#if defined(VK_AMD_draw_indirect_count)
	vkCmdDrawIndexedIndirectCountAMD = (PFN_vkCmdDrawIndexedIndirectCountAMD)load(context, "vkCmdDrawIndexedIndirectCountAMD");
	vkCmdDrawIndirectCountAMD = (PFN_vkCmdDrawIndirectCountAMD)load(context, "vkCmdDrawIndirectCountAMD");
#endif /* defined(VK_AMD_draw_indirect_count) */
#if defined(VK_AMD_shader_info)
	vkGetShaderInfoAMD = (PFN_vkGetShaderInfoAMD)load(context, "vkGetShaderInfoAMD");
#endif /* defined(VK_AMD_shader_info) */
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
	vkGetAndroidHardwareBufferPropertiesANDROID = (PFN_vkGetAndroidHardwareBufferPropertiesANDROID)load(context, "vkGetAndroidHardwareBufferPropertiesANDROID");
	vkGetMemoryAndroidHardwareBufferANDROID = (PFN_vkGetMemoryAndroidHardwareBufferANDROID)load(context, "vkGetMemoryAndroidHardwareBufferANDROID");
#endif /* defined(VK_ANDROID_external_memory_android_hardware_buffer) */
#if defined(VK_ANDROID_native_buffer)
	vkAcquireImageANDROID = (PFN_vkAcquireImageANDROID)load(context, "vkAcquireImageANDROID");
	vkGetSwapchainGrallocUsageANDROID = (PFN_vkGetSwapchainGrallocUsageANDROID)load(context, "vkGetSwapchainGrallocUsageANDROID");
	vkQueueSignalReleaseImageANDROID = (PFN_vkQueueSignalReleaseImageANDROID)load(context, "vkQueueSignalReleaseImageANDROID");
#endif /* defined(VK_ANDROID_native_buffer) */
#if defined(VK_EXT_debug_marker)
	vkCmdDebugMarkerBeginEXT = (PFN_vkCmdDebugMarkerBeginEXT)load(context, "vkCmdDebugMarkerBeginEXT");
	vkCmdDebugMarkerEndEXT = (PFN_vkCmdDebugMarkerEndEXT)load(context, "vkCmdDebugMarkerEndEXT");
	vkCmdDebugMarkerInsertEXT = (PFN_vkCmdDebugMarkerInsertEXT)load(context, "vkCmdDebugMarkerInsertEXT");
	vkDebugMarkerSetObjectNameEXT = (PFN_vkDebugMarkerSetObjectNameEXT)load(context, "vkDebugMarkerSetObjectNameEXT");
	vkDebugMarkerSetObjectTagEXT = (PFN_vkDebugMarkerSetObjectTagEXT)load(context, "vkDebugMarkerSetObjectTagEXT");
#endif /* defined(VK_EXT_debug_marker) */
#if defined(VK_EXT_debug_utils)
	vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)load(context, "vkCmdBeginDebugUtilsLabelEXT");
	vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)load(context, "vkCmdEndDebugUtilsLabelEXT");
	vkCmdInsertDebugUtilsLabelEXT = (PFN_vkCmdInsertDebugUtilsLabelEXT)load(context, "vkCmdInsertDebugUtilsLabelEXT");
	vkQueueBeginDebugUtilsLabelEXT = (PFN_vkQueueBeginDebugUtilsLabelEXT)load(context, "vkQueueBeginDebugUtilsLabelEXT");
	vkQueueEndDebugUtilsLabelEXT = (PFN_vkQueueEndDebugUtilsLabelEXT)load(context, "vkQueueEndDebugUtilsLabelEXT");
	vkQueueInsertDebugUtilsLabelEXT = (PFN_vkQueueInsertDebugUtilsLabelEXT)load(context, "vkQueueInsertDebugUtilsLabelEXT");
	vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)load(context, "vkSetDebugUtilsObjectNameEXT");
	vkSetDebugUtilsObjectTagEXT = (PFN_vkSetDebugUtilsObjectTagEXT)load(context, "vkSetDebugUtilsObjectTagEXT");
#endif /* defined(VK_EXT_debug_utils) */
#if defined(VK_EXT_discard_rectangles)
	vkCmdSetDiscardRectangleEXT = (PFN_vkCmdSetDiscardRectangleEXT)load(context, "vkCmdSetDiscardRectangleEXT");
#endif /* defined(VK_EXT_discard_rectangles) */
#if defined(VK_EXT_display_control)
	vkDisplayPowerControlEXT = (PFN_vkDisplayPowerControlEXT)load(context, "vkDisplayPowerControlEXT");
	vkGetSwapchainCounterEXT = (PFN_vkGetSwapchainCounterEXT)load(context, "vkGetSwapchainCounterEXT");
	vkRegisterDeviceEventEXT = (PFN_vkRegisterDeviceEventEXT)load(context, "vkRegisterDeviceEventEXT");
	vkRegisterDisplayEventEXT = (PFN_vkRegisterDisplayEventEXT)load(context, "vkRegisterDisplayEventEXT");
#endif /* defined(VK_EXT_display_control) */
#if defined(VK_EXT_external_memory_host)
	vkGetMemoryHostPointerPropertiesEXT = (PFN_vkGetMemoryHostPointerPropertiesEXT)load(context, "vkGetMemoryHostPointerPropertiesEXT");
#endif /* defined(VK_EXT_external_memory_host) */
#if defined(VK_EXT_hdr_metadata)
	vkSetHdrMetadataEXT = (PFN_vkSetHdrMetadataEXT)load(context, "vkSetHdrMetadataEXT");
#endif /* defined(VK_EXT_hdr_metadata) */
#if defined(VK_EXT_sample_locations)
	vkCmdSetSampleLocationsEXT = (PFN_vkCmdSetSampleLocationsEXT)load(context, "vkCmdSetSampleLocationsEXT");
#endif /* defined(VK_EXT_sample_locations) */
#if defined(VK_EXT_validation_cache)
	vkCreateValidationCacheEXT = (PFN_vkCreateValidationCacheEXT)load(context, "vkCreateValidationCacheEXT");
	vkDestroyValidationCacheEXT = (PFN_vkDestroyValidationCacheEXT)load(context, "vkDestroyValidationCacheEXT");
	vkGetValidationCacheDataEXT = (PFN_vkGetValidationCacheDataEXT)load(context, "vkGetValidationCacheDataEXT");
	vkMergeValidationCachesEXT = (PFN_vkMergeValidationCachesEXT)load(context, "vkMergeValidationCachesEXT");
#endif /* defined(VK_EXT_validation_cache) */
#if defined(VK_GOOGLE_display_timing)
	vkGetPastPresentationTimingGOOGLE = (PFN_vkGetPastPresentationTimingGOOGLE)load(context, "vkGetPastPresentationTimingGOOGLE");
	vkGetRefreshCycleDurationGOOGLE = (PFN_vkGetRefreshCycleDurationGOOGLE)load(context, "vkGetRefreshCycleDurationGOOGLE");
#endif /* defined(VK_GOOGLE_display_timing) */
#if defined(VK_KHR_bind_memory2)
	vkBindBufferMemory2KHR = (PFN_vkBindBufferMemory2KHR)load(context, "vkBindBufferMemory2KHR");
	vkBindImageMemory2KHR = (PFN_vkBindImageMemory2KHR)load(context, "vkBindImageMemory2KHR");
#endif /* defined(VK_KHR_bind_memory2) */
#if defined(VK_KHR_descriptor_update_template)
	vkCreateDescriptorUpdateTemplateKHR = (PFN_vkCreateDescriptorUpdateTemplateKHR)load(context, "vkCreateDescriptorUpdateTemplateKHR");
	vkDestroyDescriptorUpdateTemplateKHR = (PFN_vkDestroyDescriptorUpdateTemplateKHR)load(context, "vkDestroyDescriptorUpdateTemplateKHR");
	vkUpdateDescriptorSetWithTemplateKHR = (PFN_vkUpdateDescriptorSetWithTemplateKHR)load(context, "vkUpdateDescriptorSetWithTemplateKHR");
#endif /* defined(VK_KHR_descriptor_update_template) */
#if defined(VK_KHR_device_group)
	vkCmdDispatchBaseKHR = (PFN_vkCmdDispatchBaseKHR)load(context, "vkCmdDispatchBaseKHR");
	vkCmdSetDeviceMaskKHR = (PFN_vkCmdSetDeviceMaskKHR)load(context, "vkCmdSetDeviceMaskKHR");
	vkGetDeviceGroupPeerMemoryFeaturesKHR = (PFN_vkGetDeviceGroupPeerMemoryFeaturesKHR)load(context, "vkGetDeviceGroupPeerMemoryFeaturesKHR");
#endif /* defined(VK_KHR_device_group) */
#if defined(VK_KHR_display_swapchain)
	vkCreateSharedSwapchainsKHR = (PFN_vkCreateSharedSwapchainsKHR)load(context, "vkCreateSharedSwapchainsKHR");
#endif /* defined(VK_KHR_display_swapchain) */
#if defined(VK_KHR_draw_indirect_count)
	vkCmdDrawIndexedIndirectCountKHR = (PFN_vkCmdDrawIndexedIndirectCountKHR)load(context, "vkCmdDrawIndexedIndirectCountKHR");
	vkCmdDrawIndirectCountKHR = (PFN_vkCmdDrawIndirectCountKHR)load(context, "vkCmdDrawIndirectCountKHR");
#endif /* defined(VK_KHR_draw_indirect_count) */
#if defined(VK_KHR_external_fence_fd)
	vkGetFenceFdKHR = (PFN_vkGetFenceFdKHR)load(context, "vkGetFenceFdKHR");
	vkImportFenceFdKHR = (PFN_vkImportFenceFdKHR)load(context, "vkImportFenceFdKHR");
#endif /* defined(VK_KHR_external_fence_fd) */
#if defined(VK_KHR_external_fence_win32)
	vkGetFenceWin32HandleKHR = (PFN_vkGetFenceWin32HandleKHR)load(context, "vkGetFenceWin32HandleKHR");
	vkImportFenceWin32HandleKHR = (PFN_vkImportFenceWin32HandleKHR)load(context, "vkImportFenceWin32HandleKHR");
#endif /* defined(VK_KHR_external_fence_win32) */
#if defined(VK_KHR_external_memory_fd)
	vkGetMemoryFdKHR = (PFN_vkGetMemoryFdKHR)load(context, "vkGetMemoryFdKHR");
	vkGetMemoryFdPropertiesKHR = (PFN_vkGetMemoryFdPropertiesKHR)load(context, "vkGetMemoryFdPropertiesKHR");
#endif /* defined(VK_KHR_external_memory_fd) */
#if defined(VK_KHR_external_memory_win32)
	vkGetMemoryWin32HandleKHR = (PFN_vkGetMemoryWin32HandleKHR)load(context, "vkGetMemoryWin32HandleKHR");
	vkGetMemoryWin32HandlePropertiesKHR = (PFN_vkGetMemoryWin32HandlePropertiesKHR)load(context, "vkGetMemoryWin32HandlePropertiesKHR");
#endif /* defined(VK_KHR_external_memory_win32) */
#if defined(VK_KHR_external_semaphore_fd)
	vkGetSemaphoreFdKHR = (PFN_vkGetSemaphoreFdKHR)load(context, "vkGetSemaphoreFdKHR");
	vkImportSemaphoreFdKHR = (PFN_vkImportSemaphoreFdKHR)load(context, "vkImportSemaphoreFdKHR");
#endif /* defined(VK_KHR_external_semaphore_fd) */
#if defined(VK_KHR_external_semaphore_win32)
	vkGetSemaphoreWin32HandleKHR = (PFN_vkGetSemaphoreWin32HandleKHR)load(context, "vkGetSemaphoreWin32HandleKHR");
	vkImportSemaphoreWin32HandleKHR = (PFN_vkImportSemaphoreWin32HandleKHR)load(context, "vkImportSemaphoreWin32HandleKHR");
#endif /* defined(VK_KHR_external_semaphore_win32) */
#if defined(VK_KHR_get_memory_requirements2)
	vkGetBufferMemoryRequirements2KHR = (PFN_vkGetBufferMemoryRequirements2KHR)load(context, "vkGetBufferMemoryRequirements2KHR");
	vkGetImageMemoryRequirements2KHR = (PFN_vkGetImageMemoryRequirements2KHR)load(context, "vkGetImageMemoryRequirements2KHR");
	vkGetImageSparseMemoryRequirements2KHR = (PFN_vkGetImageSparseMemoryRequirements2KHR)load(context, "vkGetImageSparseMemoryRequirements2KHR");
#endif /* defined(VK_KHR_get_memory_requirements2) */
#if defined(VK_KHR_maintenance1)
	vkTrimCommandPoolKHR = (PFN_vkTrimCommandPoolKHR)load(context, "vkTrimCommandPoolKHR");
#endif /* defined(VK_KHR_maintenance1) */
#if defined(VK_KHR_maintenance3)
	vkGetDescriptorSetLayoutSupportKHR = (PFN_vkGetDescriptorSetLayoutSupportKHR)load(context, "vkGetDescriptorSetLayoutSupportKHR");
#endif /* defined(VK_KHR_maintenance3) */
#if defined(VK_KHR_push_descriptor)
	vkCmdPushDescriptorSetKHR = (PFN_vkCmdPushDescriptorSetKHR)load(context, "vkCmdPushDescriptorSetKHR");
#endif /* defined(VK_KHR_push_descriptor) */
#if defined(VK_KHR_sampler_ycbcr_conversion)
	vkCreateSamplerYcbcrConversionKHR = (PFN_vkCreateSamplerYcbcrConversionKHR)load(context, "vkCreateSamplerYcbcrConversionKHR");
	vkDestroySamplerYcbcrConversionKHR = (PFN_vkDestroySamplerYcbcrConversionKHR)load(context, "vkDestroySamplerYcbcrConversionKHR");
#endif /* defined(VK_KHR_sampler_ycbcr_conversion) */
#if defined(VK_KHR_shared_presentable_image)
	vkGetSwapchainStatusKHR = (PFN_vkGetSwapchainStatusKHR)load(context, "vkGetSwapchainStatusKHR");
#endif /* defined(VK_KHR_shared_presentable_image) */
#if defined(VK_KHR_swapchain)
	vkAcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)load(context, "vkAcquireNextImageKHR");
	vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)load(context, "vkCreateSwapchainKHR");
	vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)load(context, "vkDestroySwapchainKHR");
	vkGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)load(context, "vkGetSwapchainImagesKHR");
	vkQueuePresentKHR = (PFN_vkQueuePresentKHR)load(context, "vkQueuePresentKHR");
#endif /* defined(VK_KHR_swapchain) */
#if defined(VK_NVX_device_generated_commands)
	vkCmdProcessCommandsNVX = (PFN_vkCmdProcessCommandsNVX)load(context, "vkCmdProcessCommandsNVX");
	vkCmdReserveSpaceForCommandsNVX = (PFN_vkCmdReserveSpaceForCommandsNVX)load(context, "vkCmdReserveSpaceForCommandsNVX");
	vkCreateIndirectCommandsLayoutNVX = (PFN_vkCreateIndirectCommandsLayoutNVX)load(context, "vkCreateIndirectCommandsLayoutNVX");
	vkCreateObjectTableNVX = (PFN_vkCreateObjectTableNVX)load(context, "vkCreateObjectTableNVX");
	vkDestroyIndirectCommandsLayoutNVX = (PFN_vkDestroyIndirectCommandsLayoutNVX)load(context, "vkDestroyIndirectCommandsLayoutNVX");
	vkDestroyObjectTableNVX = (PFN_vkDestroyObjectTableNVX)load(context, "vkDestroyObjectTableNVX");
	vkRegisterObjectsNVX = (PFN_vkRegisterObjectsNVX)load(context, "vkRegisterObjectsNVX");
	vkUnregisterObjectsNVX = (PFN_vkUnregisterObjectsNVX)load(context, "vkUnregisterObjectsNVX");
#endif /* defined(VK_NVX_device_generated_commands) */
#if defined(VK_NV_clip_space_w_scaling)
	vkCmdSetViewportWScalingNV = (PFN_vkCmdSetViewportWScalingNV)load(context, "vkCmdSetViewportWScalingNV");
#endif /* defined(VK_NV_clip_space_w_scaling) */
#if defined(VK_NV_external_memory_win32)
	vkGetMemoryWin32HandleNV = (PFN_vkGetMemoryWin32HandleNV)load(context, "vkGetMemoryWin32HandleNV");
#endif /* defined(VK_NV_external_memory_win32) */
#if (defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor)) || (defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1))
	vkCmdPushDescriptorSetWithTemplateKHR = (PFN_vkCmdPushDescriptorSetWithTemplateKHR)load(context, "vkCmdPushDescriptorSetWithTemplateKHR");
#endif /* (defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor)) || (defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1)) */
#if (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
	vkGetDeviceGroupPresentCapabilitiesKHR = (PFN_vkGetDeviceGroupPresentCapabilitiesKHR)load(context, "vkGetDeviceGroupPresentCapabilitiesKHR");
	vkGetDeviceGroupSurfacePresentModesKHR = (PFN_vkGetDeviceGroupSurfacePresentModesKHR)load(context, "vkGetDeviceGroupSurfacePresentModesKHR");
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */
#if (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
	vkAcquireNextImage2KHR = (PFN_vkAcquireNextImage2KHR)load(context, "vkAcquireNextImage2KHR");
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */
	/* VOLK_GENERATE_LOAD_DEVICE */
}

static void volkGenLoadDeviceTable(struct VolkDeviceTable* table, void* context, PFN_vkVoidFunction (*load)(void*, const char*))
{
	/* VOLK_GENERATE_LOAD_DEVICE_TABLE */
#if defined(VK_VERSION_1_0)
	table->vkAllocateCommandBuffers = (PFN_vkAllocateCommandBuffers)load(context, "vkAllocateCommandBuffers");
	table->vkAllocateDescriptorSets = (PFN_vkAllocateDescriptorSets)load(context, "vkAllocateDescriptorSets");
	table->vkAllocateMemory = (PFN_vkAllocateMemory)load(context, "vkAllocateMemory");
	table->vkBeginCommandBuffer = (PFN_vkBeginCommandBuffer)load(context, "vkBeginCommandBuffer");
	table->vkBindBufferMemory = (PFN_vkBindBufferMemory)load(context, "vkBindBufferMemory");
	table->vkBindImageMemory = (PFN_vkBindImageMemory)load(context, "vkBindImageMemory");
	table->vkCmdBeginQuery = (PFN_vkCmdBeginQuery)load(context, "vkCmdBeginQuery");
	table->vkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)load(context, "vkCmdBeginRenderPass");
	table->vkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets)load(context, "vkCmdBindDescriptorSets");
	table->vkCmdBindIndexBuffer = (PFN_vkCmdBindIndexBuffer)load(context, "vkCmdBindIndexBuffer");
	table->vkCmdBindPipeline = (PFN_vkCmdBindPipeline)load(context, "vkCmdBindPipeline");
	table->vkCmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers)load(context, "vkCmdBindVertexBuffers");
	table->vkCmdBlitImage = (PFN_vkCmdBlitImage)load(context, "vkCmdBlitImage");
	table->vkCmdClearAttachments = (PFN_vkCmdClearAttachments)load(context, "vkCmdClearAttachments");
	table->vkCmdClearColorImage = (PFN_vkCmdClearColorImage)load(context, "vkCmdClearColorImage");
	table->vkCmdClearDepthStencilImage = (PFN_vkCmdClearDepthStencilImage)load(context, "vkCmdClearDepthStencilImage");
	table->vkCmdCopyBuffer = (PFN_vkCmdCopyBuffer)load(context, "vkCmdCopyBuffer");
	table->vkCmdCopyBufferToImage = (PFN_vkCmdCopyBufferToImage)load(context, "vkCmdCopyBufferToImage");
	table->vkCmdCopyImage = (PFN_vkCmdCopyImage)load(context, "vkCmdCopyImage");
	table->vkCmdCopyImageToBuffer = (PFN_vkCmdCopyImageToBuffer)load(context, "vkCmdCopyImageToBuffer");
	table->vkCmdCopyQueryPoolResults = (PFN_vkCmdCopyQueryPoolResults)load(context, "vkCmdCopyQueryPoolResults");
	table->vkCmdDispatch = (PFN_vkCmdDispatch)load(context, "vkCmdDispatch");
	table->vkCmdDispatchIndirect = (PFN_vkCmdDispatchIndirect)load(context, "vkCmdDispatchIndirect");
	table->vkCmdDraw = (PFN_vkCmdDraw)load(context, "vkCmdDraw");
	table->vkCmdDrawIndexed = (PFN_vkCmdDrawIndexed)load(context, "vkCmdDrawIndexed");
	table->vkCmdDrawIndexedIndirect = (PFN_vkCmdDrawIndexedIndirect)load(context, "vkCmdDrawIndexedIndirect");
	table->vkCmdDrawIndirect = (PFN_vkCmdDrawIndirect)load(context, "vkCmdDrawIndirect");
	table->vkCmdEndQuery = (PFN_vkCmdEndQuery)load(context, "vkCmdEndQuery");
	table->vkCmdEndRenderPass = (PFN_vkCmdEndRenderPass)load(context, "vkCmdEndRenderPass");
	table->vkCmdExecuteCommands = (PFN_vkCmdExecuteCommands)load(context, "vkCmdExecuteCommands");
	table->vkCmdFillBuffer = (PFN_vkCmdFillBuffer)load(context, "vkCmdFillBuffer");
	table->vkCmdNextSubpass = (PFN_vkCmdNextSubpass)load(context, "vkCmdNextSubpass");
	table->vkCmdPipelineBarrier = (PFN_vkCmdPipelineBarrier)load(context, "vkCmdPipelineBarrier");
	table->vkCmdPushConstants = (PFN_vkCmdPushConstants)load(context, "vkCmdPushConstants");
	table->vkCmdResetEvent = (PFN_vkCmdResetEvent)load(context, "vkCmdResetEvent");
	table->vkCmdResetQueryPool = (PFN_vkCmdResetQueryPool)load(context, "vkCmdResetQueryPool");
	table->vkCmdResolveImage = (PFN_vkCmdResolveImage)load(context, "vkCmdResolveImage");
	table->vkCmdSetBlendConstants = (PFN_vkCmdSetBlendConstants)load(context, "vkCmdSetBlendConstants");
	table->vkCmdSetDepthBias = (PFN_vkCmdSetDepthBias)load(context, "vkCmdSetDepthBias");
	table->vkCmdSetDepthBounds = (PFN_vkCmdSetDepthBounds)load(context, "vkCmdSetDepthBounds");
	table->vkCmdSetEvent = (PFN_vkCmdSetEvent)load(context, "vkCmdSetEvent");
	table->vkCmdSetLineWidth = (PFN_vkCmdSetLineWidth)load(context, "vkCmdSetLineWidth");
	table->vkCmdSetScissor = (PFN_vkCmdSetScissor)load(context, "vkCmdSetScissor");
	table->vkCmdSetStencilCompareMask = (PFN_vkCmdSetStencilCompareMask)load(context, "vkCmdSetStencilCompareMask");
	table->vkCmdSetStencilReference = (PFN_vkCmdSetStencilReference)load(context, "vkCmdSetStencilReference");
	table->vkCmdSetStencilWriteMask = (PFN_vkCmdSetStencilWriteMask)load(context, "vkCmdSetStencilWriteMask");
	table->vkCmdSetViewport = (PFN_vkCmdSetViewport)load(context, "vkCmdSetViewport");
	table->vkCmdUpdateBuffer = (PFN_vkCmdUpdateBuffer)load(context, "vkCmdUpdateBuffer");
	table->vkCmdWaitEvents = (PFN_vkCmdWaitEvents)load(context, "vkCmdWaitEvents");
	table->vkCmdWriteTimestamp = (PFN_vkCmdWriteTimestamp)load(context, "vkCmdWriteTimestamp");
	table->vkCreateBuffer = (PFN_vkCreateBuffer)load(context, "vkCreateBuffer");
	table->vkCreateBufferView = (PFN_vkCreateBufferView)load(context, "vkCreateBufferView");
	table->vkCreateCommandPool = (PFN_vkCreateCommandPool)load(context, "vkCreateCommandPool");
	table->vkCreateComputePipelines = (PFN_vkCreateComputePipelines)load(context, "vkCreateComputePipelines");
	table->vkCreateDescriptorPool = (PFN_vkCreateDescriptorPool)load(context, "vkCreateDescriptorPool");
	table->vkCreateDescriptorSetLayout = (PFN_vkCreateDescriptorSetLayout)load(context, "vkCreateDescriptorSetLayout");
	table->vkCreateEvent = (PFN_vkCreateEvent)load(context, "vkCreateEvent");
	table->vkCreateFence = (PFN_vkCreateFence)load(context, "vkCreateFence");
	table->vkCreateFramebuffer = (PFN_vkCreateFramebuffer)load(context, "vkCreateFramebuffer");
	table->vkCreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines)load(context, "vkCreateGraphicsPipelines");
	table->vkCreateImage = (PFN_vkCreateImage)load(context, "vkCreateImage");
	table->vkCreateImageView = (PFN_vkCreateImageView)load(context, "vkCreateImageView");
	table->vkCreatePipelineCache = (PFN_vkCreatePipelineCache)load(context, "vkCreatePipelineCache");
	table->vkCreatePipelineLayout = (PFN_vkCreatePipelineLayout)load(context, "vkCreatePipelineLayout");
	table->vkCreateQueryPool = (PFN_vkCreateQueryPool)load(context, "vkCreateQueryPool");
	table->vkCreateRenderPass = (PFN_vkCreateRenderPass)load(context, "vkCreateRenderPass");
	table->vkCreateSampler = (PFN_vkCreateSampler)load(context, "vkCreateSampler");
	table->vkCreateSemaphore = (PFN_vkCreateSemaphore)load(context, "vkCreateSemaphore");
	table->vkCreateShaderModule = (PFN_vkCreateShaderModule)load(context, "vkCreateShaderModule");
	table->vkDestroyBuffer = (PFN_vkDestroyBuffer)load(context, "vkDestroyBuffer");
	table->vkDestroyBufferView = (PFN_vkDestroyBufferView)load(context, "vkDestroyBufferView");
	table->vkDestroyCommandPool = (PFN_vkDestroyCommandPool)load(context, "vkDestroyCommandPool");
	table->vkDestroyDescriptorPool = (PFN_vkDestroyDescriptorPool)load(context, "vkDestroyDescriptorPool");
	table->vkDestroyDescriptorSetLayout = (PFN_vkDestroyDescriptorSetLayout)load(context, "vkDestroyDescriptorSetLayout");
	table->vkDestroyDevice = (PFN_vkDestroyDevice)load(context, "vkDestroyDevice");
	table->vkDestroyEvent = (PFN_vkDestroyEvent)load(context, "vkDestroyEvent");
	table->vkDestroyFence = (PFN_vkDestroyFence)load(context, "vkDestroyFence");
	table->vkDestroyFramebuffer = (PFN_vkDestroyFramebuffer)load(context, "vkDestroyFramebuffer");
	table->vkDestroyImage = (PFN_vkDestroyImage)load(context, "vkDestroyImage");
	table->vkDestroyImageView = (PFN_vkDestroyImageView)load(context, "vkDestroyImageView");
	table->vkDestroyPipeline = (PFN_vkDestroyPipeline)load(context, "vkDestroyPipeline");
	table->vkDestroyPipelineCache = (PFN_vkDestroyPipelineCache)load(context, "vkDestroyPipelineCache");
	table->vkDestroyPipelineLayout = (PFN_vkDestroyPipelineLayout)load(context, "vkDestroyPipelineLayout");
	table->vkDestroyQueryPool = (PFN_vkDestroyQueryPool)load(context, "vkDestroyQueryPool");
	table->vkDestroyRenderPass = (PFN_vkDestroyRenderPass)load(context, "vkDestroyRenderPass");
	table->vkDestroySampler = (PFN_vkDestroySampler)load(context, "vkDestroySampler");
	table->vkDestroySemaphore = (PFN_vkDestroySemaphore)load(context, "vkDestroySemaphore");
	table->vkDestroyShaderModule = (PFN_vkDestroyShaderModule)load(context, "vkDestroyShaderModule");
	table->vkDeviceWaitIdle = (PFN_vkDeviceWaitIdle)load(context, "vkDeviceWaitIdle");
	table->vkEndCommandBuffer = (PFN_vkEndCommandBuffer)load(context, "vkEndCommandBuffer");
	table->vkFlushMappedMemoryRanges = (PFN_vkFlushMappedMemoryRanges)load(context, "vkFlushMappedMemoryRanges");
	table->vkFreeCommandBuffers = (PFN_vkFreeCommandBuffers)load(context, "vkFreeCommandBuffers");
	table->vkFreeDescriptorSets = (PFN_vkFreeDescriptorSets)load(context, "vkFreeDescriptorSets");
	table->vkFreeMemory = (PFN_vkFreeMemory)load(context, "vkFreeMemory");
	table->vkGetBufferMemoryRequirements = (PFN_vkGetBufferMemoryRequirements)load(context, "vkGetBufferMemoryRequirements");
	table->vkGetDeviceMemoryCommitment = (PFN_vkGetDeviceMemoryCommitment)load(context, "vkGetDeviceMemoryCommitment");
	table->vkGetDeviceQueue = (PFN_vkGetDeviceQueue)load(context, "vkGetDeviceQueue");
	table->vkGetEventStatus = (PFN_vkGetEventStatus)load(context, "vkGetEventStatus");
	table->vkGetFenceStatus = (PFN_vkGetFenceStatus)load(context, "vkGetFenceStatus");
	table->vkGetImageMemoryRequirements = (PFN_vkGetImageMemoryRequirements)load(context, "vkGetImageMemoryRequirements");
	table->vkGetImageSparseMemoryRequirements = (PFN_vkGetImageSparseMemoryRequirements)load(context, "vkGetImageSparseMemoryRequirements");
	table->vkGetImageSubresourceLayout = (PFN_vkGetImageSubresourceLayout)load(context, "vkGetImageSubresourceLayout");
	table->vkGetPipelineCacheData = (PFN_vkGetPipelineCacheData)load(context, "vkGetPipelineCacheData");
	table->vkGetQueryPoolResults = (PFN_vkGetQueryPoolResults)load(context, "vkGetQueryPoolResults");
	table->vkGetRenderAreaGranularity = (PFN_vkGetRenderAreaGranularity)load(context, "vkGetRenderAreaGranularity");
	table->vkInvalidateMappedMemoryRanges = (PFN_vkInvalidateMappedMemoryRanges)load(context, "vkInvalidateMappedMemoryRanges");
	table->vkMapMemory = (PFN_vkMapMemory)load(context, "vkMapMemory");
	table->vkMergePipelineCaches = (PFN_vkMergePipelineCaches)load(context, "vkMergePipelineCaches");
	table->vkQueueBindSparse = (PFN_vkQueueBindSparse)load(context, "vkQueueBindSparse");
	table->vkQueueSubmit = (PFN_vkQueueSubmit)load(context, "vkQueueSubmit");
	table->vkQueueWaitIdle = (PFN_vkQueueWaitIdle)load(context, "vkQueueWaitIdle");
	table->vkResetCommandBuffer = (PFN_vkResetCommandBuffer)load(context, "vkResetCommandBuffer");
	table->vkResetCommandPool = (PFN_vkResetCommandPool)load(context, "vkResetCommandPool");
	table->vkResetDescriptorPool = (PFN_vkResetDescriptorPool)load(context, "vkResetDescriptorPool");
	table->vkResetEvent = (PFN_vkResetEvent)load(context, "vkResetEvent");
	table->vkResetFences = (PFN_vkResetFences)load(context, "vkResetFences");
	table->vkSetEvent = (PFN_vkSetEvent)load(context, "vkSetEvent");
	table->vkUnmapMemory = (PFN_vkUnmapMemory)load(context, "vkUnmapMemory");
	table->vkUpdateDescriptorSets = (PFN_vkUpdateDescriptorSets)load(context, "vkUpdateDescriptorSets");
	table->vkWaitForFences = (PFN_vkWaitForFences)load(context, "vkWaitForFences");
#endif /* defined(VK_VERSION_1_0) */
#if defined(VK_VERSION_1_1)
	table->vkBindBufferMemory2 = (PFN_vkBindBufferMemory2)load(context, "vkBindBufferMemory2");
	table->vkBindImageMemory2 = (PFN_vkBindImageMemory2)load(context, "vkBindImageMemory2");
	table->vkCmdDispatchBase = (PFN_vkCmdDispatchBase)load(context, "vkCmdDispatchBase");
	table->vkCmdSetDeviceMask = (PFN_vkCmdSetDeviceMask)load(context, "vkCmdSetDeviceMask");
	table->vkCreateDescriptorUpdateTemplate = (PFN_vkCreateDescriptorUpdateTemplate)load(context, "vkCreateDescriptorUpdateTemplate");
	table->vkCreateSamplerYcbcrConversion = (PFN_vkCreateSamplerYcbcrConversion)load(context, "vkCreateSamplerYcbcrConversion");
	table->vkDestroyDescriptorUpdateTemplate = (PFN_vkDestroyDescriptorUpdateTemplate)load(context, "vkDestroyDescriptorUpdateTemplate");
	table->vkDestroySamplerYcbcrConversion = (PFN_vkDestroySamplerYcbcrConversion)load(context, "vkDestroySamplerYcbcrConversion");
	table->vkGetBufferMemoryRequirements2 = (PFN_vkGetBufferMemoryRequirements2)load(context, "vkGetBufferMemoryRequirements2");
	table->vkGetDescriptorSetLayoutSupport = (PFN_vkGetDescriptorSetLayoutSupport)load(context, "vkGetDescriptorSetLayoutSupport");
	table->vkGetDeviceGroupPeerMemoryFeatures = (PFN_vkGetDeviceGroupPeerMemoryFeatures)load(context, "vkGetDeviceGroupPeerMemoryFeatures");
	table->vkGetDeviceQueue2 = (PFN_vkGetDeviceQueue2)load(context, "vkGetDeviceQueue2");
	table->vkGetImageMemoryRequirements2 = (PFN_vkGetImageMemoryRequirements2)load(context, "vkGetImageMemoryRequirements2");
	table->vkGetImageSparseMemoryRequirements2 = (PFN_vkGetImageSparseMemoryRequirements2)load(context, "vkGetImageSparseMemoryRequirements2");
	table->vkTrimCommandPool = (PFN_vkTrimCommandPool)load(context, "vkTrimCommandPool");
	table->vkUpdateDescriptorSetWithTemplate = (PFN_vkUpdateDescriptorSetWithTemplate)load(context, "vkUpdateDescriptorSetWithTemplate");
#endif /* defined(VK_VERSION_1_1) */
#if defined(VK_AMD_buffer_marker)
	table->vkCmdWriteBufferMarkerAMD = (PFN_vkCmdWriteBufferMarkerAMD)load(context, "vkCmdWriteBufferMarkerAMD");
#endif /* defined(VK_AMD_buffer_marker) */
#if defined(VK_AMD_draw_indirect_count)
	table->vkCmdDrawIndexedIndirectCountAMD = (PFN_vkCmdDrawIndexedIndirectCountAMD)load(context, "vkCmdDrawIndexedIndirectCountAMD");
	table->vkCmdDrawIndirectCountAMD = (PFN_vkCmdDrawIndirectCountAMD)load(context, "vkCmdDrawIndirectCountAMD");
#endif /* defined(VK_AMD_draw_indirect_count) */
#if defined(VK_AMD_shader_info)
	table->vkGetShaderInfoAMD = (PFN_vkGetShaderInfoAMD)load(context, "vkGetShaderInfoAMD");
#endif /* defined(VK_AMD_shader_info) */
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
	table->vkGetAndroidHardwareBufferPropertiesANDROID = (PFN_vkGetAndroidHardwareBufferPropertiesANDROID)load(context, "vkGetAndroidHardwareBufferPropertiesANDROID");
	table->vkGetMemoryAndroidHardwareBufferANDROID = (PFN_vkGetMemoryAndroidHardwareBufferANDROID)load(context, "vkGetMemoryAndroidHardwareBufferANDROID");
#endif /* defined(VK_ANDROID_external_memory_android_hardware_buffer) */
#if defined(VK_ANDROID_native_buffer)
	table->vkAcquireImageANDROID = (PFN_vkAcquireImageANDROID)load(context, "vkAcquireImageANDROID");
	table->vkGetSwapchainGrallocUsageANDROID = (PFN_vkGetSwapchainGrallocUsageANDROID)load(context, "vkGetSwapchainGrallocUsageANDROID");
	table->vkQueueSignalReleaseImageANDROID = (PFN_vkQueueSignalReleaseImageANDROID)load(context, "vkQueueSignalReleaseImageANDROID");
#endif /* defined(VK_ANDROID_native_buffer) */
#if defined(VK_EXT_debug_marker)
	table->vkCmdDebugMarkerBeginEXT = (PFN_vkCmdDebugMarkerBeginEXT)load(context, "vkCmdDebugMarkerBeginEXT");
	table->vkCmdDebugMarkerEndEXT = (PFN_vkCmdDebugMarkerEndEXT)load(context, "vkCmdDebugMarkerEndEXT");
	table->vkCmdDebugMarkerInsertEXT = (PFN_vkCmdDebugMarkerInsertEXT)load(context, "vkCmdDebugMarkerInsertEXT");
	table->vkDebugMarkerSetObjectNameEXT = (PFN_vkDebugMarkerSetObjectNameEXT)load(context, "vkDebugMarkerSetObjectNameEXT");
	table->vkDebugMarkerSetObjectTagEXT = (PFN_vkDebugMarkerSetObjectTagEXT)load(context, "vkDebugMarkerSetObjectTagEXT");
#endif /* defined(VK_EXT_debug_marker) */
#if defined(VK_EXT_debug_utils)
	table->vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)load(context, "vkCmdBeginDebugUtilsLabelEXT");
	table->vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)load(context, "vkCmdEndDebugUtilsLabelEXT");
	table->vkCmdInsertDebugUtilsLabelEXT = (PFN_vkCmdInsertDebugUtilsLabelEXT)load(context, "vkCmdInsertDebugUtilsLabelEXT");
	table->vkQueueBeginDebugUtilsLabelEXT = (PFN_vkQueueBeginDebugUtilsLabelEXT)load(context, "vkQueueBeginDebugUtilsLabelEXT");
	table->vkQueueEndDebugUtilsLabelEXT = (PFN_vkQueueEndDebugUtilsLabelEXT)load(context, "vkQueueEndDebugUtilsLabelEXT");
	table->vkQueueInsertDebugUtilsLabelEXT = (PFN_vkQueueInsertDebugUtilsLabelEXT)load(context, "vkQueueInsertDebugUtilsLabelEXT");
	table->vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)load(context, "vkSetDebugUtilsObjectNameEXT");
	table->vkSetDebugUtilsObjectTagEXT = (PFN_vkSetDebugUtilsObjectTagEXT)load(context, "vkSetDebugUtilsObjectTagEXT");
#endif /* defined(VK_EXT_debug_utils) */
#if defined(VK_EXT_discard_rectangles)
	table->vkCmdSetDiscardRectangleEXT = (PFN_vkCmdSetDiscardRectangleEXT)load(context, "vkCmdSetDiscardRectangleEXT");
#endif /* defined(VK_EXT_discard_rectangles) */
#if defined(VK_EXT_display_control)
	table->vkDisplayPowerControlEXT = (PFN_vkDisplayPowerControlEXT)load(context, "vkDisplayPowerControlEXT");
	table->vkGetSwapchainCounterEXT = (PFN_vkGetSwapchainCounterEXT)load(context, "vkGetSwapchainCounterEXT");
	table->vkRegisterDeviceEventEXT = (PFN_vkRegisterDeviceEventEXT)load(context, "vkRegisterDeviceEventEXT");
	table->vkRegisterDisplayEventEXT = (PFN_vkRegisterDisplayEventEXT)load(context, "vkRegisterDisplayEventEXT");
#endif /* defined(VK_EXT_display_control) */
#if defined(VK_EXT_external_memory_host)
	table->vkGetMemoryHostPointerPropertiesEXT = (PFN_vkGetMemoryHostPointerPropertiesEXT)load(context, "vkGetMemoryHostPointerPropertiesEXT");
#endif /* defined(VK_EXT_external_memory_host) */
#if defined(VK_EXT_hdr_metadata)
	table->vkSetHdrMetadataEXT = (PFN_vkSetHdrMetadataEXT)load(context, "vkSetHdrMetadataEXT");
#endif /* defined(VK_EXT_hdr_metadata) */
#if defined(VK_EXT_sample_locations)
	table->vkCmdSetSampleLocationsEXT = (PFN_vkCmdSetSampleLocationsEXT)load(context, "vkCmdSetSampleLocationsEXT");
#endif /* defined(VK_EXT_sample_locations) */
#if defined(VK_EXT_validation_cache)
	table->vkCreateValidationCacheEXT = (PFN_vkCreateValidationCacheEXT)load(context, "vkCreateValidationCacheEXT");
	table->vkDestroyValidationCacheEXT = (PFN_vkDestroyValidationCacheEXT)load(context, "vkDestroyValidationCacheEXT");
	table->vkGetValidationCacheDataEXT = (PFN_vkGetValidationCacheDataEXT)load(context, "vkGetValidationCacheDataEXT");
	table->vkMergeValidationCachesEXT = (PFN_vkMergeValidationCachesEXT)load(context, "vkMergeValidationCachesEXT");
#endif /* defined(VK_EXT_validation_cache) */
#if defined(VK_GOOGLE_display_timing)
	table->vkGetPastPresentationTimingGOOGLE = (PFN_vkGetPastPresentationTimingGOOGLE)load(context, "vkGetPastPresentationTimingGOOGLE");
	table->vkGetRefreshCycleDurationGOOGLE = (PFN_vkGetRefreshCycleDurationGOOGLE)load(context, "vkGetRefreshCycleDurationGOOGLE");
#endif /* defined(VK_GOOGLE_display_timing) */
#if defined(VK_KHR_bind_memory2)
	table->vkBindBufferMemory2KHR = (PFN_vkBindBufferMemory2KHR)load(context, "vkBindBufferMemory2KHR");
	table->vkBindImageMemory2KHR = (PFN_vkBindImageMemory2KHR)load(context, "vkBindImageMemory2KHR");
#endif /* defined(VK_KHR_bind_memory2) */
#if defined(VK_KHR_descriptor_update_template)
	table->vkCreateDescriptorUpdateTemplateKHR = (PFN_vkCreateDescriptorUpdateTemplateKHR)load(context, "vkCreateDescriptorUpdateTemplateKHR");
	table->vkDestroyDescriptorUpdateTemplateKHR = (PFN_vkDestroyDescriptorUpdateTemplateKHR)load(context, "vkDestroyDescriptorUpdateTemplateKHR");
	table->vkUpdateDescriptorSetWithTemplateKHR = (PFN_vkUpdateDescriptorSetWithTemplateKHR)load(context, "vkUpdateDescriptorSetWithTemplateKHR");
#endif /* defined(VK_KHR_descriptor_update_template) */
#if defined(VK_KHR_device_group)
	table->vkCmdDispatchBaseKHR = (PFN_vkCmdDispatchBaseKHR)load(context, "vkCmdDispatchBaseKHR");
	table->vkCmdSetDeviceMaskKHR = (PFN_vkCmdSetDeviceMaskKHR)load(context, "vkCmdSetDeviceMaskKHR");
	table->vkGetDeviceGroupPeerMemoryFeaturesKHR = (PFN_vkGetDeviceGroupPeerMemoryFeaturesKHR)load(context, "vkGetDeviceGroupPeerMemoryFeaturesKHR");
#endif /* defined(VK_KHR_device_group) */
#if defined(VK_KHR_display_swapchain)
	table->vkCreateSharedSwapchainsKHR = (PFN_vkCreateSharedSwapchainsKHR)load(context, "vkCreateSharedSwapchainsKHR");
#endif /* defined(VK_KHR_display_swapchain) */
#if defined(VK_KHR_draw_indirect_count)
	table->vkCmdDrawIndexedIndirectCountKHR = (PFN_vkCmdDrawIndexedIndirectCountKHR)load(context, "vkCmdDrawIndexedIndirectCountKHR");
	table->vkCmdDrawIndirectCountKHR = (PFN_vkCmdDrawIndirectCountKHR)load(context, "vkCmdDrawIndirectCountKHR");
#endif /* defined(VK_KHR_draw_indirect_count) */
#if defined(VK_KHR_external_fence_fd)
	table->vkGetFenceFdKHR = (PFN_vkGetFenceFdKHR)load(context, "vkGetFenceFdKHR");
	table->vkImportFenceFdKHR = (PFN_vkImportFenceFdKHR)load(context, "vkImportFenceFdKHR");
#endif /* defined(VK_KHR_external_fence_fd) */
#if defined(VK_KHR_external_fence_win32)
	table->vkGetFenceWin32HandleKHR = (PFN_vkGetFenceWin32HandleKHR)load(context, "vkGetFenceWin32HandleKHR");
	table->vkImportFenceWin32HandleKHR = (PFN_vkImportFenceWin32HandleKHR)load(context, "vkImportFenceWin32HandleKHR");
#endif /* defined(VK_KHR_external_fence_win32) */
#if defined(VK_KHR_external_memory_fd)
	table->vkGetMemoryFdKHR = (PFN_vkGetMemoryFdKHR)load(context, "vkGetMemoryFdKHR");
	table->vkGetMemoryFdPropertiesKHR = (PFN_vkGetMemoryFdPropertiesKHR)load(context, "vkGetMemoryFdPropertiesKHR");
#endif /* defined(VK_KHR_external_memory_fd) */
#if defined(VK_KHR_external_memory_win32)
	table->vkGetMemoryWin32HandleKHR = (PFN_vkGetMemoryWin32HandleKHR)load(context, "vkGetMemoryWin32HandleKHR");
	table->vkGetMemoryWin32HandlePropertiesKHR = (PFN_vkGetMemoryWin32HandlePropertiesKHR)load(context, "vkGetMemoryWin32HandlePropertiesKHR");
#endif /* defined(VK_KHR_external_memory_win32) */
#if defined(VK_KHR_external_semaphore_fd)
	table->vkGetSemaphoreFdKHR = (PFN_vkGetSemaphoreFdKHR)load(context, "vkGetSemaphoreFdKHR");
	table->vkImportSemaphoreFdKHR = (PFN_vkImportSemaphoreFdKHR)load(context, "vkImportSemaphoreFdKHR");
#endif /* defined(VK_KHR_external_semaphore_fd) */
#if defined(VK_KHR_external_semaphore_win32)
	table->vkGetSemaphoreWin32HandleKHR = (PFN_vkGetSemaphoreWin32HandleKHR)load(context, "vkGetSemaphoreWin32HandleKHR");
	table->vkImportSemaphoreWin32HandleKHR = (PFN_vkImportSemaphoreWin32HandleKHR)load(context, "vkImportSemaphoreWin32HandleKHR");
#endif /* defined(VK_KHR_external_semaphore_win32) */
#if defined(VK_KHR_get_memory_requirements2)
	table->vkGetBufferMemoryRequirements2KHR = (PFN_vkGetBufferMemoryRequirements2KHR)load(context, "vkGetBufferMemoryRequirements2KHR");
	table->vkGetImageMemoryRequirements2KHR = (PFN_vkGetImageMemoryRequirements2KHR)load(context, "vkGetImageMemoryRequirements2KHR");
	table->vkGetImageSparseMemoryRequirements2KHR = (PFN_vkGetImageSparseMemoryRequirements2KHR)load(context, "vkGetImageSparseMemoryRequirements2KHR");
#endif /* defined(VK_KHR_get_memory_requirements2) */
#if defined(VK_KHR_maintenance1)
	table->vkTrimCommandPoolKHR = (PFN_vkTrimCommandPoolKHR)load(context, "vkTrimCommandPoolKHR");
#endif /* defined(VK_KHR_maintenance1) */
#if defined(VK_KHR_maintenance3)
	table->vkGetDescriptorSetLayoutSupportKHR = (PFN_vkGetDescriptorSetLayoutSupportKHR)load(context, "vkGetDescriptorSetLayoutSupportKHR");
#endif /* defined(VK_KHR_maintenance3) */
#if defined(VK_KHR_push_descriptor)
	table->vkCmdPushDescriptorSetKHR = (PFN_vkCmdPushDescriptorSetKHR)load(context, "vkCmdPushDescriptorSetKHR");
#endif /* defined(VK_KHR_push_descriptor) */
#if defined(VK_KHR_sampler_ycbcr_conversion)
	table->vkCreateSamplerYcbcrConversionKHR = (PFN_vkCreateSamplerYcbcrConversionKHR)load(context, "vkCreateSamplerYcbcrConversionKHR");
	table->vkDestroySamplerYcbcrConversionKHR = (PFN_vkDestroySamplerYcbcrConversionKHR)load(context, "vkDestroySamplerYcbcrConversionKHR");
#endif /* defined(VK_KHR_sampler_ycbcr_conversion) */
#if defined(VK_KHR_shared_presentable_image)
	table->vkGetSwapchainStatusKHR = (PFN_vkGetSwapchainStatusKHR)load(context, "vkGetSwapchainStatusKHR");
#endif /* defined(VK_KHR_shared_presentable_image) */
#if defined(VK_KHR_swapchain)
	table->vkAcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)load(context, "vkAcquireNextImageKHR");
	table->vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)load(context, "vkCreateSwapchainKHR");
	table->vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)load(context, "vkDestroySwapchainKHR");
	table->vkGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)load(context, "vkGetSwapchainImagesKHR");
	table->vkQueuePresentKHR = (PFN_vkQueuePresentKHR)load(context, "vkQueuePresentKHR");
#endif /* defined(VK_KHR_swapchain) */
#if defined(VK_NVX_device_generated_commands)
	table->vkCmdProcessCommandsNVX = (PFN_vkCmdProcessCommandsNVX)load(context, "vkCmdProcessCommandsNVX");
	table->vkCmdReserveSpaceForCommandsNVX = (PFN_vkCmdReserveSpaceForCommandsNVX)load(context, "vkCmdReserveSpaceForCommandsNVX");
	table->vkCreateIndirectCommandsLayoutNVX = (PFN_vkCreateIndirectCommandsLayoutNVX)load(context, "vkCreateIndirectCommandsLayoutNVX");
	table->vkCreateObjectTableNVX = (PFN_vkCreateObjectTableNVX)load(context, "vkCreateObjectTableNVX");
	table->vkDestroyIndirectCommandsLayoutNVX = (PFN_vkDestroyIndirectCommandsLayoutNVX)load(context, "vkDestroyIndirectCommandsLayoutNVX");
	table->vkDestroyObjectTableNVX = (PFN_vkDestroyObjectTableNVX)load(context, "vkDestroyObjectTableNVX");
	table->vkRegisterObjectsNVX = (PFN_vkRegisterObjectsNVX)load(context, "vkRegisterObjectsNVX");
	table->vkUnregisterObjectsNVX = (PFN_vkUnregisterObjectsNVX)load(context, "vkUnregisterObjectsNVX");
#endif /* defined(VK_NVX_device_generated_commands) */
#if defined(VK_NV_clip_space_w_scaling)
	table->vkCmdSetViewportWScalingNV = (PFN_vkCmdSetViewportWScalingNV)load(context, "vkCmdSetViewportWScalingNV");
#endif /* defined(VK_NV_clip_space_w_scaling) */
#if defined(VK_NV_external_memory_win32)
	table->vkGetMemoryWin32HandleNV = (PFN_vkGetMemoryWin32HandleNV)load(context, "vkGetMemoryWin32HandleNV");
#endif /* defined(VK_NV_external_memory_win32) */
#if (defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor)) || (defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1))
	table->vkCmdPushDescriptorSetWithTemplateKHR = (PFN_vkCmdPushDescriptorSetWithTemplateKHR)load(context, "vkCmdPushDescriptorSetWithTemplateKHR");
#endif /* (defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor)) || (defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1)) */
#if (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
	table->vkGetDeviceGroupPresentCapabilitiesKHR = (PFN_vkGetDeviceGroupPresentCapabilitiesKHR)load(context, "vkGetDeviceGroupPresentCapabilitiesKHR");
	table->vkGetDeviceGroupSurfacePresentModesKHR = (PFN_vkGetDeviceGroupSurfacePresentModesKHR)load(context, "vkGetDeviceGroupSurfacePresentModesKHR");
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */
#if (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
	table->vkAcquireNextImage2KHR = (PFN_vkAcquireNextImage2KHR)load(context, "vkAcquireNextImage2KHR");
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */
	/* VOLK_GENERATE_LOAD_DEVICE_TABLE */
}

#ifdef __GNUC__
#	pragma GCC visibility push(hidden)
#endif

/* VOLK_GENERATE_PROTOTYPES_C */
#if defined(VK_VERSION_1_0)
PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers = 0;
PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets = 0;
PFN_vkAllocateMemory vkAllocateMemory = 0;
PFN_vkBeginCommandBuffer vkBeginCommandBuffer = 0;
PFN_vkBindBufferMemory vkBindBufferMemory = 0;
PFN_vkBindImageMemory vkBindImageMemory = 0;
PFN_vkCmdBeginQuery vkCmdBeginQuery = 0;
PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass = 0;
PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets = 0;
PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer = 0;
PFN_vkCmdBindPipeline vkCmdBindPipeline = 0;
PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers = 0;
PFN_vkCmdBlitImage vkCmdBlitImage = 0;
PFN_vkCmdClearAttachments vkCmdClearAttachments = 0;
PFN_vkCmdClearColorImage vkCmdClearColorImage = 0;
PFN_vkCmdClearDepthStencilImage vkCmdClearDepthStencilImage = 0;
PFN_vkCmdCopyBuffer vkCmdCopyBuffer = 0;
PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage = 0;
PFN_vkCmdCopyImage vkCmdCopyImage = 0;
PFN_vkCmdCopyImageToBuffer vkCmdCopyImageToBuffer = 0;
PFN_vkCmdCopyQueryPoolResults vkCmdCopyQueryPoolResults = 0;
PFN_vkCmdDispatch vkCmdDispatch = 0;
PFN_vkCmdDispatchIndirect vkCmdDispatchIndirect = 0;
PFN_vkCmdDraw vkCmdDraw = 0;
PFN_vkCmdDrawIndexed vkCmdDrawIndexed = 0;
PFN_vkCmdDrawIndexedIndirect vkCmdDrawIndexedIndirect = 0;
PFN_vkCmdDrawIndirect vkCmdDrawIndirect = 0;
PFN_vkCmdEndQuery vkCmdEndQuery = 0;
PFN_vkCmdEndRenderPass vkCmdEndRenderPass = 0;
PFN_vkCmdExecuteCommands vkCmdExecuteCommands = 0;
PFN_vkCmdFillBuffer vkCmdFillBuffer = 0;
PFN_vkCmdNextSubpass vkCmdNextSubpass = 0;
PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier = 0;
PFN_vkCmdPushConstants vkCmdPushConstants = 0;
PFN_vkCmdResetEvent vkCmdResetEvent = 0;
PFN_vkCmdResetQueryPool vkCmdResetQueryPool = 0;
PFN_vkCmdResolveImage vkCmdResolveImage = 0;
PFN_vkCmdSetBlendConstants vkCmdSetBlendConstants = 0;
PFN_vkCmdSetDepthBias vkCmdSetDepthBias = 0;
PFN_vkCmdSetDepthBounds vkCmdSetDepthBounds = 0;
PFN_vkCmdSetEvent vkCmdSetEvent = 0;
PFN_vkCmdSetLineWidth vkCmdSetLineWidth = 0;
PFN_vkCmdSetScissor vkCmdSetScissor = 0;
PFN_vkCmdSetStencilCompareMask vkCmdSetStencilCompareMask = 0;
PFN_vkCmdSetStencilReference vkCmdSetStencilReference = 0;
PFN_vkCmdSetStencilWriteMask vkCmdSetStencilWriteMask = 0;
PFN_vkCmdSetViewport vkCmdSetViewport = 0;
PFN_vkCmdUpdateBuffer vkCmdUpdateBuffer = 0;
PFN_vkCmdWaitEvents vkCmdWaitEvents = 0;
PFN_vkCmdWriteTimestamp vkCmdWriteTimestamp = 0;
PFN_vkCreateBuffer vkCreateBuffer = 0;
PFN_vkCreateBufferView vkCreateBufferView = 0;
PFN_vkCreateCommandPool vkCreateCommandPool = 0;
PFN_vkCreateComputePipelines vkCreateComputePipelines = 0;
PFN_vkCreateDescriptorPool vkCreateDescriptorPool = 0;
PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout = 0;
PFN_vkCreateDevice vkCreateDevice = 0;
PFN_vkCreateEvent vkCreateEvent = 0;
PFN_vkCreateFence vkCreateFence = 0;
PFN_vkCreateFramebuffer vkCreateFramebuffer = 0;
PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines = 0;
PFN_vkCreateImage vkCreateImage = 0;
PFN_vkCreateImageView vkCreateImageView = 0;
PFN_vkCreateInstance vkCreateInstance = 0;
PFN_vkCreatePipelineCache vkCreatePipelineCache = 0;
PFN_vkCreatePipelineLayout vkCreatePipelineLayout = 0;
PFN_vkCreateQueryPool vkCreateQueryPool = 0;
PFN_vkCreateRenderPass vkCreateRenderPass = 0;
PFN_vkCreateSampler vkCreateSampler = 0;
PFN_vkCreateSemaphore vkCreateSemaphore = 0;
PFN_vkCreateShaderModule vkCreateShaderModule = 0;
PFN_vkDestroyBuffer vkDestroyBuffer = 0;
PFN_vkDestroyBufferView vkDestroyBufferView = 0;
PFN_vkDestroyCommandPool vkDestroyCommandPool = 0;
PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool = 0;
PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout = 0;
PFN_vkDestroyDevice vkDestroyDevice = 0;
PFN_vkDestroyEvent vkDestroyEvent = 0;
PFN_vkDestroyFence vkDestroyFence = 0;
PFN_vkDestroyFramebuffer vkDestroyFramebuffer = 0;
PFN_vkDestroyImage vkDestroyImage = 0;
PFN_vkDestroyImageView vkDestroyImageView = 0;
PFN_vkDestroyInstance vkDestroyInstance = 0;
PFN_vkDestroyPipeline vkDestroyPipeline = 0;
PFN_vkDestroyPipelineCache vkDestroyPipelineCache = 0;
PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout = 0;
PFN_vkDestroyQueryPool vkDestroyQueryPool = 0;
PFN_vkDestroyRenderPass vkDestroyRenderPass = 0;
PFN_vkDestroySampler vkDestroySampler = 0;
PFN_vkDestroySemaphore vkDestroySemaphore = 0;
PFN_vkDestroyShaderModule vkDestroyShaderModule = 0;
PFN_vkDeviceWaitIdle vkDeviceWaitIdle = 0;
PFN_vkEndCommandBuffer vkEndCommandBuffer = 0;
PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties = 0;
PFN_vkEnumerateDeviceLayerProperties vkEnumerateDeviceLayerProperties = 0;
PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties = 0;
PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties = 0;
PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices = 0;
PFN_vkFlushMappedMemoryRanges vkFlushMappedMemoryRanges = 0;
PFN_vkFreeCommandBuffers vkFreeCommandBuffers = 0;
PFN_vkFreeDescriptorSets vkFreeDescriptorSets = 0;
PFN_vkFreeMemory vkFreeMemory = 0;
PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements = 0;
PFN_vkGetDeviceMemoryCommitment vkGetDeviceMemoryCommitment = 0;
PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr = 0;
PFN_vkGetDeviceQueue vkGetDeviceQueue = 0;
PFN_vkGetEventStatus vkGetEventStatus = 0;
PFN_vkGetFenceStatus vkGetFenceStatus = 0;
PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements = 0;
PFN_vkGetImageSparseMemoryRequirements vkGetImageSparseMemoryRequirements = 0;
PFN_vkGetImageSubresourceLayout vkGetImageSubresourceLayout = 0;
PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = 0;
PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures = 0;
PFN_vkGetPhysicalDeviceFormatProperties vkGetPhysicalDeviceFormatProperties = 0;
PFN_vkGetPhysicalDeviceImageFormatProperties vkGetPhysicalDeviceImageFormatProperties = 0;
PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties = 0;
PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties = 0;
PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties = 0;
PFN_vkGetPhysicalDeviceSparseImageFormatProperties vkGetPhysicalDeviceSparseImageFormatProperties = 0;
PFN_vkGetPipelineCacheData vkGetPipelineCacheData = 0;
PFN_vkGetQueryPoolResults vkGetQueryPoolResults = 0;
PFN_vkGetRenderAreaGranularity vkGetRenderAreaGranularity = 0;
PFN_vkInvalidateMappedMemoryRanges vkInvalidateMappedMemoryRanges = 0;
PFN_vkMapMemory vkMapMemory = 0;
PFN_vkMergePipelineCaches vkMergePipelineCaches = 0;
PFN_vkQueueBindSparse vkQueueBindSparse = 0;
PFN_vkQueueSubmit vkQueueSubmit = 0;
PFN_vkQueueWaitIdle vkQueueWaitIdle = 0;
PFN_vkResetCommandBuffer vkResetCommandBuffer = 0;
PFN_vkResetCommandPool vkResetCommandPool = 0;
PFN_vkResetDescriptorPool vkResetDescriptorPool = 0;
PFN_vkResetEvent vkResetEvent = 0;
PFN_vkResetFences vkResetFences = 0;
PFN_vkSetEvent vkSetEvent = 0;
PFN_vkUnmapMemory vkUnmapMemory = 0;
PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets = 0;
PFN_vkWaitForFences vkWaitForFences = 0;
#endif /* defined(VK_VERSION_1_0) */
#if defined(VK_VERSION_1_1)
PFN_vkBindBufferMemory2 vkBindBufferMemory2 = 0;
PFN_vkBindImageMemory2 vkBindImageMemory2 = 0;
PFN_vkCmdDispatchBase vkCmdDispatchBase = 0;
PFN_vkCmdSetDeviceMask vkCmdSetDeviceMask = 0;
PFN_vkCreateDescriptorUpdateTemplate vkCreateDescriptorUpdateTemplate = 0;
PFN_vkCreateSamplerYcbcrConversion vkCreateSamplerYcbcrConversion = 0;
PFN_vkDestroyDescriptorUpdateTemplate vkDestroyDescriptorUpdateTemplate = 0;
PFN_vkDestroySamplerYcbcrConversion vkDestroySamplerYcbcrConversion = 0;
PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion = 0;
PFN_vkEnumeratePhysicalDeviceGroups vkEnumeratePhysicalDeviceGroups = 0;
PFN_vkGetBufferMemoryRequirements2 vkGetBufferMemoryRequirements2 = 0;
PFN_vkGetDescriptorSetLayoutSupport vkGetDescriptorSetLayoutSupport = 0;
PFN_vkGetDeviceGroupPeerMemoryFeatures vkGetDeviceGroupPeerMemoryFeatures = 0;
PFN_vkGetDeviceQueue2 vkGetDeviceQueue2 = 0;
PFN_vkGetImageMemoryRequirements2 vkGetImageMemoryRequirements2 = 0;
PFN_vkGetImageSparseMemoryRequirements2 vkGetImageSparseMemoryRequirements2 = 0;
PFN_vkGetPhysicalDeviceExternalBufferProperties vkGetPhysicalDeviceExternalBufferProperties = 0;
PFN_vkGetPhysicalDeviceExternalFenceProperties vkGetPhysicalDeviceExternalFenceProperties = 0;
PFN_vkGetPhysicalDeviceExternalSemaphoreProperties vkGetPhysicalDeviceExternalSemaphoreProperties = 0;
PFN_vkGetPhysicalDeviceFeatures2 vkGetPhysicalDeviceFeatures2 = 0;
PFN_vkGetPhysicalDeviceFormatProperties2 vkGetPhysicalDeviceFormatProperties2 = 0;
PFN_vkGetPhysicalDeviceImageFormatProperties2 vkGetPhysicalDeviceImageFormatProperties2 = 0;
PFN_vkGetPhysicalDeviceMemoryProperties2 vkGetPhysicalDeviceMemoryProperties2 = 0;
PFN_vkGetPhysicalDeviceProperties2 vkGetPhysicalDeviceProperties2 = 0;
PFN_vkGetPhysicalDeviceQueueFamilyProperties2 vkGetPhysicalDeviceQueueFamilyProperties2 = 0;
PFN_vkGetPhysicalDeviceSparseImageFormatProperties2 vkGetPhysicalDeviceSparseImageFormatProperties2 = 0;
PFN_vkTrimCommandPool vkTrimCommandPool = 0;
PFN_vkUpdateDescriptorSetWithTemplate vkUpdateDescriptorSetWithTemplate = 0;
#endif /* defined(VK_VERSION_1_1) */
#if defined(VK_AMD_buffer_marker)
PFN_vkCmdWriteBufferMarkerAMD vkCmdWriteBufferMarkerAMD = 0;
#endif /* defined(VK_AMD_buffer_marker) */
#if defined(VK_AMD_draw_indirect_count)
PFN_vkCmdDrawIndexedIndirectCountAMD vkCmdDrawIndexedIndirectCountAMD = 0;
PFN_vkCmdDrawIndirectCountAMD vkCmdDrawIndirectCountAMD = 0;
#endif /* defined(VK_AMD_draw_indirect_count) */
#if defined(VK_AMD_shader_info)
PFN_vkGetShaderInfoAMD vkGetShaderInfoAMD = 0;
#endif /* defined(VK_AMD_shader_info) */
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
PFN_vkGetAndroidHardwareBufferPropertiesANDROID vkGetAndroidHardwareBufferPropertiesANDROID = 0;
PFN_vkGetMemoryAndroidHardwareBufferANDROID vkGetMemoryAndroidHardwareBufferANDROID = 0;
#endif /* defined(VK_ANDROID_external_memory_android_hardware_buffer) */
#if defined(VK_ANDROID_native_buffer)
PFN_vkAcquireImageANDROID vkAcquireImageANDROID = 0;
PFN_vkGetSwapchainGrallocUsageANDROID vkGetSwapchainGrallocUsageANDROID = 0;
PFN_vkQueueSignalReleaseImageANDROID vkQueueSignalReleaseImageANDROID = 0;
#endif /* defined(VK_ANDROID_native_buffer) */
#if defined(VK_EXT_acquire_xlib_display)
PFN_vkAcquireXlibDisplayEXT vkAcquireXlibDisplayEXT = 0;
PFN_vkGetRandROutputDisplayEXT vkGetRandROutputDisplayEXT = 0;
#endif /* defined(VK_EXT_acquire_xlib_display) */
#if defined(VK_EXT_debug_marker)
PFN_vkCmdDebugMarkerBeginEXT vkCmdDebugMarkerBeginEXT = 0;
PFN_vkCmdDebugMarkerEndEXT vkCmdDebugMarkerEndEXT = 0;
PFN_vkCmdDebugMarkerInsertEXT vkCmdDebugMarkerInsertEXT = 0;
PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectNameEXT = 0;
PFN_vkDebugMarkerSetObjectTagEXT vkDebugMarkerSetObjectTagEXT = 0;
#endif /* defined(VK_EXT_debug_marker) */
#if defined(VK_EXT_debug_report)
PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = 0;
PFN_vkDebugReportMessageEXT vkDebugReportMessageEXT = 0;
PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT = 0;
#endif /* defined(VK_EXT_debug_report) */
#if defined(VK_EXT_debug_utils)
PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT = 0;
PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT = 0;
PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT = 0;
PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = 0;
PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = 0;
PFN_vkQueueBeginDebugUtilsLabelEXT vkQueueBeginDebugUtilsLabelEXT = 0;
PFN_vkQueueEndDebugUtilsLabelEXT vkQueueEndDebugUtilsLabelEXT = 0;
PFN_vkQueueInsertDebugUtilsLabelEXT vkQueueInsertDebugUtilsLabelEXT = 0;
PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = 0;
PFN_vkSetDebugUtilsObjectTagEXT vkSetDebugUtilsObjectTagEXT = 0;
PFN_vkSubmitDebugUtilsMessageEXT vkSubmitDebugUtilsMessageEXT = 0;
#endif /* defined(VK_EXT_debug_utils) */
#if defined(VK_EXT_direct_mode_display)
PFN_vkReleaseDisplayEXT vkReleaseDisplayEXT = 0;
#endif /* defined(VK_EXT_direct_mode_display) */
#if defined(VK_EXT_discard_rectangles)
PFN_vkCmdSetDiscardRectangleEXT vkCmdSetDiscardRectangleEXT = 0;
#endif /* defined(VK_EXT_discard_rectangles) */
#if defined(VK_EXT_display_control)
PFN_vkDisplayPowerControlEXT vkDisplayPowerControlEXT = 0;
PFN_vkGetSwapchainCounterEXT vkGetSwapchainCounterEXT = 0;
PFN_vkRegisterDeviceEventEXT vkRegisterDeviceEventEXT = 0;
PFN_vkRegisterDisplayEventEXT vkRegisterDisplayEventEXT = 0;
#endif /* defined(VK_EXT_display_control) */
#if defined(VK_EXT_display_surface_counter)
PFN_vkGetPhysicalDeviceSurfaceCapabilities2EXT vkGetPhysicalDeviceSurfaceCapabilities2EXT = 0;
#endif /* defined(VK_EXT_display_surface_counter) */
#if defined(VK_EXT_external_memory_host)
PFN_vkGetMemoryHostPointerPropertiesEXT vkGetMemoryHostPointerPropertiesEXT = 0;
#endif /* defined(VK_EXT_external_memory_host) */
#if defined(VK_EXT_hdr_metadata)
PFN_vkSetHdrMetadataEXT vkSetHdrMetadataEXT = 0;
#endif /* defined(VK_EXT_hdr_metadata) */
#if defined(VK_EXT_sample_locations)
PFN_vkCmdSetSampleLocationsEXT vkCmdSetSampleLocationsEXT = 0;
PFN_vkGetPhysicalDeviceMultisamplePropertiesEXT vkGetPhysicalDeviceMultisamplePropertiesEXT = 0;
#endif /* defined(VK_EXT_sample_locations) */
#if defined(VK_EXT_validation_cache)
PFN_vkCreateValidationCacheEXT vkCreateValidationCacheEXT = 0;
PFN_vkDestroyValidationCacheEXT vkDestroyValidationCacheEXT = 0;
PFN_vkGetValidationCacheDataEXT vkGetValidationCacheDataEXT = 0;
PFN_vkMergeValidationCachesEXT vkMergeValidationCachesEXT = 0;
#endif /* defined(VK_EXT_validation_cache) */
#if defined(VK_GOOGLE_display_timing)
PFN_vkGetPastPresentationTimingGOOGLE vkGetPastPresentationTimingGOOGLE = 0;
PFN_vkGetRefreshCycleDurationGOOGLE vkGetRefreshCycleDurationGOOGLE = 0;
#endif /* defined(VK_GOOGLE_display_timing) */
#if defined(VK_KHR_android_surface)
PFN_vkCreateAndroidSurfaceKHR vkCreateAndroidSurfaceKHR = 0;
#endif /* defined(VK_KHR_android_surface) */
#if defined(VK_KHR_bind_memory2)
PFN_vkBindBufferMemory2KHR vkBindBufferMemory2KHR = 0;
PFN_vkBindImageMemory2KHR vkBindImageMemory2KHR = 0;
#endif /* defined(VK_KHR_bind_memory2) */
#if defined(VK_KHR_descriptor_update_template)
PFN_vkCreateDescriptorUpdateTemplateKHR vkCreateDescriptorUpdateTemplateKHR = 0;
PFN_vkDestroyDescriptorUpdateTemplateKHR vkDestroyDescriptorUpdateTemplateKHR = 0;
PFN_vkUpdateDescriptorSetWithTemplateKHR vkUpdateDescriptorSetWithTemplateKHR = 0;
#endif /* defined(VK_KHR_descriptor_update_template) */
#if defined(VK_KHR_device_group)
PFN_vkCmdDispatchBaseKHR vkCmdDispatchBaseKHR = 0;
PFN_vkCmdSetDeviceMaskKHR vkCmdSetDeviceMaskKHR = 0;
PFN_vkGetDeviceGroupPeerMemoryFeaturesKHR vkGetDeviceGroupPeerMemoryFeaturesKHR = 0;
#endif /* defined(VK_KHR_device_group) */
#if defined(VK_KHR_device_group_creation)
PFN_vkEnumeratePhysicalDeviceGroupsKHR vkEnumeratePhysicalDeviceGroupsKHR = 0;
#endif /* defined(VK_KHR_device_group_creation) */
#if defined(VK_KHR_display)
PFN_vkCreateDisplayModeKHR vkCreateDisplayModeKHR = 0;
PFN_vkCreateDisplayPlaneSurfaceKHR vkCreateDisplayPlaneSurfaceKHR = 0;
PFN_vkGetDisplayModePropertiesKHR vkGetDisplayModePropertiesKHR = 0;
PFN_vkGetDisplayPlaneCapabilitiesKHR vkGetDisplayPlaneCapabilitiesKHR = 0;
PFN_vkGetDisplayPlaneSupportedDisplaysKHR vkGetDisplayPlaneSupportedDisplaysKHR = 0;
PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR vkGetPhysicalDeviceDisplayPlanePropertiesKHR = 0;
PFN_vkGetPhysicalDeviceDisplayPropertiesKHR vkGetPhysicalDeviceDisplayPropertiesKHR = 0;
#endif /* defined(VK_KHR_display) */
#if defined(VK_KHR_display_swapchain)
PFN_vkCreateSharedSwapchainsKHR vkCreateSharedSwapchainsKHR = 0;
#endif /* defined(VK_KHR_display_swapchain) */
#if defined(VK_KHR_draw_indirect_count)
PFN_vkCmdDrawIndexedIndirectCountKHR vkCmdDrawIndexedIndirectCountKHR = 0;
PFN_vkCmdDrawIndirectCountKHR vkCmdDrawIndirectCountKHR = 0;
#endif /* defined(VK_KHR_draw_indirect_count) */
#if defined(VK_KHR_external_fence_capabilities)
PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR vkGetPhysicalDeviceExternalFencePropertiesKHR = 0;
#endif /* defined(VK_KHR_external_fence_capabilities) */
#if defined(VK_KHR_external_fence_fd)
PFN_vkGetFenceFdKHR vkGetFenceFdKHR = 0;
PFN_vkImportFenceFdKHR vkImportFenceFdKHR = 0;
#endif /* defined(VK_KHR_external_fence_fd) */
#if defined(VK_KHR_external_fence_win32)
PFN_vkGetFenceWin32HandleKHR vkGetFenceWin32HandleKHR = 0;
PFN_vkImportFenceWin32HandleKHR vkImportFenceWin32HandleKHR = 0;
#endif /* defined(VK_KHR_external_fence_win32) */
#if defined(VK_KHR_external_memory_capabilities)
PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR vkGetPhysicalDeviceExternalBufferPropertiesKHR = 0;
#endif /* defined(VK_KHR_external_memory_capabilities) */
#if defined(VK_KHR_external_memory_fd)
PFN_vkGetMemoryFdKHR vkGetMemoryFdKHR = 0;
PFN_vkGetMemoryFdPropertiesKHR vkGetMemoryFdPropertiesKHR = 0;
#endif /* defined(VK_KHR_external_memory_fd) */
#if defined(VK_KHR_external_memory_win32)
PFN_vkGetMemoryWin32HandleKHR vkGetMemoryWin32HandleKHR = 0;
PFN_vkGetMemoryWin32HandlePropertiesKHR vkGetMemoryWin32HandlePropertiesKHR = 0;
#endif /* defined(VK_KHR_external_memory_win32) */
#if defined(VK_KHR_external_semaphore_capabilities)
PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR vkGetPhysicalDeviceExternalSemaphorePropertiesKHR = 0;
#endif /* defined(VK_KHR_external_semaphore_capabilities) */
#if defined(VK_KHR_external_semaphore_fd)
PFN_vkGetSemaphoreFdKHR vkGetSemaphoreFdKHR = 0;
PFN_vkImportSemaphoreFdKHR vkImportSemaphoreFdKHR = 0;
#endif /* defined(VK_KHR_external_semaphore_fd) */
#if defined(VK_KHR_external_semaphore_win32)
PFN_vkGetSemaphoreWin32HandleKHR vkGetSemaphoreWin32HandleKHR = 0;
PFN_vkImportSemaphoreWin32HandleKHR vkImportSemaphoreWin32HandleKHR = 0;
#endif /* defined(VK_KHR_external_semaphore_win32) */
#if defined(VK_KHR_get_display_properties2)
PFN_vkGetDisplayModeProperties2KHR vkGetDisplayModeProperties2KHR = 0;
PFN_vkGetDisplayPlaneCapabilities2KHR vkGetDisplayPlaneCapabilities2KHR = 0;
PFN_vkGetPhysicalDeviceDisplayPlaneProperties2KHR vkGetPhysicalDeviceDisplayPlaneProperties2KHR = 0;
PFN_vkGetPhysicalDeviceDisplayProperties2KHR vkGetPhysicalDeviceDisplayProperties2KHR = 0;
#endif /* defined(VK_KHR_get_display_properties2) */
#if defined(VK_KHR_get_memory_requirements2)
PFN_vkGetBufferMemoryRequirements2KHR vkGetBufferMemoryRequirements2KHR = 0;
PFN_vkGetImageMemoryRequirements2KHR vkGetImageMemoryRequirements2KHR = 0;
PFN_vkGetImageSparseMemoryRequirements2KHR vkGetImageSparseMemoryRequirements2KHR = 0;
#endif /* defined(VK_KHR_get_memory_requirements2) */
#if defined(VK_KHR_get_physical_device_properties2)
PFN_vkGetPhysicalDeviceFeatures2KHR vkGetPhysicalDeviceFeatures2KHR = 0;
PFN_vkGetPhysicalDeviceFormatProperties2KHR vkGetPhysicalDeviceFormatProperties2KHR = 0;
PFN_vkGetPhysicalDeviceImageFormatProperties2KHR vkGetPhysicalDeviceImageFormatProperties2KHR = 0;
PFN_vkGetPhysicalDeviceMemoryProperties2KHR vkGetPhysicalDeviceMemoryProperties2KHR = 0;
PFN_vkGetPhysicalDeviceProperties2KHR vkGetPhysicalDeviceProperties2KHR = 0;
PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR vkGetPhysicalDeviceQueueFamilyProperties2KHR = 0;
PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR vkGetPhysicalDeviceSparseImageFormatProperties2KHR = 0;
#endif /* defined(VK_KHR_get_physical_device_properties2) */
#if defined(VK_KHR_get_surface_capabilities2)
PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR vkGetPhysicalDeviceSurfaceCapabilities2KHR = 0;
PFN_vkGetPhysicalDeviceSurfaceFormats2KHR vkGetPhysicalDeviceSurfaceFormats2KHR = 0;
#endif /* defined(VK_KHR_get_surface_capabilities2) */
#if defined(VK_KHR_maintenance1)
PFN_vkTrimCommandPoolKHR vkTrimCommandPoolKHR = 0;
#endif /* defined(VK_KHR_maintenance1) */
#if defined(VK_KHR_maintenance3)
PFN_vkGetDescriptorSetLayoutSupportKHR vkGetDescriptorSetLayoutSupportKHR = 0;
#endif /* defined(VK_KHR_maintenance3) */
#if defined(VK_KHR_mir_surface)
PFN_vkCreateMirSurfaceKHR vkCreateMirSurfaceKHR = 0;
PFN_vkGetPhysicalDeviceMirPresentationSupportKHR vkGetPhysicalDeviceMirPresentationSupportKHR = 0;
#endif /* defined(VK_KHR_mir_surface) */
#if defined(VK_KHR_push_descriptor)
PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSetKHR = 0;
#endif /* defined(VK_KHR_push_descriptor) */
#if defined(VK_KHR_sampler_ycbcr_conversion)
PFN_vkCreateSamplerYcbcrConversionKHR vkCreateSamplerYcbcrConversionKHR = 0;
PFN_vkDestroySamplerYcbcrConversionKHR vkDestroySamplerYcbcrConversionKHR = 0;
#endif /* defined(VK_KHR_sampler_ycbcr_conversion) */
#if defined(VK_KHR_shared_presentable_image)
PFN_vkGetSwapchainStatusKHR vkGetSwapchainStatusKHR = 0;
#endif /* defined(VK_KHR_shared_presentable_image) */
#if defined(VK_KHR_surface)
PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR = 0;
PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR = 0;
PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR = 0;
PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR = 0;
PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR = 0;
#endif /* defined(VK_KHR_surface) */
#if defined(VK_KHR_swapchain)
PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR = 0;
PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR = 0;
PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR = 0;
PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR = 0;
PFN_vkQueuePresentKHR vkQueuePresentKHR = 0;
#endif /* defined(VK_KHR_swapchain) */
#if defined(VK_KHR_wayland_surface)
PFN_vkCreateWaylandSurfaceKHR vkCreateWaylandSurfaceKHR = 0;
PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR vkGetPhysicalDeviceWaylandPresentationSupportKHR = 0;
#endif /* defined(VK_KHR_wayland_surface) */
#if defined(VK_KHR_win32_surface)
PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = 0;
PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR vkGetPhysicalDeviceWin32PresentationSupportKHR = 0;
#endif /* defined(VK_KHR_win32_surface) */
#if defined(VK_KHR_xcb_surface)
PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR = 0;
PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR vkGetPhysicalDeviceXcbPresentationSupportKHR = 0;
#endif /* defined(VK_KHR_xcb_surface) */
#if defined(VK_KHR_xlib_surface)
PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR = 0;
PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR vkGetPhysicalDeviceXlibPresentationSupportKHR = 0;
#endif /* defined(VK_KHR_xlib_surface) */
#if defined(VK_MVK_ios_surface)
PFN_vkCreateIOSSurfaceMVK vkCreateIOSSurfaceMVK = 0;
#endif /* defined(VK_MVK_ios_surface) */
#if defined(VK_MVK_macos_surface)
PFN_vkCreateMacOSSurfaceMVK vkCreateMacOSSurfaceMVK = 0;
#endif /* defined(VK_MVK_macos_surface) */
#if defined(VK_NN_vi_surface)
PFN_vkCreateViSurfaceNN vkCreateViSurfaceNN = 0;
#endif /* defined(VK_NN_vi_surface) */
#if defined(VK_NVX_device_generated_commands)
PFN_vkCmdProcessCommandsNVX vkCmdProcessCommandsNVX = 0;
PFN_vkCmdReserveSpaceForCommandsNVX vkCmdReserveSpaceForCommandsNVX = 0;
PFN_vkCreateIndirectCommandsLayoutNVX vkCreateIndirectCommandsLayoutNVX = 0;
PFN_vkCreateObjectTableNVX vkCreateObjectTableNVX = 0;
PFN_vkDestroyIndirectCommandsLayoutNVX vkDestroyIndirectCommandsLayoutNVX = 0;
PFN_vkDestroyObjectTableNVX vkDestroyObjectTableNVX = 0;
PFN_vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX = 0;
PFN_vkRegisterObjectsNVX vkRegisterObjectsNVX = 0;
PFN_vkUnregisterObjectsNVX vkUnregisterObjectsNVX = 0;
#endif /* defined(VK_NVX_device_generated_commands) */
#if defined(VK_NV_clip_space_w_scaling)
PFN_vkCmdSetViewportWScalingNV vkCmdSetViewportWScalingNV = 0;
#endif /* defined(VK_NV_clip_space_w_scaling) */
#if defined(VK_NV_external_memory_capabilities)
PFN_vkGetPhysicalDeviceExternalImageFormatPropertiesNV vkGetPhysicalDeviceExternalImageFormatPropertiesNV = 0;
#endif /* defined(VK_NV_external_memory_capabilities) */
#if defined(VK_NV_external_memory_win32)
PFN_vkGetMemoryWin32HandleNV vkGetMemoryWin32HandleNV = 0;
#endif /* defined(VK_NV_external_memory_win32) */
#if (defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor)) || (defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1))
PFN_vkCmdPushDescriptorSetWithTemplateKHR vkCmdPushDescriptorSetWithTemplateKHR = 0;
#endif /* (defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor)) || (defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1)) */
#if (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
PFN_vkGetDeviceGroupPresentCapabilitiesKHR vkGetDeviceGroupPresentCapabilitiesKHR = 0;
PFN_vkGetDeviceGroupSurfacePresentModesKHR vkGetDeviceGroupSurfacePresentModesKHR = 0;
PFN_vkGetPhysicalDevicePresentRectanglesKHR vkGetPhysicalDevicePresentRectanglesKHR = 0;
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */
#if (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
PFN_vkAcquireNextImage2KHR vkAcquireNextImage2KHR = 0;
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */
/* VOLK_GENERATE_PROTOTYPES_C */

#ifdef __GNUC__
#	pragma GCC visibility pop
#endif

#ifdef __cplusplus
}
#endif
