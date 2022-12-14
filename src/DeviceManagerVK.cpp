/*
* Copyright (c) 2014-2021, NVIDIA CORPORATION. All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

// Adapted from Donut's DeviceManagerVK
// https://github.com/NVIDIAGameWorks/donut/blob/main/src/app/vulkan/DeviceManager_VK.cpp

#include <string>
#include <queue>
#include <unordered_set>

#include "elegy-rhi/DeviceManager.hpp"

#include <nvrhi/vulkan.h>
#include <nvrhi/validation.h>

using namespace nvrhi::app;

const char* va( const char* message, ... )
{
	static char buffer[4096];
	va_list arguments;

	va_start( arguments, message );
	vsprintf( buffer, message, arguments );
	va_end( arguments );

	return buffer;
}

// Define the Vulkan dynamic dispatcher - this needs to occur in exactly one cpp file in the program.
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

class DeviceManager_VK : public DeviceManager
{
public:
	[[nodiscard]] nvrhi::IDevice* GetDevice() const override
	{
		if ( m_ValidationLayer )
			return m_ValidationLayer;

		return m_NvrhiDevice;
	}

	[[nodiscard]] nvrhi::GraphicsAPI GetGraphicsAPI() const override
	{
		return nvrhi::GraphicsAPI::VULKAN;
	}

protected:
	bool CreateDeviceAndSwapChain() override;
	void DestroyDeviceAndSwapChain() override;

	void ResizeSwapChain() override
	{
		if ( m_VulkanDevice )
		{
			destroySwapChain();
			createSwapChain();
		}
	}

	nvrhi::ITexture* GetCurrentBackBuffer() override
	{
		return m_SwapChainImages[m_SwapChainIndex].rhiHandle;
	}
	nvrhi::ITexture* GetBackBuffer( uint32_t index ) override
	{
		if ( index < m_SwapChainImages.size() )
			return m_SwapChainImages[index].rhiHandle;
		return nullptr;
	}
	uint32_t GetCurrentBackBufferIndex() override
	{
		return m_SwapChainIndex;
	}
	uint32_t GetBackBufferCount() override
	{
		return uint32_t( m_SwapChainImages.size() );
	}

	void BeginFrame() override;
	void Present() override;

	const char* GetRendererString() const override
	{
		return m_RendererString.c_str();
	}

	bool IsVulkanInstanceExtensionEnabled( const char* extensionName ) const override
	{
		return enabledExtensions.instance.find( extensionName ) != enabledExtensions.instance.end();
	}

	bool IsVulkanDeviceExtensionEnabled( const char* extensionName ) const override
	{
		return enabledExtensions.device.find( extensionName ) != enabledExtensions.device.end();
	}

	bool IsVulkanLayerEnabled( const char* layerName ) const override
	{
		return enabledExtensions.layers.find( layerName ) != enabledExtensions.layers.end();
	}

	void GetEnabledVulkanInstanceExtensions( std::vector<std::string>& extensions ) const override
	{
		for ( const auto& ext : enabledExtensions.instance )
			extensions.push_back( ext );
	}

	void GetEnabledVulkanDeviceExtensions( std::vector<std::string>& extensions ) const override
	{
		for ( const auto& ext : enabledExtensions.device )
			extensions.push_back( ext );
	}

	void GetEnabledVulkanLayers( std::vector<std::string>& layers ) const override
	{
		for ( const auto& ext : enabledExtensions.layers )
			layers.push_back( ext );
	}

private:
	bool createInstance();
	bool createWindowSurface();
	void installDebugCallback();
	bool pickPhysicalDevice();
	bool findQueueFamilies( vk::PhysicalDevice physicalDevice );
	bool createDevice();
	bool createSwapChain();
	void destroySwapChain();
	bool getPhysicalDevicePresentationSupport( vk::PhysicalDevice& physicalDevice, int queueFamilyIndex );

	struct VulkanExtensionSet
	{
		std::unordered_set<std::string> instance;
		std::unordered_set<std::string> layers;
		std::unordered_set<std::string> device;
	};

	// minimal set of required extensions
	VulkanExtensionSet enabledExtensions = {
		// instance
		{
			VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
		},
		// layers
		{ },
		// device
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_MAINTENANCE1_EXTENSION_NAME
		},
	};

	// optional extensions
	VulkanExtensionSet optionalExtensions = {
		// instance
		{
			VK_EXT_SAMPLER_FILTER_MINMAX_EXTENSION_NAME,
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME
		},
		// layers
		{ },
		// device
		{
			VK_EXT_DEBUG_MARKER_EXTENSION_NAME,
			VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
			VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
			VK_NV_MESH_SHADER_EXTENSION_NAME,
			VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME
		},
	};

	std::unordered_set<std::string> m_RayTracingExtensions = {
		VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
		VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
		VK_KHR_RAY_QUERY_EXTENSION_NAME,
		VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME
	};

	std::string m_RendererString;

	vk::Instance m_VulkanInstance;
	vk::DebugReportCallbackEXT m_DebugReportCallback;

	vk::PhysicalDevice m_VulkanPhysicalDevice;
	int m_GraphicsQueueFamily = -1;
	int m_ComputeQueueFamily = -1;
	int m_TransferQueueFamily = -1;
	int m_PresentQueueFamily = -1;

	vk::Device m_VulkanDevice;
	vk::Queue m_GraphicsQueue;
	vk::Queue m_ComputeQueue;
	vk::Queue m_TransferQueue;
	vk::Queue m_PresentQueue;

	vk::SurfaceKHR m_WindowSurface;

	vk::SurfaceFormatKHR m_SwapChainFormat;
	vk::SwapchainKHR m_SwapChain;

	struct SwapChainImage
	{
		vk::Image image;
		nvrhi::TextureHandle rhiHandle;
	};

	std::vector<SwapChainImage> m_SwapChainImages;
	uint32_t m_SwapChainIndex = uint32_t( -1 );

	nvrhi::vulkan::DeviceHandle m_NvrhiDevice;
	nvrhi::DeviceHandle m_ValidationLayer;

	nvrhi::CommandListHandle m_BarrierCommandList;
	vk::Semaphore m_PresentSemaphore;

	std::queue<nvrhi::EventQueryHandle> m_FramesInFlight;
	std::vector<nvrhi::EventQueryHandle> m_QueryPool;

private:
	static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objType,
		uint64_t obj,
		size_t location,
		int32_t code,
		const char* layerPrefix,
		const char* msg,
		void* userData )
	{
		const DeviceManager_VK* manager = (const DeviceManager_VK*)userData;

		if ( manager )
		{
			// Vulkan stuff is super wordy
			const_cast<DeviceManager_VK*>( manager )->Message( 
				va( "[Vulkan: location=0x%zx code=%d, layerPrefix='%s'] %s", location, code, layerPrefix, msg ), 
				nvrhi::MessageSeverity::Warning );
			
			const auto& ignored = manager->m_DeviceParams.ignoredVulkanValidationMessageLocations;
			const auto found = std::find( ignored.begin(), ignored.end(), location );
			if ( found != ignored.end() )
				return VK_FALSE;
		}


		return VK_FALSE;
	}

};

static std::vector<const char*> stringSetToVector( const std::unordered_set<std::string>& set )
{
	std::vector<const char*> ret;
	for ( const auto& s : set )
	{
		ret.push_back( s.c_str() );
	}

	return ret;
}

template <typename T>
static std::vector<T> setToVector( const std::unordered_set<T>& set )
{
	std::vector<T> ret;
	for ( const auto& s : set )
	{
		ret.push_back( s );
	}

	return ret;
}

bool DeviceManager_VK::createInstance()
{
	// add instance extensions requested by the user
	for ( const std::string& name : m_DeviceParams.requiredVulkanInstanceExtensions )
	{
		enabledExtensions.instance.insert( name );
	}
	for ( const std::string& name : m_DeviceParams.optionalVulkanInstanceExtensions )
	{
		optionalExtensions.instance.insert( name );
	}

	// add layers requested by the user
	for ( const std::string& name : m_DeviceParams.requiredVulkanLayers )
	{
		enabledExtensions.layers.insert( name );
	}
	for ( const std::string& name : m_DeviceParams.optionalVulkanLayers )
	{
		optionalExtensions.layers.insert( name );
	}

	std::unordered_set<std::string> requiredExtensions = enabledExtensions.instance;

	// figure out which optional extensions are supported
	for ( const auto& instanceExt : vk::enumerateInstanceExtensionProperties() )
	{
		const std::string name = instanceExt.extensionName;
		if ( optionalExtensions.instance.find( name ) != optionalExtensions.instance.end() )
		{
			enabledExtensions.instance.insert( name );
		}

		requiredExtensions.erase( name );
	}

	if ( !requiredExtensions.empty() )
	{
		std::stringstream ss;
		ss << "Cannot create a Vulkan instance because the following required extension(s) are not supported:";
		for ( const auto& ext : requiredExtensions )
			ss << std::endl << "  - " << ext;

		Error( ss.str().c_str() );
		return false;
	}

	Message( "Enabled Vulkan instance extensions:", m_DeviceParams.infoLogSeverity );
	for ( const auto& ext : enabledExtensions.instance )
	{
		Message( va( "    %s", ext.c_str() ), m_DeviceParams.infoLogSeverity );
	}

	std::unordered_set<std::string> requiredLayers = enabledExtensions.layers;

	for ( const auto& layer : vk::enumerateInstanceLayerProperties() )
	{
		const std::string name = layer.layerName;
		if ( optionalExtensions.layers.find( name ) != optionalExtensions.layers.end() )
		{
			enabledExtensions.layers.insert( name );
		}

		requiredLayers.erase( name );
	}

	if ( !requiredLayers.empty() )
	{
		std::stringstream ss;
		ss << "Cannot create a Vulkan instance because the following required layer(s) are not supported:";
		for ( const auto& ext : requiredLayers )
			ss << std::endl << "  - " << ext;

		Error( ss.str().c_str() );
		return false;
	}

	Message( "Enabled Vulkan layers:", m_DeviceParams.infoLogSeverity );
	for ( const auto& layer : enabledExtensions.layers )
	{
		Message( va( "    %s", layer.c_str() ), m_DeviceParams.infoLogSeverity );
	}

	auto instanceExtVec = stringSetToVector( enabledExtensions.instance );
	auto layerVec = stringSetToVector( enabledExtensions.layers );

	auto applicationInfo = vk::ApplicationInfo()
		.setApiVersion( VK_MAKE_VERSION( 1, 2, 0 ) );

	// create the vulkan instance
	vk::InstanceCreateInfo info = vk::InstanceCreateInfo()
		.setEnabledLayerCount( uint32_t( layerVec.size() ) )
		.setPpEnabledLayerNames( layerVec.data() )
		.setEnabledExtensionCount( uint32_t( instanceExtVec.size() ) )
		.setPpEnabledExtensionNames( instanceExtVec.data() )
		.setPApplicationInfo( &applicationInfo );

	const vk::Result res = vk::createInstance( &info, nullptr, &m_VulkanInstance );
	if ( res != vk::Result::eSuccess )
	{
		Error( va( "Failed to create a Vulkan instance, error code = %s", nvrhi::vulkan::resultToString( res ) ) );
		return false;
	}

	VULKAN_HPP_DEFAULT_DISPATCHER.init( m_VulkanInstance );

	return true;
}

void DeviceManager_VK::installDebugCallback()
{
	auto info = vk::DebugReportCallbackCreateInfoEXT()
		.setFlags( vk::DebugReportFlagBitsEXT::eError |
		vk::DebugReportFlagBitsEXT::eWarning |
		//   vk::DebugReportFlagBitsEXT::eInformation |
		vk::DebugReportFlagBitsEXT::ePerformanceWarning )
		.setPfnCallback( vulkanDebugCallback )
		.setPUserData( this );

	vk::Result res = m_VulkanInstance.createDebugReportCallbackEXT( &info, nullptr, &m_DebugReportCallback );
	assert( res == vk::Result::eSuccess );
}

bool DeviceManager_VK::pickPhysicalDevice()
{
	vk::Format requestedFormat = nvrhi::vulkan::convertFormat( m_DeviceParams.swapChainFormat );
	vk::Extent2D requestedExtent( m_DeviceParams.backBufferWidth, m_DeviceParams.backBufferHeight );

	auto devices = m_VulkanInstance.enumeratePhysicalDevices();

	// Start building an error message in case we cannot find a device.
	std::stringstream errorStream;
	errorStream << "Cannot find a Vulkan device that supports all the required extensions and properties.";

	// build a list of GPUs
	std::vector<vk::PhysicalDevice> discreteGPUs;
	std::vector<vk::PhysicalDevice> otherGPUs;
	for ( const auto& dev : devices )
	{
		auto prop = dev.getProperties();

		errorStream << std::endl << prop.deviceName.data() << ":";

		// check that all required device extensions are present
		std::unordered_set<std::string> requiredExtensions = enabledExtensions.device;
		auto deviceExtensions = dev.enumerateDeviceExtensionProperties();
		for ( const auto& ext : deviceExtensions )
		{
			requiredExtensions.erase( std::string( ext.extensionName.data() ) );
		}

		bool deviceIsGood = true;

		if ( !requiredExtensions.empty() )
		{
			// device is missing one or more required extensions
			for ( const auto& ext : requiredExtensions )
			{
				errorStream << std::endl << "  - missing " << ext;
			}
			deviceIsGood = false;
		}

		auto deviceFeatures = dev.getFeatures();
		if ( !deviceFeatures.samplerAnisotropy )
		{
			// device is a toaster oven
			errorStream << std::endl << "  - does not support samplerAnisotropy";
			deviceIsGood = false;
		}
		if ( !deviceFeatures.textureCompressionBC )
		{
			errorStream << std::endl << "  - does not support textureCompressionBC";
			deviceIsGood = false;
		}

		// check that this device supports our intended swap chain creation parameters
		auto surfaceCaps = dev.getSurfaceCapabilitiesKHR( m_WindowSurface );
		auto surfaceFmts = dev.getSurfaceFormatsKHR( m_WindowSurface );
		auto surfacePModes = dev.getSurfacePresentModesKHR( m_WindowSurface );

		if ( surfaceCaps.minImageCount > m_DeviceParams.swapChainBufferCount ||
			(surfaceCaps.maxImageCount < m_DeviceParams.swapChainBufferCount && surfaceCaps.maxImageCount > 0) )
		{
			errorStream << std::endl << "  - cannot support the requested swap chain image count:";
			errorStream << " requested " << m_DeviceParams.swapChainBufferCount << ", available " << surfaceCaps.minImageCount << " - " << surfaceCaps.maxImageCount;
			deviceIsGood = false;
		}

		if ( surfaceCaps.minImageExtent.width > requestedExtent.width ||
			surfaceCaps.minImageExtent.height > requestedExtent.height ||
			surfaceCaps.maxImageExtent.width < requestedExtent.width ||
			surfaceCaps.maxImageExtent.height < requestedExtent.height )
		{
			errorStream << std::endl << "  - cannot support the requested swap chain size:";
			errorStream << " requested " << requestedExtent.width << "x" << requestedExtent.height << ", ";
			errorStream << " available " << surfaceCaps.minImageExtent.width << "x" << surfaceCaps.minImageExtent.height;
			errorStream << " - " << surfaceCaps.maxImageExtent.width << "x" << surfaceCaps.maxImageExtent.height;
			deviceIsGood = false;
		}

		bool surfaceFormatPresent = false;
		for ( const vk::SurfaceFormatKHR& surfaceFmt : surfaceFmts )
		{
			if ( surfaceFmt.format == requestedFormat )
			{
				surfaceFormatPresent = true;
				break;
			}
		}

		if ( !surfaceFormatPresent )
		{
			// can't create a swap chain using the format requested
			errorStream << std::endl << "  - does not support the requested swap chain format";
			deviceIsGood = false;
		}

		if ( !findQueueFamilies( dev ) )
		{
			// device doesn't have all the queue families we need
			errorStream << std::endl << "  - does not support the necessary queue types";
			deviceIsGood = false;
		}

		// check that we can present from the graphics queue
		uint32_t canPresent = dev.getSurfaceSupportKHR( m_GraphicsQueueFamily, m_WindowSurface );
		if ( !canPresent )
		{
			errorStream << std::endl << "  - cannot present";
			deviceIsGood = false;
		}

		if ( !deviceIsGood )
			continue;

		if ( prop.deviceType == vk::PhysicalDeviceType::eDiscreteGpu )
		{
			discreteGPUs.push_back( dev );
		}
		else {
			otherGPUs.push_back( dev );
		}
	}

	// pick the first discrete GPU if it exists, otherwise the first integrated GPU
	if ( !discreteGPUs.empty() )
	{
		m_VulkanPhysicalDevice = discreteGPUs[0];
		return true;
	}

	if ( !otherGPUs.empty() )
	{
		m_VulkanPhysicalDevice = otherGPUs[0];
		return true;
	}

	Error( errorStream.str().c_str() );

	return false;
}

bool DeviceManager_VK::findQueueFamilies( vk::PhysicalDevice physicalDevice )
{
	auto props = physicalDevice.getQueueFamilyProperties();

	Message( va( "Physical device has %i queue families", int( props.size() ) ) );

	for ( int i = 0; i < int( props.size() ); i++ )
	{
		const auto& queueFamily = props[i];

		if ( m_GraphicsQueueFamily == -1 )
		{
			if ( queueFamily.queueCount > 0 &&
				(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) )
			{
				m_GraphicsQueueFamily = i;
			}
		}

		if ( m_ComputeQueueFamily == -1 )
		{
			if ( queueFamily.queueCount > 0 &&
				(queueFamily.queueFlags & vk::QueueFlagBits::eCompute) &&
				!(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) )
			{
				m_ComputeQueueFamily = i;
			}
		}

		if ( m_TransferQueueFamily == -1 )
		{
			if ( queueFamily.queueCount > 0 &&
				(queueFamily.queueFlags & vk::QueueFlagBits::eTransfer) &&
				!(queueFamily.queueFlags & vk::QueueFlagBits::eCompute) &&
				!(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) )
			{
				m_TransferQueueFamily = i;
			}
		}

		if ( m_PresentQueueFamily == -1 )
		{
			if ( queueFamily.queueCount > 0 &&
				getPhysicalDevicePresentationSupport( physicalDevice, i ) )
			{
				m_PresentQueueFamily = i;
			}
		}
	}

	if ( m_GraphicsQueueFamily == -1 ||
		m_PresentQueueFamily == -1 ||
		(m_ComputeQueueFamily == -1 && m_DeviceParams.enableComputeQueue) ||
		(m_TransferQueueFamily == -1 && m_DeviceParams.enableCopyQueue) )
	{
		return false;
	}

	return true;
}

bool DeviceManager_VK::createDevice()
{
	// figure out which optional extensions are supported
	auto deviceExtensions = m_VulkanPhysicalDevice.enumerateDeviceExtensionProperties();
	for ( const auto& ext : deviceExtensions )
	{
		const std::string name = ext.extensionName;
		if ( optionalExtensions.device.find( name ) != optionalExtensions.device.end() )
		{
			enabledExtensions.device.insert( name );
		}

		if ( m_DeviceParams.enableRayTracingExtensions && m_RayTracingExtensions.find( name ) != m_RayTracingExtensions.end() )
		{
			enabledExtensions.device.insert( name );
		}
	}

	bool accelStructSupported = false;
	bool bufferAddressSupported = false;
	bool rayPipelineSupported = false;
	bool rayQuerySupported = false;
	bool meshletsSupported = false;
	bool vrsSupported = false;

	Message( "Enabled Vulkan device extensions:", m_DeviceParams.infoLogSeverity );
	for ( const auto& ext : enabledExtensions.device )
	{
		Message( va( "    %s", ext.c_str() ), m_DeviceParams.infoLogSeverity );

		if ( ext == VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME )
			accelStructSupported = true;
		else if ( ext == VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME )
			bufferAddressSupported = true;
		else if ( ext == VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME )
			rayPipelineSupported = true;
		else if ( ext == VK_KHR_RAY_QUERY_EXTENSION_NAME )
			rayQuerySupported = true;
		else if ( ext == VK_NV_MESH_SHADER_EXTENSION_NAME )
			meshletsSupported = true;
		else if ( ext == VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME )
			vrsSupported = true;
	}

	std::unordered_set<int> uniqueQueueFamilies = {
		m_GraphicsQueueFamily,
		m_PresentQueueFamily };

	if ( m_DeviceParams.enableComputeQueue )
		uniqueQueueFamilies.insert( m_ComputeQueueFamily );

	if ( m_DeviceParams.enableCopyQueue )
		uniqueQueueFamilies.insert( m_TransferQueueFamily );

	float priority = 1.f;
	std::vector<vk::DeviceQueueCreateInfo> queueDesc;
	for ( int queueFamily : uniqueQueueFamilies )
	{
		queueDesc.push_back( vk::DeviceQueueCreateInfo()
			.setQueueFamilyIndex( queueFamily )
			.setQueueCount( 1 )
			.setPQueuePriorities( &priority ) );
	}

	auto accelStructFeatures = vk::PhysicalDeviceAccelerationStructureFeaturesKHR()
		.setAccelerationStructure( true );
	auto bufferAddressFeatures = vk::PhysicalDeviceBufferAddressFeaturesEXT()
		.setBufferDeviceAddress( true );
	auto rayPipelineFeatures = vk::PhysicalDeviceRayTracingPipelineFeaturesKHR()
		.setRayTracingPipeline( true )
		.setRayTraversalPrimitiveCulling( true );
	auto rayQueryFeatures = vk::PhysicalDeviceRayQueryFeaturesKHR()
		.setRayQuery( true );
	auto meshletFeatures = vk::PhysicalDeviceMeshShaderFeaturesNV()
		.setTaskShader( true )
		.setMeshShader( true );
	auto vrsFeatures = vk::PhysicalDeviceFragmentShadingRateFeaturesKHR()
		.setPipelineFragmentShadingRate( true )
		.setPrimitiveFragmentShadingRate( true )
		.setAttachmentFragmentShadingRate( true );

	void* pNext = nullptr;
#define APPEND_EXTENSION(condition, desc) if (condition) { (desc).pNext = pNext; pNext = &(desc); }  // NOLINT(cppcoreguidelines-macro-usage)
	APPEND_EXTENSION( accelStructSupported, accelStructFeatures )
		APPEND_EXTENSION( bufferAddressSupported, bufferAddressFeatures )
		APPEND_EXTENSION( rayPipelineSupported, rayPipelineFeatures )
		APPEND_EXTENSION( rayQuerySupported, rayQueryFeatures )
		APPEND_EXTENSION( meshletsSupported, meshletFeatures )
		APPEND_EXTENSION( vrsSupported, vrsFeatures )
#undef APPEND_EXTENSION

		auto deviceFeatures = vk::PhysicalDeviceFeatures()
		.setShaderImageGatherExtended( true )
		.setSamplerAnisotropy( true )
		.setTessellationShader( true )
		.setTextureCompressionBC( true )
		.setGeometryShader( true )
		.setImageCubeArray( true )
		.setDualSrcBlend( true );

	auto vulkan12features = vk::PhysicalDeviceVulkan12Features()
		.setDescriptorIndexing( true )
		.setRuntimeDescriptorArray( true )
		.setDescriptorBindingPartiallyBound( true )
		.setDescriptorBindingVariableDescriptorCount( true )
		.setTimelineSemaphore( true )
		.setShaderSampledImageArrayNonUniformIndexing( true )
		.setPNext( pNext );

	auto layerVec = stringSetToVector( enabledExtensions.layers );
	auto extVec = stringSetToVector( enabledExtensions.device );

	auto deviceDesc = vk::DeviceCreateInfo()
		.setPQueueCreateInfos( queueDesc.data() )
		.setQueueCreateInfoCount( uint32_t( queueDesc.size() ) )
		.setPEnabledFeatures( &deviceFeatures )
		.setEnabledExtensionCount( uint32_t( extVec.size() ) )
		.setPpEnabledExtensionNames( extVec.data() )
		.setEnabledLayerCount( uint32_t( layerVec.size() ) )
		.setPpEnabledLayerNames( layerVec.data() )
		.setPNext( &vulkan12features );

	//if ( m_DeviceParams.deviceCreateInfoCallback )
	//	m_DeviceParams.deviceCreateInfoCallback( deviceDesc );

	const vk::Result res = m_VulkanPhysicalDevice.createDevice( &deviceDesc, nullptr, &m_VulkanDevice );
	if ( res != vk::Result::eSuccess )
	{
		Error( va( "Failed to create a Vulkan physical device, error code = %s", nvrhi::vulkan::resultToString( res ) ) );
		return false;
	}

	m_VulkanDevice.getQueue( m_GraphicsQueueFamily, 0, &m_GraphicsQueue );
	if ( m_DeviceParams.enableComputeQueue )
		m_VulkanDevice.getQueue( m_ComputeQueueFamily, 0, &m_ComputeQueue );
	if ( m_DeviceParams.enableCopyQueue )
		m_VulkanDevice.getQueue( m_TransferQueueFamily, 0, &m_TransferQueue );
	m_VulkanDevice.getQueue( m_PresentQueueFamily, 0, &m_PresentQueue );

	VULKAN_HPP_DEFAULT_DISPATCHER.init( m_VulkanDevice );

	// stash the renderer string
	auto prop = m_VulkanPhysicalDevice.getProperties();
	m_RendererString = std::string( prop.deviceName.data() );

	Message( va( "Created Vulkan device: %s", m_RendererString.c_str() ), m_DeviceParams.infoLogSeverity );

	return true;
}

bool DeviceManager_VK::createWindowSurface()
{
	vk::Result res = vk::Result::eErrorUnknown;

#if VK_USE_PLATFORM_WIN32_KHR
	const auto& window = m_DeviceParams.windowSurfaceData;
	const auto surfaceCreateInfo = vk::Win32SurfaceCreateInfoKHR{ {}, window.hInstance, window.hWindow };

	res = m_VulkanInstance.createWin32SurfaceKHR( &surfaceCreateInfo, nullptr, &m_WindowSurface );
#elif VK_USE_PLATFORM_XLIB_KHR
	const auto& window = m_DeviceParams.windowSurfaceData;
	const auto surfaceCreateInfo = vk::XlibSurfaceCreateInfoKHR{ {}, window.display, window.window };

	res = m_VulkanInstance.createXlibSurfaceKHR( &surfaceCreateInfo, nullptr, &m_WindowSurface );
#elif VK_USE_PLATFORM_WAYLAND_KHR
#error "Wayland is not yet supported"
#endif

	if ( res != vk::Result::eSuccess )
	{
		Error( va( "Failed to create a window surface, error code = %s", nvrhi::vulkan::resultToString( res ) ) );
		return false;
	}

	return true;
}

bool DeviceManager_VK::getPhysicalDevicePresentationSupport( vk::PhysicalDevice& physicalDevice, int queueFamilyIndex )
{
#if VK_USE_PLATFORM_WIN32_KHR
	return physicalDevice.getWin32PresentationSupportKHR( queueFamilyIndex );
#elif VK_USE_PLATFORM_XLIB_KHR
	const auto& window = m_DeviceParams.windowSurfaceData;

	// Screen is hardcoded to 0 here, investigate later on to support multiple monitors maybe???
	VisualID visualId = XVisualIDFromVisual( DefaultVisual( window.display, 0 ) );
	return physicalDevice.getXlibPresentationSupportKHR( queueFamilyIndex, window.display, visualId );
#else
	return false;
#endif
}

void DeviceManager_VK::destroySwapChain()
{
	if ( m_VulkanDevice )
	{
		m_VulkanDevice.waitIdle();
	}

	if ( m_SwapChain )
	{
		m_VulkanDevice.destroySwapchainKHR( m_SwapChain );
		m_SwapChain = nullptr;
	}

	m_SwapChainImages.clear();
}

bool DeviceManager_VK::createSwapChain()
{
	destroySwapChain();

	m_SwapChainFormat = {
		vk::Format( nvrhi::vulkan::convertFormat( m_DeviceParams.swapChainFormat ) ),
		vk::ColorSpaceKHR::eSrgbNonlinear
	};

	vk::Extent2D extent = vk::Extent2D( m_DeviceParams.backBufferWidth, m_DeviceParams.backBufferHeight );

	std::unordered_set<uint32_t> uniqueQueues = {
		uint32_t( m_GraphicsQueueFamily ),
		uint32_t( m_PresentQueueFamily ) };

	std::vector<uint32_t> queues = setToVector( uniqueQueues );

	const bool enableSwapChainSharing = queues.size() > 1;

	auto desc = vk::SwapchainCreateInfoKHR()
		.setSurface( m_WindowSurface )
		.setMinImageCount( m_DeviceParams.swapChainBufferCount )
		.setImageFormat( m_SwapChainFormat.format )
		.setImageColorSpace( m_SwapChainFormat.colorSpace )
		.setImageExtent( extent )
		.setImageArrayLayers( 1 )
		.setImageUsage( vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled )
		.setImageSharingMode( enableSwapChainSharing ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive )
		.setQueueFamilyIndexCount( enableSwapChainSharing ? uint32_t( queues.size() ) : 0 )
		.setPQueueFamilyIndices( enableSwapChainSharing ? queues.data() : nullptr )
		.setPreTransform( vk::SurfaceTransformFlagBitsKHR::eIdentity )
		.setCompositeAlpha( vk::CompositeAlphaFlagBitsKHR::eOpaque )
		.setPresentMode( m_DeviceParams.vsyncEnabled ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eImmediate )
		.setClipped( true )
		.setOldSwapchain( nullptr );

	const vk::Result res = m_VulkanDevice.createSwapchainKHR( &desc, nullptr, &m_SwapChain );
	if ( res != vk::Result::eSuccess )
	{
		Error( va( "Failed to create a Vulkan swap chain, error code = %s", nvrhi::vulkan::resultToString( res ) ) );
		return false;
	}

	// retrieve swap chain images
	auto images = m_VulkanDevice.getSwapchainImagesKHR( m_SwapChain );
	for ( auto image : images )
	{
		SwapChainImage sci;
		sci.image = image;

		nvrhi::TextureDesc textureDesc;
		textureDesc.width = m_DeviceParams.backBufferWidth;
		textureDesc.height = m_DeviceParams.backBufferHeight;
		textureDesc.format = m_DeviceParams.swapChainFormat;
		textureDesc.debugName = "Swap chain image";
		textureDesc.initialState = nvrhi::ResourceStates::Present;
		textureDesc.keepInitialState = true;
		textureDesc.isRenderTarget = true;

		sci.rhiHandle = m_NvrhiDevice->createHandleForNativeTexture( nvrhi::ObjectTypes::VK_Image, nvrhi::Object( sci.image ), textureDesc );
		m_SwapChainImages.push_back( sci );
	}

	m_SwapChainIndex = 0;

	return true;
}

bool DeviceManager_VK::CreateDeviceAndSwapChain()
{
	if ( m_DeviceParams.enableDebugRuntime )
	{
		enabledExtensions.instance.insert( "VK_EXT_debug_report" );
		enabledExtensions.layers.insert( "VK_LAYER_KHRONOS_validation" );
	}

	const vk::DynamicLoader dl;
	const PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr =   // NOLINT(misc-misplaced-const)
		dl.getProcAddress<PFN_vkGetInstanceProcAddr>( "vkGetInstanceProcAddr" );
	VULKAN_HPP_DEFAULT_DISPATCHER.init( vkGetInstanceProcAddr );

#define CHECK(a) if (!(a)) { return false; }

	CHECK( createInstance() )

		if ( m_DeviceParams.enableDebugRuntime )
		{
			installDebugCallback();
		}

	if ( m_DeviceParams.swapChainFormat == nvrhi::Format::SRGBA8_UNORM )
		m_DeviceParams.swapChainFormat = nvrhi::Format::SBGRA8_UNORM;
	else if ( m_DeviceParams.swapChainFormat == nvrhi::Format::RGBA8_UNORM )
		m_DeviceParams.swapChainFormat = nvrhi::Format::BGRA8_UNORM;

	// add device extensions requested by the user
	for ( const std::string& name : m_DeviceParams.requiredVulkanDeviceExtensions )
	{
		enabledExtensions.device.insert( name );
	}
	for ( const std::string& name : m_DeviceParams.optionalVulkanDeviceExtensions )
	{
		optionalExtensions.device.insert( name );
	}

	CHECK( createWindowSurface() )
		CHECK( pickPhysicalDevice() )
		CHECK( findQueueFamilies( m_VulkanPhysicalDevice ) )
		CHECK( createDevice() )

		auto vecInstanceExt = stringSetToVector( enabledExtensions.instance );
	auto vecLayers = stringSetToVector( enabledExtensions.layers );
	auto vecDeviceExt = stringSetToVector( enabledExtensions.device );

	nvrhi::vulkan::DeviceDesc deviceDesc;
	deviceDesc.errorCB = m_DeviceParams.messageCallback;
	deviceDesc.instance = m_VulkanInstance;
	deviceDesc.physicalDevice = m_VulkanPhysicalDevice;
	deviceDesc.device = m_VulkanDevice;
	deviceDesc.graphicsQueue = m_GraphicsQueue;
	deviceDesc.graphicsQueueIndex = m_GraphicsQueueFamily;
	if ( m_DeviceParams.enableComputeQueue )
	{
		deviceDesc.computeQueue = m_ComputeQueue;
		deviceDesc.computeQueueIndex = m_ComputeQueueFamily;
	}
	if ( m_DeviceParams.enableCopyQueue )
	{
		deviceDesc.transferQueue = m_TransferQueue;
		deviceDesc.transferQueueIndex = m_TransferQueueFamily;
	}
	deviceDesc.instanceExtensions = vecInstanceExt.data();
	deviceDesc.numInstanceExtensions = vecInstanceExt.size();
	deviceDesc.deviceExtensions = vecDeviceExt.data();
	deviceDesc.numDeviceExtensions = vecDeviceExt.size();

	m_NvrhiDevice = nvrhi::vulkan::createDevice( deviceDesc );

	if ( m_DeviceParams.enableNvrhiValidationLayer )
	{
		m_ValidationLayer = nvrhi::validation::createValidationLayer( m_NvrhiDevice );
	}

	CHECK( createSwapChain() )

		m_BarrierCommandList = m_NvrhiDevice->createCommandList();

	m_PresentSemaphore = m_VulkanDevice.createSemaphore( vk::SemaphoreCreateInfo() );

#undef CHECK

	return true;
}

void DeviceManager_VK::DestroyDeviceAndSwapChain()
{
	destroySwapChain();

	m_VulkanDevice.destroySemaphore( m_PresentSemaphore );
	m_PresentSemaphore = vk::Semaphore();

	m_BarrierCommandList = nullptr;

	m_NvrhiDevice = nullptr;
	m_ValidationLayer = nullptr;
	m_RendererString.clear();

	if ( m_DebugReportCallback )
	{
		m_VulkanInstance.destroyDebugReportCallbackEXT( m_DebugReportCallback );
	}

	if ( m_VulkanDevice )
	{
		m_VulkanDevice.destroy();
		m_VulkanDevice = nullptr;
	}

	if ( m_WindowSurface )
	{
		assert( m_VulkanInstance );
		m_VulkanInstance.destroySurfaceKHR( m_WindowSurface );
		m_WindowSurface = nullptr;
	}

	if ( m_VulkanInstance )
	{
		m_VulkanInstance.destroy();
		m_VulkanInstance = nullptr;
	}
}

void DeviceManager_VK::BeginFrame()
{
	const vk::Result res = m_VulkanDevice.acquireNextImageKHR( m_SwapChain,
		std::numeric_limits<uint64_t>::max(), // timeout
		m_PresentSemaphore,
		vk::Fence(),
		&m_SwapChainIndex );

	assert( res == vk::Result::eSuccess );

	m_NvrhiDevice->queueWaitForSemaphore( nvrhi::CommandQueue::Graphics, m_PresentSemaphore, 0 );
}

void DeviceManager_VK::Present()
{
	m_NvrhiDevice->queueSignalSemaphore( nvrhi::CommandQueue::Graphics, m_PresentSemaphore, 0 );

	m_BarrierCommandList->open(); // umm...
	m_BarrierCommandList->close();
	m_NvrhiDevice->executeCommandList( m_BarrierCommandList );

	vk::PresentInfoKHR info = vk::PresentInfoKHR()
		.setWaitSemaphoreCount( 1 )
		.setPWaitSemaphores( &m_PresentSemaphore )
		.setSwapchainCount( 1 )
		.setPSwapchains( &m_SwapChain )
		.setPImageIndices( &m_SwapChainIndex );

	const vk::Result res = m_PresentQueue.presentKHR( &info );
	assert( res == vk::Result::eSuccess || res == vk::Result::eErrorOutOfDateKHR );

	if ( m_DeviceParams.enableDebugRuntime )
	{
		// according to vulkan-tutorial.com, "the validation layer implementation expects
		// the application to explicitly synchronize with the GPU"
		m_PresentQueue.waitIdle();
	}
	else
	{
#ifndef _WIN32
		if ( m_DeviceParams.vsyncEnabled )
		{
			m_PresentQueue.waitIdle();
		}
#endif

		while ( m_FramesInFlight.size() > m_DeviceParams.maxFramesInFlight )
		{
			auto query = m_FramesInFlight.front();
			m_FramesInFlight.pop();

			m_NvrhiDevice->waitEventQuery( query );

			m_QueryPool.push_back( query );
		}

		nvrhi::EventQueryHandle query;
		if ( !m_QueryPool.empty() )
		{
			query = m_QueryPool.back();
			m_QueryPool.pop_back();
		}
		else
		{
			query = m_NvrhiDevice->createEventQuery();
		}

		m_NvrhiDevice->resetEventQuery( query );
		m_NvrhiDevice->setEventQuery( query, nvrhi::CommandQueue::Graphics );
		m_FramesInFlight.push( query );
	}
}

DeviceManager* DeviceManager::CreateVK()
{
	return new DeviceManager_VK();
}
