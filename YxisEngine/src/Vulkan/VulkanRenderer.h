#pragma once

#include "Device.h"

namespace Yxis::Vulkan
{
	class VulkanRenderer
	{
	public:
		using DevicePtr = std::unique_ptr<Device>;

		static void initialize(const std::string& appName);
		static void destroy();

		static const std::string& getAppName();
		static const VkInstance getInstance();
		static const DevicePtr& getDevice();
	private:
		static std::string m_appName;
		static VkInstance m_instance;
#ifdef YX_DEBUG
		static VkDebugUtilsMessengerEXT m_debugMessenger;
#endif
		static DevicePtr m_device;
	};
}