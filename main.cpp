#include "vulkan/vulkan_core.h"
#include <cstdint>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

// Constants for the window size
const uint32_t WINDOW_WIDTH = 800;
const uint32_t WINDOW_HEIGHT = 600;

class HelloTriangleApplication {
public:
    void run() {
		initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
	// The object that stores the window created by GLFW
	GLFWwindow *window;

	// The object that stores the Vulkan instance
	VkInstance instance;

	void initWindow() {
		// Initialises GLFW
		glfwInit();

		// Tells GLFW not to create an OpenGL context (we are using Vulkan)
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		// Disables window resizing (creates complications at the beginning)
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		// Creates the actual window and assigns it to its object
		// Parameters:
		// 1. Window width
		// 2. Window height
		// 3. Window title
		// 4. Monitor to open the window on
		// 5. Only relevant to OpenGL
		window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", nullptr, nullptr);
	}

    void initVulkan() {
		createInstance();
    }

	void createInstance() {
		// This struct contains info about the instance we are creating.
		// This struct is optional.
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		// This struct tells the Vulkan driver which global extensions
		// and validation layers we want to use in our application.
		// This struct is not optional.
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		// This code checks for all available extensions in the current
		// Vulkan configuration.
		uint32_t availableExtensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, availableExtensions.data());

		std::cout << "[INFO ] " << availableExtensionCount << " available extensions:\n";
		for (const auto &extension: availableExtensions) {
			std::cout << "\t" << extension.extensionName << "\n";
		}

		// This code returns the GLFW extensions and adds them to the
		// createInfo struct.
		uint32_t glfwExtensionCount = 0;
		const char **glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		createInfo.enabledExtensionCount = glfwExtensionCount;
		createInfo.ppEnabledExtensionNames = glfwExtensions;

		// This code specifies the extension layers to be enabled.
		// For now, this will remain empty.
		createInfo.enabledLayerCount = 0;

		// NOTE: this code is here to fix a macOS error, as explained in the tutorial.
		// In macOS, starting from Vulkan SDK version 1.3.216, the VK_KHR_PORTABILITY_subset
		// extension is mandatory.
		// Conditional compilation was used to avoid cluttering binaries for Linux and Windows.

		#ifdef __APPLE__
		std::vector<const char*> requiredExtensions;

		for (uint32_t i = 0; i < glfwExtensionCount; i++) {
			requiredExtensions.emplace_back(glfwExtensions[i]);
		}

		requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

		createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

		createInfo.enabledExtensionCount = (uint32_t) requiredExtensions.size();
		createInfo.ppEnabledExtensionNames = requiredExtensions.data();

		std::cout << "[INFO ] Loaded extensions required for macOS\n";
		#endif

		// Finally, we can actually create our instance.
		// The function takes three arguments:
		// 1. A pointer to the struct with the creation info (createInfo)
		// 2. A pointer to custom allocator callbacks (left empty in this tutorial)
		// 3. A pointer to the object that stores our instance (instance)
		VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

		// Now we check whether this operation executed successfully and, if not,
		// we throw a runtime error.
		if (result != VK_SUCCESS) {
			std::cout << result << std::endl;
			throw std::runtime_error("[ERROR] Failed to create instance!");
		}
	}

    void mainLoop() {
		// While the window is open, check for events (like key presses)
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
		}
    }

    void cleanup() {
		// Destroys the vkInstance
		vkDestroyInstance(instance, nullptr);

		// Destroys the application window
		glfwDestroyWindow(window);

		// Terminates GLFW
		glfwTerminate();
    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
