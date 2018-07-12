#pragma once

#ifndef _IO_UTILS_HPP_
#define	_IO_UTILS_HPP_

#include <string>
#include <vulkan/vulkan.h>
#include <vector>
#include <fstream>

class ShaderExtensions
{
public:
	static std::vector<char> ReadShaderFile(const std::string& fileName);
	static VkShaderModule CreateShaderModule(const VkDevice& device, const std::vector<char>& shaderText);
};

#endif
