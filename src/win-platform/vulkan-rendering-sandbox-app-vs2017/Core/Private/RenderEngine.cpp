#include <stdafx.h>

VulkanCore::RenderEngine::RenderEngine(int width, int height, GLFWwindow* window) :
	ViewportWidth(width),
	ViewportHeight(height),
	GLWindow(window)
{
	if (!this->IsPipelineInitialized)
	{
		this->RenderEngine::BootstrapPipeline();
	}
}

VulkanCore::RenderEngine::~RenderEngine()
{
	if (this->IsPipelineInitialized)
	{
		this->RenderEngine::CleanPipeline();
	}

	this->GLWindow = nullptr;
}

void VulkanCore::RenderEngine::BootstrapPipeline()
{
	if (this->IsPipelineInitialized)
		return;

	this->CreateVulkanInstance();
	this->SetupDebugCallback();
	this->CreateSurface();
	this->PickPhysicalDevice();
	this->CreateLogicalDevice();
	this->CreateSwapChain();
	this->CreateImageViews();
	this->CreateRenderPass();
	this->CreateGraphicsPipeline();
	this->CreateFrameBuffers();
	this->CreateCommandPool();
	this->CreateCommandBuffers();
	this->CreatePipelineSyncObjects();

	this->IsPipelineInitialized = true;
}

void VulkanCore::RenderEngine::CleanPipeline()
{
	if (!this->IsPipelineInitialized)
		return;

	this->CleanSwapChain();

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		vkDestroySemaphore(this->vkDevice, this->vkRenderFinishedSemLocks[i], nullptr);
		vkDestroySemaphore(this->vkDevice, this->vkImageAvailableSemLocks[i], nullptr);
		vkDestroyFence(this->vkDevice, this->vkInFlightSyncFences[i], nullptr);
	}

	vkDestroyCommandPool(this->vkDevice, this->vkCommandPool, nullptr);

	vkDestroyDevice(this->vkDevice, nullptr);
	if (this->enableValidationLayers)
	{
		this->DestroyVkDebugReportCallback(this->vkInstance, this->vkCallback, nullptr);
	}
	vkDestroySurfaceKHR(this->vkInstance, this->vkSurface, nullptr);
	vkDestroyInstance(this->vkInstance, nullptr);

	this->IsPipelineInitialized = false;
}

