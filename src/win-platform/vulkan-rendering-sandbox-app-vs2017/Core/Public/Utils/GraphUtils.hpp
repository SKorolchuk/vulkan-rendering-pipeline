#ifndef _GRAPH_UTILS_HPP_
#define	_GRAPH_UTILS_HPP_

#define GLM_FORCE_RADIANS
#include <vulkan/vulkan.h>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

struct Vertex
{
	glm::vec3 position;
	glm::vec3 color;
	glm::vec2 textureCoords;

	static VkVertexInputBindingDescription GetBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions();
	static std::vector<Vertex> GetSampleVertexMatrix();
	static std::vector<uint16_t> GetSampleVertexIndices();
};

struct UniformBufferObject
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
};

struct GraphicsPipelineUtils
{
	static VkCommandBuffer BeginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);
	static void EndSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkCommandBuffer commandBuffer);
	static VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkDevice device);
	static VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
	                             VkFormatFeatureFlags features, VkPhysicalDevice physicalDevice);
	static VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice);
	static bool HasStencilComponent(VkFormat format);
};

#endif