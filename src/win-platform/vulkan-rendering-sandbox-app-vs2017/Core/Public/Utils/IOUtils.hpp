#ifndef _IO_UTILS_HPP_
#define	_IO_UTILS_HPP_

#include <stb_image.h>

#include <string>
#include "MemoryUtils.hpp"
#include <vector>
#include <unordered_map>
#include <fstream>
#include <tiny_obj_loader.h>

class ShaderExtensions
{
public:
	static std::vector<char> ReadShaderFile(const std::string& fileName);
	static VkShaderModule CreateShaderModule(const VkDevice& device, const std::vector<char>& shaderText);
	static stbi_uc* CreateTextureImage(const char* filePath, int* textureWidth, int* textureHeight, int* textureChannels);
};

class MeshExtensions
{
public:
	static void LoadModelToMemoryBuffer(
		const char* modelPath,
		VkDevice device,
		VkPhysicalDevice physicalDevice,
		VkCommandPool commandPool,
		VkQueue graphicsQueue,
		VkBuffer& vertexBuffer,
		VkDeviceMemory& vertexBufferMemory,
		VkBuffer& indexBuffer,
		VkDeviceMemory& indexBufferMemory);
};

#endif
