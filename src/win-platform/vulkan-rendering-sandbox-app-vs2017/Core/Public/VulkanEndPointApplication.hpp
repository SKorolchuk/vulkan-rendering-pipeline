#pragma once

class VulkanEndPointApplication
{
public:
	VulkanEndPointApplication();
	VulkanEndPointApplication(int witdth, int height, std::string title);
	virtual void Run();

protected:
	virtual void OpenWindow();
	virtual void Init();
	virtual void Loop();
	virtual void Update();
	virtual void Clean();
	GLFWwindow *window;

private:
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
	VkFormat vkSwapChainImageFormat;
	VkExtent2D vkExtent;
	VkColorSpaceKHR vkSwapChainImageColorSpace;

	VkRenderPass vkRenderPass;
	VkPipelineLayout vkPipelineLayout;

	int width;
	int height;
	std::string title;
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

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
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