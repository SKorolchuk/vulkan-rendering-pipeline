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

	protected:
		virtual void OpenWindow();
		virtual void Init();
		virtual void Wait();
		virtual void Loop();
		virtual void Update();
		virtual void Clean();
		GLFWwindow *window;
		RenderEngine *VkEngine;

	private:
		int width;
		int height;
		std::string title;
	};
}
