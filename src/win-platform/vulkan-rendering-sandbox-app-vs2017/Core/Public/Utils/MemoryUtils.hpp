#ifndef _MEMORY_UTILS_HPP_
#define	_MEMORY_UTILS_HPP_

#include <vulkan/vulkan.h>
#include <vector>

namespace MemoryUtils
{
	void CreateBuffer(
		VkDeviceSize size,
		VkBufferUsageFlags usageFlags,
		VkMemoryPropertyFlags properties,
		VkDevice& device,
		VkPhysicalDevice& physicalDevice,
		VkBuffer& buffer,
		VkDeviceMemory& bufferMemory);

	uint32_t FindMemoryType(
		uint32_t typeFilter,
		VkMemoryPropertyFlags properties,
		VkPhysicalDevice& physicalDevice);

	void CopyBuffer(
		VkBuffer srcBuffer,
		VkBuffer dstBuffer,
		VkDeviceSize size,
		VkCommandPool& commandPool,
		VkDevice& device,
		VkQueue& graphicsQueue);
}
#endif