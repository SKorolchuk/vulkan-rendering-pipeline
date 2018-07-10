#pragma once

namespace VulkanCore
{
	class EndPointApplication
	{
	public:
		EndPointApplication();
		EndPointApplication(int witdth, int height, const char* title);
		~EndPointApplication();
		virtual void Run();
		RenderEngine *VkEngine;

	protected:
		virtual void OpenWindow();
		virtual void Init();
		virtual void Wait();
		virtual void Loop();
		virtual void Update();
		virtual void Clean();
		GLFWwindow *window;

	private:
		int width;
		int height;
		std::string title;
	};

	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		auto app = reinterpret_cast<EndPointApplication*>(glfwGetWindowUserPointer(window));
		app->VkEngine->FrameBufferResized = true;
	}
}
