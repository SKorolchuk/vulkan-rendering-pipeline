#ifndef _RENDER_ENGINE_HPP_
#define	_RENDER_ENGINE_HPP_

#include <chrono>
#include <vector>
#include <iostream>
#include <set>
#include <map>
#include "Utils/MemoryUtils.hpp"
#include "Utils/IOUtils.hpp"
#include "Infrastructure/Extensions/QueueFamilyIndices.hpp"
#include "Infrastructure/Extensions/SwapChainSupportDetails.hpp"

namespace VulkanCore
{
	 class RenderEngine
	 {
	 public:
		 const static int MAX_FRAMES_IN_FLIGHT = 2;
		 RenderEngine(int width, int height, GLFWwindow* window);
		 ~RenderEngine();
		 virtual void BootstrapPipeline();
		 virtual void CleanPipeline();
		 virtual void Draw();
		 virtual void WaitDevice();
		 bool FrameBufferResized = false;

	 protected:
		 int ViewportWidth;
		 int ViewportHeight;
		 GLFWwindow *GLWindow = nullptr;
		 bool IsPipelineInitialized = false;
		 bool CheckVkValidationLayerSupport() const;
		 VkResult CreateVkInstanceWithCheck(
			 const VkInstanceCreateInfo* pCreateInfo,
			 const VkAllocationCallbacks* pAllocator,
			 VkInstance* instance);
		 VkResult CreateVkDebugReportCallback(
			 VkInstance instance,
			 const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
			 const VkAllocationCallbacks* pAllocator,
			 VkDebugReportCallbackEXT* pCallback
		 );
		 static void DestroyVkDebugReportCallback(
			 VkInstance instance,
			 VkDebugReportCallbackEXT callback,
			 const VkAllocationCallbacks* pAllocator);
		 std::vector<const char*> GetRequiredExtensions() const;
		 bool ThrottleCheck() const;
		 void CreateVulkanInstance();
		 void SetupDebugCallback();
		 void CreateLogicalDevice();
		 void CreateSwapChain();
		 void PickPhysicalDevice();
		 void CreateSurface();
		 void CreateImageViews();
		 void LoadTextures();
		 void CreateTextureViews();
		 void CreateDescriptorSetLayout();
		 void CreateGraphicsPipeline();
		 void CreateFrameBuffers();
		 void CreateCommandPool();
		 void CreateCommandBuffers();
		 void CreatePipelineSyncObjects();
		 void CleanSwapChain();
		 void UpdateSwapChain();
		 void CreateDescriptorSet();
		 bool IsDeviceSuitable(VkPhysicalDevice device) const;
		 bool CheckDeviceExtensionsSupport(VkPhysicalDevice device) const;
		 void CreateRenderPass();
		 void CreateVertexBuffer();
		 void CreateIndexBuffer();
		 void CreateDescriptorPool();
		 void CreateUniformBuffer();
		 void UpdateUniformBuffer(uint32_t ImageIndex);
		 static int RateDeviceSuitability(VkPhysicalDevice device);
		 QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
		 VkInstance vkInstance;
		 VkDebugReportCallbackEXT vkCallback;
		 VkPhysicalDevice vkPhysicalDevice = VK_NULL_HANDLE;
		 VkDevice vkDevice;
		 VkQueue vkGraphicsQueue;
		 VkQueue vkPresentQueue;
		 VkSurfaceKHR vkSurface;
		 VkSwapchainKHR vkSwapChain;
		 std::vector<VkImage> vkSwapChainImages;
		 std::vector<VkImageView> vkSwapChainImageViews;
		 std::vector<VkFramebuffer> vkSwapChainFrameBuffers;
		 VkFormat vkSwapChainImageFormat;
		 VkExtent2D vkExtent;
		 VkColorSpaceKHR vkSwapChainImageColorSpace;

		 VkRenderPass vkRenderPass;
		 VkDescriptorSetLayout vkDescriptorSetLayout;
		 VkDescriptorPool vkDescriptorPool;
		 std::vector<VkDescriptorSet> vkDescriptorSets;

		 VkPipelineLayout vkPipelineLayout;
		 VkPipeline vkGraphicsPipeline;

		 // buffers

		 VkBuffer vkVertexBuffer;
		 VkDeviceMemory vkVertexBufferMemory;
		 VkBuffer vkIndexBuffer;
		 VkDeviceMemory vkIndexBufferMemory;

		 std::vector<VkBuffer> vkUniformBuffers;
		 std::vector<VkDeviceMemory> vkUniformBuffersMemory;

		 // semaphores

		 std::vector<VkSemaphore> vkImageAvailableSemLocks;
		 std::vector<VkSemaphore> vkRenderFinishedSemLocks;
		 std::vector<VkFence> vkInFlightSyncFences;
		 size_t currentFrame = 0;

		 // command pool and etc.

		 VkCommandPool vkCommandPool;
		 std::vector<VkCommandBuffer> vkCommandBuffers;

		 // Textures

		 stbi_uc* PixelBuffer;
		 VkImage vkTextureImage;
		 VkDeviceMemory vkTextureImageMemory;
		 VkImageView vkTextureImageView;

		 const bool enableValidationLayers = true;
		 const std::vector<const char*> validationLayers = {
			 "VK_LAYER_LUNARG_standard_validation"
		 };
		 const std::vector<const char*> deviceExtensions = {
			 VK_KHR_SWAPCHAIN_EXTENSION_NAME
		 };

		 // shaders

		 VkShaderModule vkVertrexShader;
		 VkShaderModule vkFragmentShader;

	 private:
		std::chrono::time_point
	 		<std::chrono::system_clock,
	 		std::chrono::milliseconds> lastDrawTime;
	 };

	 static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		 VkDebugReportFlagsEXT flags,
		 VkDebugReportObjectTypeEXT objType,
		 uint64_t obj,
		 size_t location,
		 int32_t code,
		 const char* layerPrefix,
		 const char* msg,
		 void* userData) {

		 std::cerr << "validation layer: " << msg << std::endl;

		 return VK_FALSE;
	 }
}

#endif
