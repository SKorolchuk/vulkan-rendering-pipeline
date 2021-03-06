#include "../Public/RenderEngine.hpp"

VulkanCore::RenderEngine::RenderEngine(
	int width,
	int height,
	std::string modelPath,
	std::string baseColorTexturePath,
	GLFWwindow* window) :
	ViewportWidth(width),
	ViewportHeight(height),
	ModelPath(modelPath),
	BaseColorTexturePath(baseColorTexturePath),
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
	this->CreateDescriptorSetLayout();
	this->CreateGraphicsPipeline();
	this->CreateCommandPool();
	this->CreateDepthResources();
	this->CreateFrameBuffers();
	this->LoadTextures();
	this->CreateTextureViews();
	this->InitializeSampler();
	this->CreateGeometryBuffers();
	this->CreateUniformBuffer();
	this->CreateDescriptorPool();
	this->CreateDescriptorSet();
	this->CreateCommandBuffers();
	this->CreatePipelineSyncObjects();

	this->IsPipelineInitialized = true;
}

void VulkanCore::RenderEngine::CleanPipeline()
{
	if (!this->IsPipelineInitialized)
		return;

	this->CleanSwapChain();

	vkDestroySampler(this->vkDevice, this->vkTextureSampler, nullptr);

	vkDestroyImageView(this->vkDevice, this->vkTextureImageView, nullptr);

	vkDestroyImage(this->vkDevice, this->vkTextureImage, nullptr);
	vkFreeMemory(this->vkDevice, this->vkTextureImageMemory, nullptr);

	vkDestroyDescriptorPool(this->vkDevice, this->vkDescriptorPool, nullptr);

	vkDestroyDescriptorSetLayout(this->vkDevice, this->vkDescriptorSetLayout, nullptr);

	for (size_t i = 0; i < this->vkSwapChainImages.size(); i++) {
		vkDestroyBuffer(this->vkDevice, this->vkUniformBuffers[i], nullptr);
		vkFreeMemory(this->vkDevice, this->vkUniformBuffersMemory[i], nullptr);
	}

	vkDestroyBuffer(this->vkDevice, vkIndexBuffer, nullptr);
	vkFreeMemory(this->vkDevice, vkIndexBufferMemory, nullptr);

	vkDestroyBuffer(this->vkDevice, this->vkVertexBuffer, nullptr);
	vkFreeMemory(this->vkDevice, this->vkVertexBufferMemory, nullptr);

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
	if (this->ThrottleCheck())
	{
		return;
	}

	this->lastDrawTime = 
		std::chrono::time_point_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now()
		);	 

	vkWaitForFences(this->vkDevice, 1, &this->vkInFlightSyncFences[this->currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
	vkResetFences(this->vkDevice, 1, &this->vkInFlightSyncFences[this->currentFrame]);

	uint32_t imageIndex;

	const VkResult result = vkAcquireNextImageKHR(
		this->vkDevice,
		this->vkSwapChain,
		std::numeric_limits<uint64_t>::max(),
		this->vkImageAvailableSemLocks[this->currentFrame],
		nullptr,
		&imageIndex);

	this->UpdateUniformBuffer(imageIndex);

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
		std::array<VkImageView, 2> attachments = {
			this->vkSwapChainImageViews[i],
			this->vkDepthImagesView[i]
		};

		VkFramebufferCreateInfo frameBufferCreateInfo = {};

		frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferCreateInfo.renderPass = this->vkRenderPass;
		frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		frameBufferCreateInfo.pAttachments = attachments.data();
		frameBufferCreateInfo.width = this->vkExtent.width;
		frameBufferCreateInfo.height = this->vkExtent.height;
		frameBufferCreateInfo.layers = 1;

		if (vkCreateFramebuffer(
			this->vkDevice,
			&frameBufferCreateInfo,
			nullptr,
			&this->vkSwapChainFrameBuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create frame buffer!");
		}
	}
}

void VulkanCore::RenderEngine::CreateCommandPool()
{
	const QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(this->vkPhysicalDevice);

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

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.7f, 0.76f, 0.8f, 0.95f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(this->vkCommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(this->vkCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, this->vkGraphicsPipeline);

		VkBuffer vertexBuffers[] = { this->vkVertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(
			this->vkCommandBuffers[i],
			0, 1,
			vertexBuffers, offsets);

		vkCmdBindIndexBuffer(
			this->vkCommandBuffers[i],
			this->vkIndexBuffer,
			0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(
			this->vkCommandBuffers[i],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			this->vkPipelineLayout,
			0, 1,
			&this->vkDescriptorSets[i], 0, nullptr);

		vkCmdDrawIndexed(this->vkCommandBuffers[i], static_cast<uint32_t>(Vertex::GetSampleVertexIndices().size()), 1, 0, 0, 0);

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
	for (size_t i = 0; i < this->vkSwapChainImages.size(); i++) {
		vkDestroyImageView(this->vkDevice, this->vkDepthImagesView[i], nullptr);
		vkDestroyImage(this->vkDevice, this->vkDepthImages[i], nullptr);
		vkFreeMemory(this->vkDevice, this->vkDepthImagesMemory[i], nullptr);
	}

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
	this->CreateDepthResources();
	this->CreateFrameBuffers();
	this->CreateCommandBuffers();
}

void VulkanCore::RenderEngine::InitializeSampler()
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	if (vkCreateSampler(this->vkDevice, &samplerInfo, nullptr, &this->vkTextureSampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}
}

void VulkanCore::RenderEngine::CreateDescriptorSet()
{
	std::vector<VkDescriptorSetLayout> layouts(this->vkSwapChainImages.size(), this->vkDescriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = this->vkDescriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(this->vkSwapChainImages.size());
	allocInfo.pSetLayouts = layouts.data();

	this->vkDescriptorSets.resize(this->vkSwapChainImages.size());
	if (vkAllocateDescriptorSets(this->vkDevice, &allocInfo, &this->vkDescriptorSets[0]) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < this->vkSwapChainImages.size(); i++) {
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = this->vkUniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = this->vkTextureImageView;
		imageInfo.sampler = this->vkTextureSampler;

		std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = this->vkDescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = this->vkDescriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		descriptorWrites[0].pImageInfo = nullptr; // Optional
		descriptorWrites[0].pTexelBufferView = nullptr; // Optional

		vkUpdateDescriptorSets(
			this->vkDevice,
			static_cast<uint32_t>(descriptorWrites.size()),
			descriptorWrites.data(),
			0,
			nullptr);
	}
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
	createInfo.oldSwapchain = nullptr;

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
		this->vkSwapChainImageViews[i] = GraphicsPipelineUtils::CreateImageView(
			this->vkSwapChainImages[i],
			this->vkSwapChainImageFormat,
			VK_IMAGE_ASPECT_COLOR_BIT,
			this->vkDevice);
	}
}

void VulkanCore::RenderEngine::LoadTextures()
{
	int textureWidth, textureHeight, textureChannels;

	this->PixelBuffer = ShaderExtensions::CreateTextureImage(this->BaseColorTexturePath.c_str(), &textureWidth, &textureHeight, &textureChannels);

	VkDeviceSize imageSize = textureWidth * textureHeight * 4;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	MemoryUtils::CreateBuffer(
		imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		this->vkDevice,
		this->vkPhysicalDevice,
		stagingBuffer,
		stagingBufferMemory);

	void* data;
	vkMapMemory(this->vkDevice, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, this->PixelBuffer, static_cast<size_t>(imageSize));
	vkUnmapMemory(this->vkDevice, stagingBufferMemory);

	stbi_image_free(this->PixelBuffer);

	MemoryUtils::CreateImage(
		textureWidth,
		textureHeight,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		this->vkTextureImage,
		this->vkTextureImageMemory,
		this->vkDevice,
		this->vkPhysicalDevice);

	MemoryUtils::TransitionImageLayout(
		this->vkTextureImage,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		this->vkDevice,
		this->vkCommandPool,
		this->vkGraphicsQueue);
	MemoryUtils::CopyBufferToImage(
		stagingBuffer,
		this->vkTextureImage,
		static_cast<uint32_t>(textureWidth),
		static_cast<uint32_t>(textureHeight),
		this->vkDevice,
		this->vkCommandPool,
		this->vkGraphicsQueue);
	MemoryUtils::TransitionImageLayout(
		this->vkTextureImage,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		this->vkDevice,
		this->vkCommandPool,
		this->vkGraphicsQueue);

	vkDestroyBuffer(this->vkDevice, stagingBuffer, nullptr);
	vkFreeMemory(this->vkDevice, stagingBufferMemory, nullptr);
}

void VulkanCore::RenderEngine::CreateTextureViews()
{
	this->vkTextureImageView = GraphicsPipelineUtils::CreateImageView(
		this->vkTextureImage,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT,
		this->vkDevice);
}

void VulkanCore::RenderEngine::CreateDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(this->vkDevice, &layoutInfo, nullptr, &this->vkDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void VulkanCore::RenderEngine::CreateGraphicsPipeline()
{
	const std::vector<char> vertrexShaderText = ShaderExtensions::ReadShaderFile("../Shaders/base_ubo_vertrex_shader.vert");
	const std::vector<char> fragmentShaderText = ShaderExtensions::ReadShaderFile("../Shaders/base_fragment_shader.frag");

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
	
	auto bindingDescription = Vertex::GetBindingDescription();
	auto attributeDescriptions = Vertex::GetAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

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
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;


	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.front = {}; // Optional
	depthStencil.back = {}; // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &this->vkDescriptorSetLayout;

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
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = this->vkPipelineLayout;
	pipelineInfo.renderPass = this->vkRenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(
		this->vkDevice,
		nullptr,
		1, &pipelineInfo,
		nullptr, &this->vkGraphicsPipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(this->vkDevice, this->vkFragmentShader, nullptr);
	vkDestroyShaderModule(this->vkDevice, this->vkVertrexShader, nullptr);
}

void VulkanCore::RenderEngine::CreateLogicalDevice()
{
	const QueueFamilyIndices indices = this->FindQueueFamilies(this->vkPhysicalDevice);

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
	deviceFeatures.samplerAnisotropy = VK_TRUE;

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

bool VulkanCore::RenderEngine::IsDeviceSuitable(VkPhysicalDevice device) const
{
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
		&& swapChainValidationResult
		&& deviceFeatures.samplerAnisotropy;
}

bool VulkanCore::RenderEngine::CheckDeviceExtensionsSupport(VkPhysicalDevice device) const
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

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = GraphicsPipelineUtils::FindDepthFormat(this->vkPhysicalDevice);
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(this->vkDevice, &renderPassInfo, nullptr, &this->vkRenderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

void VulkanCore::RenderEngine::CreateGeometryBuffers()
{
	MeshExtensions::LoadModelToMemoryBuffer(
		this->ModelPath.c_str(),
		this->vkDevice,
		this->vkPhysicalDevice,
		this->vkCommandPool,
		this->vkGraphicsQueue,
		this->vkVertexBuffer,
		this->vkVertexBufferMemory,
		this->vkIndexBuffer,
		this->vkIndexBufferMemory);
}

void VulkanCore::RenderEngine::CreateDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 2> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(this->vkSwapChainImages.size());
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(this->vkSwapChainImages.size());

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(this->vkSwapChainImages.size());

	if (vkCreateDescriptorPool(this->vkDevice, &poolInfo, nullptr, &this->vkDescriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void VulkanCore::RenderEngine::CreateUniformBuffer()
{
	const VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	this->vkUniformBuffers.resize(this->vkSwapChainImages.size());
	this->vkUniformBuffersMemory.resize(this->vkSwapChainImages.size());

	for (size_t i = 0; i < this->vkSwapChainImages.size(); i++) {
		MemoryUtils::CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			this->vkDevice,
			this->vkPhysicalDevice,
			this->vkUniformBuffers[i],
			this->vkUniformBuffersMemory[i]);
	}
}

void VulkanCore::RenderEngine::CreateDepthResources()
{
	VkFormat depthFormat = GraphicsPipelineUtils::FindDepthFormat(this->vkPhysicalDevice);

	this->vkDepthImages.resize(this->vkSwapChainImages.size());
	this->vkDepthImagesMemory.resize(this->vkSwapChainImages.size());
	this->vkDepthImagesView.resize(this->vkSwapChainImages.size());

	for (size_t i = 0; i < this->vkSwapChainImages.size(); i++) {
		MemoryUtils::CreateImage(
			this->vkExtent.width,
			this->vkExtent.height,
			depthFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			this->vkDepthImages[i],
			this->vkDepthImagesMemory[i],
			this->vkDevice,
			this->vkPhysicalDevice
		);

		this->vkDepthImagesView[i] = GraphicsPipelineUtils::CreateImageView(
			this->vkDepthImages[i],
			depthFormat,
			VK_IMAGE_ASPECT_DEPTH_BIT,
			this->vkDevice);

		MemoryUtils::TransitionImageLayout(
			this->vkDepthImages[i],
			depthFormat,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			this->vkDevice,
			this->vkCommandPool,
			this->vkGraphicsQueue);
	}
}

void VulkanCore::RenderEngine::UpdateUniformBuffer(uint32_t currentImage)
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	const auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo = {};
	ubo.model = rotate(glm::mat4(1.0f), time * glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	ubo.view = lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	ubo.projection = glm::perspective(glm::radians(45.0f), this->vkExtent.width / static_cast<float>(this->vkExtent.height), 0.01f, 2000.0f);

	//ubo.projection[0][0] *= -1;
	ubo.projection[1][1] *= -1;
	//ubo.projection[2][2] *= -1;
	//ubo.projection[3][3] *= -1;

	void* data;
	vkMapMemory(this->vkDevice, this->vkUniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(this->vkDevice, this->vkUniformBuffersMemory[currentImage]);
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
	const auto func = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(
		instance, "vkCreateDebugReportCallbackEXT"));

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
	const auto func = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(
		instance, "vkDestroyDebugReportCallbackEXT"));

	if (func != nullptr)
	{
		func(instance, callback, pAllocator);
	}
}

bool VulkanCore::RenderEngine::CheckVkValidationLayerSupport() const
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

std::vector<const char*> VulkanCore::RenderEngine::GetRequiredExtensions() const
{
	uint32_t glfwExtensionCount = 0;

	const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (this->enableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	return extensions;
}

bool VulkanCore::RenderEngine::ThrottleCheck() const
{
	const auto currentTime = std::chrono::system_clock::now();

	const double dilation = std::chrono::duration<double, std::milli>(currentTime - this->lastDrawTime).count();

	return dilation < 1000/60;
}
