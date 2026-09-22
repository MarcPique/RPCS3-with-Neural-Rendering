// Headless integration smoke test: instance, layers, NVIDIA device and interop.
// This deliberately makes no claim about neural evaluation or game rendering.
#include "neural_rendering_config.h"
#include <QCoreApplication>
#include <QTextStream>
#include <vulkan/vulkan.h>
#include <cstring>
#include <vector>

int main(int argc, char** argv)
{
	QCoreApplication app(argc, argv);
	QTextStream out(stdout);
	if (!app.arguments().contains("--enable-local-runtime"))
	{
		out << "Run in a disposable runtime kit with --enable-local-runtime.\n";
		return 2;
	}
	QString error;
	if (!neural_rendering::save_enabled(true, &error))
	{
		out << error << '\n';
		return 3;
	}
	out << neural_rendering::initialize() << '\n';
	out << "VK_INSTANCE_LAYERS=" << qEnvironmentVariable("VK_INSTANCE_LAYERS") << '\n';
	uint32_t count = 0;
	if (vkEnumerateInstanceLayerProperties(&count, nullptr) != VK_SUCCESS) return 4;
	std::vector<VkLayerProperties> layers(count);
	if (vkEnumerateInstanceLayerProperties(&count, layers.data()) != VK_SUCCESS) return 4;
	bool reshade = false, feeder = false;
	for (const auto& layer : layers)
	{
		out << "Layer: " << layer.layerName << '\n';
		reshade |= std::strcmp(layer.layerName, "VK_LAYER_RPCS3_reshade") == 0;
		feeder |= std::strcmp(layer.layerName, "VK_LAYER_feed_vk") == 0;
	}
	if (!reshade || !feeder) { out << "FAIL: required layers were not discovered.\n"; return 5; }
	VkApplicationInfo application{VK_STRUCTURE_TYPE_APPLICATION_INFO};
	application.pApplicationName = "RPCS3 Neural Vulkan smoke";
	application.apiVersion = VK_API_VERSION_1_3;
	VkInstanceCreateInfo create{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
	create.pApplicationInfo = &application;
	VkInstance instance = VK_NULL_HANDLE;
	VkResult status = vkCreateInstance(&create, nullptr, &instance);
	if (status != VK_SUCCESS) { out << "FAIL vkCreateInstance: " << status << '\n'; return 6; }
	count = 0;
	status = vkEnumeratePhysicalDevices(instance, &count, nullptr);
	if (status != VK_SUCCESS || count == 0) { vkDestroyInstance(instance, nullptr); return 7; }
	std::vector<VkPhysicalDevice> physical(count);
	status = vkEnumeratePhysicalDevices(instance, &count, physical.data());
	VkPhysicalDevice nvidia = VK_NULL_HANDLE;
	for (const auto& candidate : physical)
	{
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(candidate, &properties);
		out << "GPU: " << properties.deviceName << " vendor=" << properties.vendorID << '\n';
		if (properties.vendorID == 0x10de) nvidia = candidate;
	}
	if (status != VK_SUCCESS || !nvidia) { vkDestroyInstance(instance, nullptr); return 8; }
	count = 0;
	if (vkEnumerateDeviceExtensionProperties(nvidia, nullptr, &count, nullptr) != VK_SUCCESS) { vkDestroyInstance(instance, nullptr); return 9; }
	std::vector<VkExtensionProperties> extensions(count);
	if (vkEnumerateDeviceExtensionProperties(nvidia, nullptr, &count, extensions.data()) != VK_SUCCESS) { vkDestroyInstance(instance, nullptr); return 9; }
	for (const char* required : {"VK_KHR_external_memory_win32", "VK_KHR_external_semaphore_win32", "VK_KHR_timeline_semaphore"})
	{
		bool present = false;
		for (const auto& extension : extensions) present |= std::strcmp(extension.extensionName, required) == 0;
		out << required << ": " << (present ? "supported" : "MISSING") << '\n';
		if (!present) { vkDestroyInstance(instance, nullptr); return 10; }
	}
	count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(nvidia, &count, nullptr);
	std::vector<VkQueueFamilyProperties> families(count);
	vkGetPhysicalDeviceQueueFamilyProperties(nvidia, &count, families.data());
	uint32_t graphics_family = UINT32_MAX;
	for (uint32_t i = 0; i < count; ++i) if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) { graphics_family = i; break; }
	if (graphics_family == UINT32_MAX) { vkDestroyInstance(instance, nullptr); return 11; }
	float priority = 1.f;
	VkDeviceQueueCreateInfo queue{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
	queue.queueFamilyIndex = graphics_family;
	queue.queueCount = 1;
	queue.pQueuePriorities = &priority;
	VkDeviceCreateInfo device_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
	device_info.queueCreateInfoCount = 1;
	device_info.pQueueCreateInfos = &queue;
	VkDevice device = VK_NULL_HANDLE;
	status = vkCreateDevice(nvidia, &device_info, nullptr, &device);
	out << "vkCreateDevice: " << status << '\n';
	if (device) { vkDeviceWaitIdle(device); vkDestroyDevice(device, nullptr); }
	vkDestroyInstance(instance, nullptr);
	out << (status == VK_SUCCESS ? "PASS" : "FAIL") << ": Vulkan loader/device smoke only; no neural frames or game were evaluated.\n";
	return status == VK_SUCCESS ? 0 : 12;
}
