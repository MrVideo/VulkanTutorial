#include "vulkan/vk_platform.h"
#include "vulkan/vulkan_core.h"
#include <cstdint>
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

// Constants for the window size
const uint32_t WINDOW_WIDTH = 800;
const uint32_t WINDOW_HEIGHT = 600;

// Constant that lists the validation layers to be enabled
const std::vector<const char *> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

// To define whether or not the validation layers should be
// enabled, the compiler checks for the definition of the 
// NDEBUG macro, which disables the loading of validation
// layers if the application is compiled with a "no debug"
// flag.
#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

// This code is used to load the vkCreateDebugUtilsMessengerEXT function, which
// is not loaded automatically because it comes from an extension.
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
									  const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
									  const VkAllocationCallbacks *pAllocator,
									  VkDebugUtilsMessengerEXT *pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

// This code is used to destroy the debug messenger the same way the function above
// is used to create it.
void DestroyDebugUtilsMessengerEXT(VkInstance instance,
								   VkDebugUtilsMessengerEXT debugMessenger,
								   const VkAllocationCallbacks *pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

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

	// The object that stores our custom callback messenger
	VkDebugUtilsMessengerEXT debugMessenger;

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
		setupDebugMessenger();
    }

	void createInstance() {
		// Check whether the required validation layers are available and throw
		// an error if they are not.
		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("[ERROR] Validation layers requested, but not available!");
		}

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

		// This code checks whether the requested validation layers should be loaded and,
		// if so, adds them to the createInfo struct.
		// This also creates the debug messenger information and enables the debug messages
		// for the creation and destruction of said debug messenger.
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

		if (enableValidationLayers) {
			uint32_t enabledLayers = static_cast<uint32_t>(validationLayers.size());
			createInfo.enabledLayerCount = enabledLayers;
			createInfo.ppEnabledLayerNames = validationLayers.data();
			std::cout << "[INFO ] " << enabledLayers << " validation layers requested.\n";

			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;
			std::cout << "[INFO ] Created debug messenger\n";
		} else {
			createInfo.enabledLayerCount = 0;
			std::cout << "[INFO ] No validation layers requested.\n";

			createInfo.pNext = nullptr;
		}

		// Here, we use the getRequiredExtensions function to retrieve the number
		// of required extensions to add to our createInfo struct.
		auto extensions = getRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		// NOTE: this code is here to fix a macOS error, as explained in the tutorial.
		// In macOS, starting from Vulkan SDK version 1.3.216, the VK_KHR_PORTABILITY_subset
		// extension is mandatory.
		// Conditional compilation was used to avoid cluttering binaries for Linux and Windows.

		#ifdef __APPLE__
		std::vector<const char*> requiredExtensions;

		for (uint32_t i = 0; i < static_cast<uint32_t>(extensions.size()); i++) {
			requiredExtensions.emplace_back(extensions[i]);
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

	// This function sets up the custom debug messenger we created.
	void setupDebugMessenger() {
		if (!enableValidationLayers) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		populateDebugMessengerCreateInfo(createInfo);
		
		if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			throw std::runtime_error("[ERROR] Failed to setup the debug messenger!");
		}
	}

	// This function populates the createInfo struct for the debug messenger.
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
								   | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
								   | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
								   | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
							   | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
							   | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr; // This is optional
	}

	// This function checks whether all validation layers that are requested
	// in validationLayers are available. It returns true if all requested
	// layers are available, and false otherwise.
	bool checkValidationLayerSupport() {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char *layerName : validationLayers) {
			bool layerFound = false;

			for (const auto &layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				return false;
			}
		}

		return true;
	}

	// This function returns the required list of extensions depending on whether
	// validation layers are enabled or not.
	std::vector<const char *> getRequiredExtensions() {
		uint32_t glfwExtensionCount = 0;
		const char **glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (enableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	// This is a callback function we will use to redirect some output
	// from extensions to stdout (and to suppress some if needed too).
	//
	// The first parameter of the function specifies the severity of the message, which can be:
	// - VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: Diagnostic message
	// - VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT   : Information message
	// - VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: Warning message
	// - VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT  : Error message
	//
	// This can be used to show only certain messages, as the values grow with the severity of the message
	// (i.e. if we check for a severity greater or equal to WARNING, we only get warnings and errors).
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
														VkDebugUtilsMessageTypeFlagsEXT messageType,
														const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
														void *pUserData) {
		std::cerr << "[ERROR] Validation layer: " << pCallbackData->pMessage << std::endl;
		return VK_FALSE;
	}

    void mainLoop() {
		// While the window is open, check for events (like key presses)
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
		}
    }

    void cleanup() {
		// If the validation layers were enabled, this destroys
		// the debug messenger created beforehand.
		if (enableValidationLayers) {
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}
		
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
