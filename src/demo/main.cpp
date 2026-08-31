#include "Renderer/VulkanRenderer.hpp"

int main()
{
	try
	{
		HelloVulkan::HelloTriangleApplication app;
		app.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}