#ifndef _IO_UTILS_HPP_
#define	_IO_UTILS_HPP_

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <string>
#include <vulkan/vulkan.h>
#include <vector>
#include <fstream>

class ShaderExtensions
{
public:
	static std::vector<char> ReadShaderFile(const std::string& fileName);
	static VkShaderModule CreateShaderModule(const VkDevice& device, const std::vector<char>& shaderText);
	static stbi_uc* CreateTextureImage(const char* filePath, int* textureWidth, int* textureHeight, int* textureChannels);
};

#endif