void VulkanCore::RenderEngine::Draw()
{
	/*if (this->ThrottleCheck())
	{
		return;
	}
	else
	{
		this->lastDrawTime = 
			std::chrono::time_point_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now()
			);
	} */ 

	vkWaitForFences(this->vkDevice, 1, &this->vkInFlightSyncFences[this->currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
	vkResetFences(this->vkDevice, 1, &this->vkInFlightSyncFences[this->currentFrame]);

	uint32_t imageIndex;

	VkResult result = vkAcquireNextImageKHR(
		this->vkDevice,
		this->vkSwapChain,
		std::numeric_limits<uint64_t>::max(),
		this->vkImageAvailableSemLocks[this->currentFrame],
		VK_NULL_HANDLE,
		&imageIndex);

	VkSubmitInfo submitInfo = {};

	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore waitLocks[] = { this->vkImageAvailableSemLocks[this->currentFrame] };

	VkPipelineStageFlags waitFlags[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitLocks;
	submitInfo.pWaitDstStageMask = waitFlags;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &this->vkCommandBuffers[imageIndex];

	VkSemaphore signalLocks[] = { this->vkRenderFinishedSemLocks[this->currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalLocks;

	if (vkQueueSubmit(this->vkGraphicsQueue, 1, &submitInfo, this->vkInFlightSyncFences[this->currentFrame]) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to submit draw commands");
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalLocks;

	VkSwapchainKHR swapChains[] = { this->vkSwapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional param

	vkQueuePresentKHR(this->vkPresentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || FrameBufferResized)
	{
		FrameBufferResized = false;
		this->UpdateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("Failed to acquire swap chain image view");
	}

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanCore::RenderEngine::WaitDevice()
{
	vkDeviceWaitIdle(this->vkDevice);
}


void VulkanCore::RenderEngine::CreateFrameBuffers()
{
	this->vkSwapChainFrameBuffers.resize(this->vkSwapChainImageViews.size());

	for (size_t i = 0; i < this->vkSwapChainImageViews.size(); ++i)
	{
		VkImageView attachments[] = {
			this->vkSwapChainImageViews[i]
		};

		VkFramebufferCreateInfo frameBufferCreateInfo = {};

		frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferCreateInfo.renderPass = this->vkRenderPass;
		frameBufferCreateInfo.attachmentCount = 1;
		frameBufferCreateInfo.pAttachments = attachments;
		frameBufferCreateInfo.width = this->vkExtent.width;
		frameBufferCreateInfo.height = this->vkExtent.height;
		frameBufferCreateInfo.layers = 1;

		if (vkCreateFramebuffer(this->vkDevice, &frameBufferCreateInfo, nullptr, &this->vkSwapChainFrameBuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create frame buffer!");
		}
	}
}

void VulkanCore::RenderEngine::CreateCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(this->vkPhysicalDevice);

	VkCommandPoolCreateInfo poolCreateInfo = {};

	poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
	poolCreateInfo.flags = 0; // Optional param

	if (vkCreateCommandPool(this->vkDevice, &poolCreateInfo, nullptr, &this->vkCommandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create command pool");
	}
}

void VulkanCore::RenderEngine::CreateCommandBuffers()
{
	this->vkCommandBuffers.resize(this->vkSwapChainFrameBuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = this->vkCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(this->vkCommandBuffers.size());

	if (vkAllocateCommandBuffers(this->vkDevice, &allocInfo, this->vkCommandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate command buffers!");
	}

	for (size_t i = 0; i < this->vkCommandBuffers.size(); i++) {
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr; // Optional

		if (vkBeginCommandBuffer(this->vkCommandBuffers[i], &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to begin recording command buffer!");
		}

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = this->vkRenderPass;
		renderPassInfo.framebuffer = this->vkSwapChainFrameBuffers[i];

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = this->vkExtent;

		VkClearValue clearColor = { 0.3f, 0.3f, 0.3f, 0.5f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(this->vkCommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(this->vkCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, this->vkGraphicsPipeline);

		vkCmdDraw(this->vkCommandBuffers[i], 3, 1, 0, 0);

		vkCmdEndRenderPass(this->vkCommandBuffers[i]);

		if (vkEndCommandBuffer(this->vkCommandBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}
}

void VulkanCore::RenderEngine::CreatePipelineSyncObjects()
{
	this->vkImageAvailableSemLocks.resize(MAX_FRAMES_IN_FLIGHT);
	this->vkRenderFinishedSemLocks.resize(MAX_FRAMES_IN_FLIGHT);
	this->vkInFlightSyncFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		if (vkCreateSemaphore(this->vkDevice, &semaphoreCreateInfo, nullptr, &this->vkImageAvailableSemLocks[i]) != VK_SUCCESS
			|| vkCreateSemaphore(this->vkDevice, &semaphoreCreateInfo, nullptr, &this->vkRenderFinishedSemLocks[i]) != VK_SUCCESS
			|| vkCreateFence(this->vkDevice, &fenceCreateInfo, nullptr, &this->vkInFlightSyncFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create pipeline lock semaphores");
		}
	}
}

void VulkanCore::RenderEngine::CleanSwapChain()
{
	for (auto frameBuffer : this->vkSwapChainFrameBuffers)
	{
		vkDestroyFramebuffer(this->vkDevice, frameBuffer, nullptr);
	}

	vkFreeCommandBuffers(
		this->vkDevice,
		this->vkCommandPool,
		static_cast<uint32_t>(this->vkCommandBuffers.size()),
		this->vkCommandBuffers.data());

	vkDestroyPipeline(this->vkDevice, this->vkGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(this->vkDevice, this->vkPipelineLayout, nullptr);
	vkDestroyRenderPass(this->vkDevice, this->vkRenderPass, nullptr);

	for (auto imageView : this->vkSwapChainImageViews) {
		vkDestroyImageView(this->vkDevice, imageView, nullptr);
	}

	vkDestroySwapchainKHR(this->vkDevice, this->vkSwapChain, nullptr);
}

void VulkanCore::RenderEngine::UpdateSwapChain()
{
	int width = 0, height = 0;
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(this->GLWindow, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(this->vkDevice);

	this->CleanSwapChain();

	this->CreateSwapChain();
	this->CreateImageViews();
	this->CreateRenderPass();
	this->CreateGraphicsPipeline();
	this->CreateFrameBuffers();
	this->CreateCommandBuffers();
}

void VulkanCore::RenderEngine::CreateSwapChain()
{
	const SwapChainSupportDetails swapChainSupportDetails = QuerySwapChainSupport(this->vkPhysicalDevice, this->vkSurface);

	const VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupportDetails.formats);
	const VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupportDetails.presentModes);
	const VkExtent2D extent = ChooseSwapExtent(swapChainSupportDetails.capabilities, this->GLWindow);

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

void VulkanCore::RenderEngine::CreateSurface() {
	/*VkWin32SurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = glfwGetWin32Window(this->window);
	createInfo.hinstance = GetModuleHandle(nullptr);  */

	/*auto CreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(this->vkInstance, "vkCreateWin32SurfaceKHR");

	if (!CreateWin32SurfaceKHR || CreateWin32SurfaceKHR(this->vkInstance, &createInfo, nullptr, &this->vkSurface) != VK_SUCCESS) {
	throw std::runtime_error("failed to create window surface!");
	}  */

	if (glfwCreateWindowSurface(this->vkInstance, this->GLWindow, nullptr, &this->vkSurface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

void VulkanCore::RenderEngine::SetupDebugCallback()
{
	if (!this->enableValidationLayers)
	{
		return;
	}

	VkDebugReportCallbackCreateInfoEXT createInfo = {};

	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfo.pfnCallback = DebugCallback;

	if (this->CreateVkDebugReportCallback(this->vkInstance, &createInfo, nullptr, &this->vkCallback) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to set up debug vulkan callback!");
	}
}

void VulkanCore::RenderEngine::CreateImageViews()
{
	this->vkSwapChainImageViews.resize(this->vkSwapChainImages.size());

	for (size_t i = 0; i < this->vkSwapChainImages.size(); i++)
	{
		VkImageViewCreateInfo createInfo = {};

		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = this->vkSwapChainImages[i];

		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = this->vkSwapChainImageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(this->vkDevice, &createInfo, nullptr, &this->vkSwapChainImageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image views!");
		}
	}
}

void VulkanCore::RenderEngine::CreateGraphicsPipeline()
{
	std::vector<char> vertrexShaderText = ShaderExtensions::ReadShaderFile("../Shaders/test_vertrex_shader.spv");
	std::vector<char> fragmentShaderText = ShaderExtensions::ReadShaderFile("../Shaders/test_fragment_shader.spv");

	this->vkVertrexShader = ShaderExtensions::CreateShaderModule(this->vkDevice, vertrexShaderText);
	this->vkFragmentShader = ShaderExtensions::CreateShaderModule(this->vkDevice, fragmentShaderText);

	VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo = {};

	vertShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageCreateInfo.module = this->vkVertrexShader;
	vertShaderStageCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragmentShaderStageCreateInfo = {};

	fragmentShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderStageCreateInfo.module = this->vkFragmentShader;
	fragmentShaderStageCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageCreateInfo, fragmentShaderStageCreateInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(this->vkExtent.width);
	viewport.height = static_cast<float>(this->vkExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = this->vkExtent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.2f;
	colorBlending.blendConstants[1] = 0.2f;
	colorBlending.blendConstants[2] = 0.2f;
	colorBlending.blendConstants[3] = 0.2f;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(this->vkDevice, &pipelineLayoutInfo, nullptr, &this->vkPipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = this->vkPipelineLayout;
	pipelineInfo.renderPass = this->vkRenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(this->vkDevice, nullptr, 1, &pipelineInfo, nullptr, &this->vkGraphicsPipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(this->vkDevice, this->vkFragmentShader, nullptr);
	vkDestroyShaderModule(this->vkDevice, this->vkVertrexShader, nullptr);
}

void VulkanCore::RenderEngine::CreateLogicalDevice()
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

QueueFamilyIndices VulkanCore::RenderEngine::FindQueueFamilies(VkPhysicalDevice device) const {
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

void VulkanCore::RenderEngine::PickPhysicalDevice() {
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

int VulkanCore::RenderEngine::RateDeviceSuitability(VkPhysicalDevice device) {
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

bool VulkanCore::RenderEngine::IsDeviceSuitable(VkPhysicalDevice device) {
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

bool VulkanCore::RenderEngine::CheckDeviceExtensionsSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;

	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);

	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(this->deviceExtensions.begin(), this->deviceExtensions.end());

	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

void VulkanCore::RenderEngine::CreateRenderPass()
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = this->vkSwapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	VkSubpassDependency dependency = {};

	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;

	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;

	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(this->vkDevice, &renderPassInfo, nullptr, &this->vkRenderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

void VulkanCore::RenderEngine::CreateVulkanInstance()
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

VkResult VulkanCore::RenderEngine::CreateVkInstanceWithCheck(
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

VkResult VulkanCore::RenderEngine::CreateVkDebugReportCallback(
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

void VulkanCore::RenderEngine::DestroyVkDebugReportCallback(
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

bool VulkanCore::RenderEngine::CheckVkValidationLayerSupport()
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

std::vector<const char*> VulkanCore::RenderEngine::GetRequiredExtensions()
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

bool VulkanCore::RenderEngine::ThrottleCheck()
{
	auto currentTime = std::chrono::system_clock::now();

	double dilation = std::chrono::duration<double, std::milli>(currentTime - this->lastDrawTime).count();

	return dilation < 1000/60;
}
