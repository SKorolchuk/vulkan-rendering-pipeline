#include <stdafx.h>

VulkanEndPointApplication::VulkanEndPointApplication() :
	width(800),
	height(600),
	title("Vulkan App")
{
}

VulkanEndPointApplication::VulkanEndPointApplication(
	int witdth,
	int height,
	std::string title) :
	width(width),
	height(height),
	title(title)
{
}

void VulkanEndPointApplication::Run()
{
	this->OpenWindow();
	this->Init();
	this->Loop();
	this->Clean();
}

void VulkanEndPointApplication::OpenWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	this->window = glfwCreateWindow(this->width, this->height, this->title.c_str(), nullptr, nullptr);
}

void VulkanEndPointApplication::Init()
{
	this->CreateVulkanInstance();
	this->SetupDebugCallback();
	this->CreateSurface();
	this->PickPhysicalDevice();
	this->CreateLogicalDevice();
	this->CreateSwapChain();
}

void VulkanEndPointApplication::CreateSwapChain()
{
	const SwapChainSupportDetails swapChainSupportDetails = QuerySwapChainSupport(this->vkPhysicalDevice, this->vkSurface);

	const VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupportDetails.formats);
	const VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupportDetails.presentModes);
	const VkExtent2D extent = ChooseSwapExtent(swapChainSupportDetails.capabilities, this->width, this->height);

	uint32_t imageCount = swapChainSupportDetails.capabilities.minImageCount + 1;

	if (swapChainSupportDetails.capabilities.maxImageCount > 0
		&& imageCount > swapChainSupportDetails.capabilities.maxImageCount)
	{
		imageCount = swapChainSupportDetails.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = this->vkSurface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = FindQueueFamilies(this->vkPhysicalDevice);

	uint32_t queueFamilyIndicies[] = {
		static_cast<uint32_t>(indices.graphicsFamily),
		static_cast<uint32_t>(indices.presentFamily)
	};

	if (indices.graphicsFamily != indices.presentFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndicies;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = swapChainSupportDetails.capabilities.currentTransform;

	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(this->vkDevice, &createInfo, nullptr, &this->vkSwapChain) != VK_SUCCESS)
	{
		throw std::runtime_error("swap chain creation failed");
	}

	vkGetSwapchainImagesKHR(this->vkDevice, this->vkSwapChain, &imageCount, nullptr);
	this->vkSwapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(this->vkDevice, this->vkSwapChain, &imageCount, this->vkSwapChainImages.data());

	this->vkExtent = extent;
	this->vkSwapChainImageFormat = surfaceFormat.format;
	this->vkSwapChainImageColorSpace = surfaceFormat.colorSpace;
}

