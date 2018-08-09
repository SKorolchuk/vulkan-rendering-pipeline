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
	glm::vec2 position;
	glm::vec3 color;

	static VkVertexInputBindingDescription GetBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions();
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
	static VkImageView CreateImageView(VkImage image, VkFormat format, VkDevice device);
};

#endif