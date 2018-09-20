#ifndef _END_POINT_APP_HPP_
#define	_END_POINT_APP_HPP_

#include "RenderEngine.hpp"

namespace VulkanCore
{
	class EndPointApplication
	{
	public:
		EndPointApplication();
		EndPointApplication(
			int witdth,
			int height,
			const char* title,
			const char* modelPath,
			const char* baseColorTexturePath
		);
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
		std::string modelPath;
		std::string baseColorTexturePath;
	};

	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		auto app = reinterpret_cast<EndPointApplication*>(glfwGetWindowUserPointer(window));
		app->VkEngine->FrameBufferResized = true;
	}
}

#endif
