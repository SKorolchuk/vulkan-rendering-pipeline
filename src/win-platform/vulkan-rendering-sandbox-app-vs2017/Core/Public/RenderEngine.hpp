#pragma once

namespace VulkanCore
{
	 class RenderEngine
	 {
	 public:
		 RenderEngine(int width, int height, GLFWwindow* window);
		 ~RenderEngine();
		 virtual void BootstrapPipeline();
		 virtual void CleanPipeline();
		 virtual void Draw();

	 protected:
		 int ViewportWidth;
		 int ViewportHeight;
		 GLFWwindow *GLWindow = nullptr;
		 bool IsPipelineInitialized = false;
		 bool CheckVkValidationLayerSupport();
		 VkResult CreateVkInstanceWithCheck(
			 const VkInstanceCreateInfo* pCreateInfo,
			 const VkAllocationCallbacks* pAllocator,
			 VkInstance* instance);
		 VkResult CreateVkDebugReportCallback(
			 VkInstance instance,
			 const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
			 const VkAllocationCallbacks* pAllocator,
			 VkDebugReportCallbackEXT* pCallback
		 );
		 void DestroyVkDebugReportCallback(
			 VkInstance instance,
			 VkDebugReportCallbackEXT callback,
			 const VkAllocationCallbacks* pAllocator);
		 std::vector<const char*> GetRequiredExtensions();
		 void CreateVulkanInstance();
		 void SetupDebugCallback();
		 void CreateLogicalDevice();
		 void CreateSwapChain();
		 void PickPhysicalDevice();
		 void CreateSurface();
		 void CreateImageViews();
		 void CreateGraphicsPipeline();
		 void CreateFrameBuffers();
		 void CreateCommandPool();
		 void CreateCommandBuffers();
		 void CreatePipelineSemaphores();
		 bool IsDeviceSuitable(VkPhysicalDevice device);
		 bool CheckDeviceExtensionsSupport(VkPhysicalDevice device);
		 void CreateRenderPass();
		 static int RateDeviceSuitability(VkPhysicalDevice device);
		 QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
		 VkInstance vkInstance;
		 VkDebugReportCallbackEXT vkCallback;
		 VkPhysicalDevice vkPhysicalDevice = VK_NULL_HANDLE;
		 VkDevice vkDevice;
		 VkQueue vkGraphicsQueue;
		 VkQueue vkPresentQueue;
		 VkSurfaceKHR vkSurface;
		 VkSwapchainKHR vkSwapChain;
		 std::vector<VkImage> vkSwapChainImages;
		 std::vector<VkImageView> vkSwapChainImageViews;
		 std::vector<VkFramebuffer> vkSwapChainFrameBuffers;
		 VkFormat vkSwapChainImageFormat;
		 VkExtent2D vkExtent;
		 VkColorSpaceKHR vkSwapChainImageColorSpace;

		 VkRenderPass vkRenderPass;
		 VkPipelineLayout vkPipelineLayout;
		 VkPipeline vkGraphicsPipeline;

		 // semaphores

		 VkSemaphore vkImageAvailableSemLock;
		 VkSemaphore vkRenderFinishedSemLock;

		 // command pool and etc.

		 VkCommandPool vkCommandPool;
		 std::vector<VkCommandBuffer> vkCommandBuffers;

		 const bool enableValidationLayers = true;
		 const std::vector<const char*> validationLayers = {
			 "VK_LAYER_LUNARG_standard_validation"
		 };
		 const std::vector<const char*> deviceExtensions = {
			 VK_KHR_SWAPCHAIN_EXTENSION_NAME
		 };

		 // shaders

		 VkShaderModule vkVertrexShader;
		 VkShaderModule vkFragmentShader;
	 };

	 static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		 VkDebugReportFlagsEXT flags,
		 VkDebugReportObjectTypeEXT objType,
		 uint64_t obj,
		 size_t location,
		 int32_t code,
		 const char* layerPrefix,
		 const char* msg,
		 void* userData) {

		 std::cerr << "validation layer: " << msg << std::endl;

		 return VK_FALSE;
	 }
}