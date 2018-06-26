// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <string>
#include <vector>
#include <iostream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <vulkan/vulkan.h> 
#include <map>
#include <set>
#include <algorithm>

#include "Public/QueueFamilyIndices.hpp"
#include "Public/SwapChainSupportDetails.hpp"
#include "Public/VulkanEndPointApplication.hpp"


// TODO: reference additional headers your program requires here
