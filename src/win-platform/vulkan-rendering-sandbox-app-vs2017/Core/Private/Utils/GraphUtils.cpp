#include "../../Public/Utils/GraphUtils.hpp"

std::array<VkVertexInputAttributeDescription, 3> Vertex::GetAttributeDescriptions()
{
	std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, position);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, color);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex, textureCoords);

	return attributeDescriptions;
}

VkVertexInputBindingDescription Vertex::GetBindingDescription()
{
	VkVertexInputBindingDescription bindDescription = {};
	bindDescription.binding = 0;
	bindDescription.stride = sizeof(Vertex);
	bindDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindDescription;
}

std::vector<Vertex> Vertex::GetSampleVertexMatrix()
{
	return {
		// plane 1

		{
			{ -1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }
		},
		{
			{ 1.0f, -1.0f, 0.0f },  { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }
		},
		{
			{ 1.0f, 1.0f, 0.0f },   { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }
		},
		{
			{ -1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f },{ 1.0f, 1.0f }
		},

		// plane 2

		{
			{ -1.0f, -1.0f, 1.0f },{ 1.0f, 0.0f, 0.0f },{ 1.0f, 0.0f }
		},
		{
			{ 1.0f, -1.0f, 1.0f },{ 0.0f, 1.0f, 0.0f },{ 0.0f, 0.0f }
		},
		{
			{ 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, 1.0f },{ 0.0f, 1.0f }
		},
		{
			{ -1.0f, 1.0f, 1.0f },{ 1.0f, 1.0f, 1.0f },{ 1.0f, 1.0f }
		}
	};
}

std::vector<uint16_t> Vertex::GetSampleVertexIndices()
{
	return {
		0, 1, 2, 2, 3, 0,
		4, 5, 6, 6, 7, 4,

		4, 0, 1, 0, 4, 7,
		5, 4, 1, 1, 6, 5,
		6, 1, 2, 2, 7, 6,
		7, 2, 3, 3, 0, 7
	};
}

VkCommandBuffer GraphicsPipelineUtils::BeginSingleTimeCommands(
	VkDevice device,
	VkCommandPool commandPool)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void GraphicsPipelineUtils::EndSingleTimeCommands(
	VkDevice device,
	VkCommandPool commandPool,
	VkQueue graphicsQueue,
	VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

VkImageView GraphicsPipelineUtils::CreateImageView(
	VkImage image,
	VkFormat format,
	VkImageAspectFlags aspectFlags,
	VkDevice device) {
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}

VkFormat GraphicsPipelineUtils::FindSupportedFormat(
	const std::vector<VkFormat>& candidates,
	VkImageTiling tiling,
	VkFormatFeatureFlags features,
	VkPhysicalDevice physicalDevice) {

	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}

		if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

VkFormat GraphicsPipelineUtils::FindDepthFormat(VkPhysicalDevice physicalDevice) {
	return FindSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
		physicalDevice
	);
}

bool GraphicsPipelineUtils::HasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}