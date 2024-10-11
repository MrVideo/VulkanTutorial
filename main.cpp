#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vulkan/vk_platform.h"
#include "vulkan/vulkan_core.h"

#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <optional>
#include <set>

// Constants for the window size
const uint32_t WINDOW_WIDTH = 800;
const uint32_t WINDOW_HEIGHT = 600;

// Constant that lists the validation layers to be enabled
const std::vector<const char *> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

// Constant that lists the device extensions required
const std::vector<const char *> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
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

// This struct will contain all the queue family indices
// found in the available devices.
struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

// This struct will contain details about the swapchain
// supported by the selected GPU.
struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

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

	// The object that stores the presentation surface
	VkSurfaceKHR surface;

	// The object that stores the Vulkan instance
	VkInstance instance;

	// The object that stores the physical device which we
	// will use to render images.
	// NOTE: this does not have to be destroyed as it is 
	// implicitly destroyed when our instance is destroyed.
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	// The object that stores the logical device to send
	// rendering information to.
	VkDevice device;

	// The object that stores the handle to the swap chain
	VkSwapchainKHR swapChain;

	// The vector which will store the swap chain images
	std::vector<VkImage> swapChainImages;

	// The vector which will store the swap chain image views
	std::vector<VkImageView> swapChainImageViews;

	// The objects that store the image format and the extent
	// of the images in the swap chain.
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	// The object that stores the handle to our graphics queues and
	// presentation queues.
	// NOTE: queues are automatically created with our logical device,
	// however we need to manually get a handle for them to interact
	// with them explicitly. Queues are also automatically destroyed
	// when the logical device is destroyed.
	VkQueue graphicsQueue;
	VkQueue presentQueue;

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
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
    }

	void createSurface() {
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("[ERROR] Failed to create presentation surface!");
		}
	}

	// This function will simply pick one of the available GPUs in the system
	// to be used as our rendering device.
	void pickPhysicalDevice() {
		// Here, we simply list all the Vulkan-compatible devices available;
		// if there are none, an error is thrown.
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (deviceCount == 0) {
			throw std::runtime_error("[ERROR] Failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		std::cout << "[INFO ] Found " << deviceCount << " physical devices\n";

		// Now, we check whether the available devices are suitable for our uses.
		for (const auto &device : devices) {
			if (isDeviceSuitable(device)) {
				physicalDevice = device;
				break;
			}
		}

		if (physicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("[ERROR] No suitable GPU found!");
		}
	}

	// This function checks the properties of a given GPU passed to it
	// and returns true if it is suitable for our use.
	// NOTE: as this is a basic tutorial, we will settle for any available GPU
	// that has a graphics queue family available.
	// The commented function is just to show a simple method of choosing a
	// GPU based on its abilities.
	// 
	// bool isDeviceSuitable(VkPhysicalDevice device) {
	// 	// Here we check for the GPU's basic properties, like its name,
	// 	// type, and supported Vulkan version.
	// 	VkPhysicalDeviceProperties deviceProperties;
	// 	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	// 	// Here we check optional features, such as texture compression
	// 	// or 64-bit floats.
	// 	VkPhysicalDeviceFeatures deviceFeatures;
	// 	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	// 	return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader;
	// }
	bool isDeviceSuitable(VkPhysicalDevice device) {
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		
		QueueFamilyIndices indices = findQueueFamilies(device);

		bool extensionsSupported = checkDeviceExtensionSupport(device);

		bool swapChainAdequate = false;
		if (extensionsSupported) {
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		if (indices.isComplete() && extensionsSupported && swapChainAdequate) {
			std::cout << "[INFO ] Device " << deviceProperties.deviceName << " selected!\n";
			return true;
		}

		return false;
	}

	bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto &extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	// This function returns the available queue families
	// for the device passed to it.
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
		QueueFamilyIndices indices;

		// This code retrieves the queue families available for a particular device.
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto &queueFamily: queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}

			// This code checks whether the selected device supports image presentation.
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

			if (presentSupport) {
				indices.presentFamily = i;
			}

			if (indices.isComplete()) {
				break;
			}

			i++;
		}

		return indices;
	}

	// This function will gather information about the available swapchains
	// for the selected GPU and return them.
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	// This function selects the best available surface format gathered from
	// the swap chain information.
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
		// If we find our optimal surface format, we return that.
		for (const auto &availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		// Otherwise, we simply return the first format available.
		return availableFormats[0];
	}

	// This function selects the best available presentation mode available
	// as gathered from the swap chain information.
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
		// If the selected GPU can perform triple buffering, we select that.
		for (const auto &availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}

		// Otherwise, the vertical sync mode is selected.
		// NOTE: this is the only mode that is guaranteed to be there for
		// every physical device.
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	// This function selects the best available swap extent for the selected
	// GPU (which represents the resolution of each swap chain image).
	// NOTE: the explanation on how this works is very complex and it wouldn't make
	// sense to summarise it here: check the URL below to learn more.
	// https://vulkan-tutorial.com/en/Drawing_a_triangle/Presentation/Swap_chain#page_Swap-extent
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		} else {
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	// This function creates the logical device starting from the physical
	// device picked beforehand and based on its available queue families.
	void createLogicalDevice() {
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };
		
		// It is required to add a queue priority (a decimal number between 0.0 and
		// 1.0) even if a single queue is present.
		float queuePriority = 1.0f;

		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};

			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			
			// We don't really need more than one queue for now as the command buffers
			// can be created on multiple threads and then can be submitted all at once 
			// on the main thread.
			queueCreateInfo.queueCount = 1;

			queueCreateInfo.pQueuePriorities = &queuePriority;

			queueCreateInfos.push_back(queueCreateInfo);
		}

		// Here we specify which set of device features we want to use.
		// NOTE: since we don't need anything special for now, we can just
		// leave this struct empty (all its values are set to VK_FALSE).
		VkPhysicalDeviceFeatures deviceFeatures{};

		// Now we can effectively fill the logical device struct with
		// the required information.
		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		} else {
			createInfo.enabledLayerCount = 0;
		}

		// Now we can finally instantiate our logical device.
		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
			throw std::runtime_error("[ERROR] Failed to create logical device!");
		}

		std::cout << "[INFO ] Logical device created!\n";

		// Finally, we get a handle for the newly-created queues.
		// NOTE: as we are creating a single queue, we can just use index 0 for it.
		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);

		std::cout << "[INFO ] Device queues retrieved!\n";
	}

	// This function uses the helper functions defined prior to create the swap chain
	// for the selected GPU. Furthermore, it decides how many images should be in the
	// swap chain.
	void createSwapChain() {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		// Here we choose how many images the swap chain should contain.
		// Ideally, we want to set this number to the minimum images supported plus one, so
		// to avoid waiting for another frame if the application is lagging behind.
		// However, if the maxImageCount in the swap chain support capabilities is set to zero,
		// it means that there is no maximum set by the GPU, thus we can produce an unlimited
		// amount of images to be put in the swap chain.
		// Here, we also check if the maximum is set (it is greater than zero) and if imageCount
		// is beyond the set maximum, we simply clamp it to that value.
		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		// Now we create our usual struct to build the swap chain.
		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		// Here, we specify how to handle swap chain images which will be 
		// used across multiple queue families.
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		// NOTE: in order to avoid complex ownership changes, if the
		// graphics and presentation queue families differ, we will
		// just use the concurrent sharing mode for now, even though
		// this may impact performance.
		// As in most devices the queue families are the same, we
		// can be pretty sure that the exclusive mode will be selected
		// in this case.
		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		} else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;	  // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}

		// This specifies whether we want a certain transform to be
		// applied to all images in the swap chain.
		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

		// This specifies whether the alpha channel should be used 
		// for blending with other windows in the window system.
		// Most of the time, this remains untouched.
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		// This simply specifies the selected present mode and tells Vulkan
		// that we do not care about the colour of pixels hidden
		// behind other windows. This improves performance.
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		// This specifies whether another swap chain should be substituted
		// by the newly created one. This is a very complex topic which
		// we will explore much later.
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
			throw std::runtime_error("[ERROR] Failed to create swap chain!");
		}

		std::cout << "[INFO ] Swap chain created!\n";

		// Here we retrieve the images in the swap chain to be able
		// to handle them later.
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

		std::cout << "[INFO ] Swap chain image handles retrieved!\n";

		// Finally, we save the extent and format of the swap chain images.
		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;

		std::cout << "[INFO ] Image format and extent saved!\n";
	}

	void createImageViews() {
		swapChainImageViews.resize(swapChainImages.size());

		for (size_t i = 0; i < swapChainImages.size(); i++) {
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChainImages[i];

			// This specifies how the image data should be interpreted.
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapChainImageFormat;

			// This specifies how to manage colour channels.
			// For example, for a monochrome texture, all channels 
			// could be mapped to the red channel.
			// In this case, we simply leave the default mapping.
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			// This specifies what the image's purpose is and 
			// which part of the image should be accessed.
			// In this case, we use images as colour targets,
			// with no mipmapping levels or multiple layers.
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("[ERROR] Failed to create image view!");
			}
		}

		std::cout << "[INFO ] Image views created!\n";
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
		// Destroys the swap chain image views
		for (auto imageView : swapChainImageViews) {
			vkDestroyImageView(device, imageView, nullptr);
		}

		// Destroys the swap chain
		vkDestroySwapchainKHR(device, swapChain, nullptr);

		// Destroys the logical device
		vkDestroyDevice(device, nullptr);
		
		// If the validation layers were enabled, this destroys
		// the debug messenger created beforehand.
		if (enableValidationLayers) {
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}

		// Destroys the presentation surface
		vkDestroySurfaceKHR(instance, surface, nullptr);
		
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
