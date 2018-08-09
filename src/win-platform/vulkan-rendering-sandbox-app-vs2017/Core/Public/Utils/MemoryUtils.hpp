#ifndef _MEMORY_UTILS_HPP_
#define	_MEMORY_UTILS_HPP_

#include "GraphUtils.hpp"
#include <vulkan/vulkan.h>

namespace MemoryUtils
{
	void CreateBuffer(
		VkDeviceSize size,
		VkBufferUsageFlags usageFlags,
		VkMemoryPropertyFlags properties,
		VkDevice device,
		VkPhysicalDevice physicalDevice,
		VkBuffer& buffer,
		VkDeviceMemory& bufferMemory);

	uint32_t FindMemoryType(
		uint32_t typeFilter,
		VkMemoryPropertyFlags properties,
		VkPhysicalDevice physicalDevice);

	void CopyBuffer(
		VkBuffer srcBuffer,
		VkBuffer dstBuffer,
		VkDeviceSize size,
		VkCommandPool commandPool,
		VkDevice device,
		VkQueue graphicsQueue);

	void CreateImage(
		uint32_t width,
		uint32_t height,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkImage& image,
		VkDeviceMemory& imageMemory,
		VkDevice device,
		VkPhysicalDevice physicalDevice);
	uint32_t FindMemoryType(
		VkPhysicalDevice physicalDevice,
		uint32_t typeFilter,
		VkMemoryPropertyFlags properties);

	void TransitionImageLayout(
		VkImage image,
		VkFormat format,
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		VkDevice device,
		VkCommandPool commandPool,
		VkQueue graphicsQueue);

	void CopyBufferToImage(
		VkBuffer buffer,
		VkImage image,
		uint32_t width,
		uint32_t height,
		VkDevice device,
		VkCommandPool commandPool,
		VkQueue graphicsQueue);
}

#endif