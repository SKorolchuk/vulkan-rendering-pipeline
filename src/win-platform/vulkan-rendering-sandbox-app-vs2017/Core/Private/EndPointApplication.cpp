#include "../Public/EndPointApplication.hpp"

VulkanCore::EndPointApplication::EndPointApplication() :
	width(1280),
	height(1024),
	title("Vulkan App")
{
}

VulkanCore::EndPointApplication::EndPointApplication(
	int witdth,
	int height,
	const char* title) :
	width(width),
	height(height),
	title(title)
{
}

VulkanCore::EndPointApplication::~EndPointApplication()
{
	delete this->VkEngine;
}

void VulkanCore::EndPointApplication::Run()
{
	this->OpenWindow();
	this->Init();
	this->Loop();
	this->Clean();
}

void VulkanCore::EndPointApplication::OpenWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	this->window = glfwCreateWindow(this->width, this->height, this->title.c_str(), nullptr, nullptr);
	glfwSetWindowUserPointer(this->window, this);
	glfwSetFramebufferSizeCallback(this->window, FramebufferResizeCallback);

	this->VkEngine = new RenderEngine(this->width, this->height, this->window);
}

void VulkanCore::EndPointApplication::Init()
{
	this->VkEngine->BootstrapPipeline();
}

void VulkanCore::EndPointApplication::Wait()
{
	this->VkEngine->WaitDevice();
}

void VulkanCore::EndPointApplication::Update()
{
	this->VkEngine->Draw();
}

void VulkanCore::EndPointApplication::Loop()
{
	while (!glfwWindowShouldClose(this->window))
	{
		glfwPollEvents();
		this->Update();
	}

	this->Wait();
}

void VulkanCore::EndPointApplication::Clean()
{
	this->VkEngine->CleanPipeline();

	glfwDestroyWindow(this->window);
	glfwTerminate();
}


