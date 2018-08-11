
# Vulkan Rendering Pipeline Examples

# Demo Preview

[![Vulkan API Rendering Engine - Milestone Demo, Code & Sample Assets](http://img.youtube.com/vi/NmwvGMmnQ0M/0.jpg)](https://www.youtube.com/watch?v=NmwvGMmnQ0M "VulkanAPI Rendering Engine - Milestone Demo, Code & Sample Assets")

# Repository contains set of example applications based on Vulkan API

# `VulkanRenderApp` Visual Studio 2017 Solution:

## Used technologies:
- `GLFW` (submodule reference)
- `GLM` ((submodule reference)
- `OpenCV` (submodule reference)
- `Vulkan SDK v.1.1.73.0` (external dependency, see `Build & Install` for details)
- `C++17 STD and STL libs`  (environment dependencies)
- `Visual Studio 2017 C++ Compiler and Windows 10 SDK` (environment dependencies)

--------------------------------------------------------

## CMake integration will be added after `VulkanRenderApp` stabilization

--------------------------------------------------------

 ## Features in progress:
- ### Add `SPIR-V` shader integration (Integrate auto-align of shader text size)
- ### Add rendering of `.OBJ` models
- ### Add mipmaps in pipeline
- ### Add tessellation control shaders
- ### Add Scene graph file format
- ### Add UI scene visualization controls
- ### Simple OpenCV texture pre-processing
- ### Integrate Cross-platform sound library

--------------------------------------------------------

## These resources are being used in development process of `VulkanRenderApp` application:
- ### [Vulkan Tutorials](https://vulkan-tutorial.com)
- ### [Vulkan Cookbook](https://www.amazon.com/Vulkan-Cookbook-Pawel-Lapinski/dp/1786468158)
- ### [Vulkan Programming Guide](https://www.amazon.com/Vulkan-Programming-Guide-Official-Learning/dp/0134464540)

- ## Build & Installation
 	- ### Using of examples required `Visual Studio 2017.5 Community Edition` and `Windows 10 SDK` pre-installed
	- ### Clone your local copy of the repository
	- ### Install [LunarG Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) into directory with the same level as `repository` folder
		- For ex., 
		- `|-..`
		- `|- repository-dir`
		- `|- VulkanSDK`
	- ### Compile `GLFW` static library
		- Uncheck `BUILD_SHARED_LIBS`
		- Uncheck `GLFW_INSTALL`
		- Use `CMAKE_INSTALL_PREFIX` with the same path as `glfw` module in your local repository copy
	- ### Open `src\win-platform\vulkan-rendering-sandbox-app-vs2017\VulkanRenderApp.sln` and Build solution
	- ### Launch example application from VS IDE or compiled file