void VulkanEndPointApplication::CreateSurface() {
	/*VkWin32SurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = glfwGetWin32Window(this->window);
	createInfo.hinstance = GetModuleHandle(nullptr);  */

	/*auto CreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(this->vkInstance, "vkCreateWin32SurfaceKHR");

	if (!CreateWin32SurfaceKHR || CreateWin32SurfaceKHR(this->vkInstance, &createInfo, nullptr, &this->vkSurface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}  */

	if (glfwCreateWindowSurface(this->vkInstance, this->window, nullptr, &this->vkSurface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

void VulkanEndPointApplication::Loop()
{
	this->Update();

	while (!glfwWindowShouldClose(this->window))
	{
		glfwPollEvents();
	}
}

void VulkanEndPointApplication::Clean()
{
	vkDestroySwapchainKHR(this->vkDevice, this->vkSwapChain, nullptr);
	if (this->enableValidationLayers)
	{
		this->DestroyVkDebugReportCallback(this->vkInstance, this->vkCallback, nullptr);
	}

	vkDestroyDevice(this->vkDevice, nullptr);
	vkDestroySurfaceKHR(this->vkInstance, this->vkSurface, nullptr);
	vkDestroyInstance(this->vkInstance, nullptr);

	glfwDestroyWindow(this->window);
	glfwTerminate();
}

void VulkanEndPointApplication::Update()
{
}

void VulkanEndPointApplication::SetupDebugCallback()
{
	if (!this->enableValidationLayers)
	{
		return;
	}

	VkDebugReportCallbackCreateInfoEXT createInfo = {};

	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfo.pfnCallback = debugCallback;

	if (this->CreateVkDebugReportCallback(this->vkInstance, &createInfo, nullptr, &this->vkCallback) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to set up debug vulkan callback!");
	}
}

void VulkanEndPointApplication::CreateLogicalDevice()
{
	QueueFamilyIndices indices = this->FindQueueFamilies(this->vkPhysicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

	float queuePriority = 1.0f;
	for (int queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(this->deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = this->deviceExtensions.data();

	if (this->enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}  

	if (vkCreateDevice(this->vkPhysicalDevice, &createInfo, nullptr, &this->vkDevice) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}
	vkGetDeviceQueue(this->vkDevice, indices.graphicsFamily, 0, &this->vkGraphicsQueue);
	vkGetDeviceQueue(this->vkDevice, indices.presentFamily, 0, &this->vkPresentQueue);
}

QueueFamilyIndices VulkanEndPointApplication::FindQueueFamilies(VkPhysicalDevice device) const {
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, this->vkSurface, &presentSupport);

		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT && presentSupport) {
			indices.graphicsFamily = i;
			indices.presentFamily = i;
		}

		if (indices.IsComplete()) {
			break;
		}

		i++;
	}

	return indices;
}

void VulkanEndPointApplication::PickPhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(this->vkInstance, &deviceCount, nullptr);
	if (deviceCount == 0) {
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(this->vkInstance, &deviceCount, devices.data());

	// Use an ordered map to automatically sort candidates by increasing score
	std::multimap<int, VkPhysicalDevice> candidates;

	for (const auto& device : devices) {
		int score = this->RateDeviceSuitability(device);
		candidates.insert(std::make_pair(score, device));

		if (this->IsDeviceSuitable(device)) {
			this->vkPhysicalDevice = device;
		}
	}

	if (this->vkPhysicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}

	if (candidates.rbegin()->first > 0) {
		this->vkPhysicalDevice = candidates.rbegin()->second;
	}
	else {
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

int VulkanEndPointApplication::RateDeviceSuitability(VkPhysicalDevice device) {
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
	
	int score = 0;

	// Discrete GPUs have a significant performance advantage
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 1000;
	}

	// Maximum possible size of textures affects graphics quality
	score += deviceProperties.limits.maxImageDimension2D;

	// Application can't function without geometry shaders
	if (!deviceFeatures.geometryShader) {
		return 0;
	}

	return score;
}

bool VulkanEndPointApplication::IsDeviceSuitable(VkPhysicalDevice device) {
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	QueueFamilyIndices indices = this->FindQueueFamilies(device);

	const bool requiredExtensionsSupported = this->CheckDeviceExtensionsSupport(device);

	bool swapChainValidationResult = false;

	if (requiredExtensionsSupported)
	{
		SwapChainSupportDetails swapChainSupportDetails = QuerySwapChainSupport(device, this->vkSurface);

		swapChainValidationResult = !swapChainSupportDetails.formats.empty() && !swapChainSupportDetails.presentModes.empty();
	}

	return indices.IsComplete()
	&& deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
	&& deviceFeatures.geometryShader
	&& requiredExtensionsSupported
	&& swapChainValidationResult;
}

bool VulkanEndPointApplication::CheckDeviceExtensionsSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;

	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);

	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(this->deviceExtensions.begin(), this->deviceExtensions.end());

	for (const auto& extension: availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

void VulkanEndPointApplication::CreateVulkanInstance()
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;


	auto glfwExtensions = this->GetRequiredExtensions();

	createInfo.enabledExtensionCount = static_cast<uint32_t>(glfwExtensions.size());
	createInfo.ppEnabledExtensionNames = glfwExtensions.data();

	if (enableValidationLayers && this->CheckVkValidationLayerSupport()) {
		std::cout << "Vulkan validation layers enabled" << std::endl;
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = this->validationLayers.data();
	}
	else if (enableValidationLayers)
	{
		throw std::runtime_error("Failed to check validation layers");
	}

	if (!enableValidationLayers) {
		createInfo.enabledLayerCount = 0;
	}

	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	std::cout << "available extensions:" << std::endl;

	for (const auto &extension : extensions)
	{
		std::cout << "\t" << extension.extensionName << std::endl;
	}

	if (this->CreateVkInstanceWithCheck(&createInfo, nullptr, &this->vkInstance) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create instance!");
	}	
}

VkResult VulkanEndPointApplication::CreateVkInstanceWithCheck(
	const VkInstanceCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkInstance* instance)
{

	if (pCreateInfo == nullptr || instance == nullptr) {
		std::cout << "Null pointer passed to required parameter!" << std::endl;
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	return vkCreateInstance(pCreateInfo, pAllocator, instance);
}

VkResult VulkanEndPointApplication::CreateVkDebugReportCallback(
	VkInstance instance,
	const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugReportCallbackEXT* pCallback)
{
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");

	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void VulkanEndPointApplication::DestroyVkDebugReportCallback(
	VkInstance instance,
	VkDebugReportCallbackEXT callback,
	const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");

	if (func != nullptr)
	{
		func(instance, callback, pAllocator);
	}
}

bool VulkanEndPointApplication::CheckVkValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : this->validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

std::vector<const char*> VulkanEndPointApplication::GetRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char **glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (this->enableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	return extensions;
}
