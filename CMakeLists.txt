
## In order to use this library, simply
## add_subdirectory( elegy-rhi )
## And then you will have an ElegyRhi project that you can use like so:
## target_link_libraries( MyProject PRIVATE ElegyRhi )
## target_include_directories( MyProject PRIVATE ElegyRhi )

cmake_minimum_required( VERSION 3.16 )

project( ElegyRhi )

## C++17 for NVRHI
set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

## ELR stands for ElegyRhi
## I have a habit of setting a root variable cuz' I'm lazy to type CMAKE_CURRENT_SOURCE_DIR every time
set( ELR_ROOT ${CMAKE_CURRENT_SOURCE_DIR} )
set_property( GLOBAL PROPERTY USE_FOLDERS ON )

if ( UNIX )
	option( ELR_USE_WAYLAND "Use Wayland instead of X11" 0 )
endif()

## Set up NVRHI

## NVRHI
option( NVRHI_BUILD_SHARED OFF )
option( NVRHI_WITH_NVAPI OFF )
option( NVRHI_WITH_RTXMU OFF )
## NVRHI-SC will be built so you can compile shaders
option( NVRHI_WITH_SHADER_COMPILER ON )
## No DirectX on Linux
if ( UNIX )
	option( NVRHI_WITH_DX11 OFF )
	option( NVRHI_WITH_DX12 OFF )
endif()

if ( NVRHI_WITH_VULKAN )
	find_package( Vulkan REQUIRED )
endif()

add_subdirectory( nvrhi )

if ( NOT (NVRHI_WITH_VULKAN OR NVRHI_WITH_DX11 OR NVRHI_WITH_DX12) )
	message( FATAL_ERROR "No graphics APIs are selected, please select at least one" )
endif()

if ( NVRHI_WITH_DX11 )
	set_target_properties( nvrhi_d3d11 PROPERTIES FOLDER "Libs" )
endif()
if ( NVRHI_WITH_DX12 )
	set_target_properties( nvrhi_d3d12 PROPERTIES FOLDER "Libs" )
endif()
if ( NVRHI_WITH_VULKAN )
	set_target_properties( nvrhi_vk PROPERTIES FOLDER "Libs" )
endif()
set_target_properties( nvrhi PROPERTIES FOLDER "Libs" )

## The sources
set( THE_SOURCES
	src/DeviceManager.cpp
	include/elegy-rhi/DeviceManager.hpp )

if ( NVRHI_WITH_DX11 )
	set( THE_SOURCES
		${THE_SOURCES}
		src/DeviceManagerDX11.cpp )
endif()

if ( NVRHI_WITH_DX12 )
	set( THE_SOURCES
		${THE_SOURCES}
		src/DeviceManagerDX12.cpp )
endif()

if ( NVRHI_WITH_VULKAN )
	set( THE_SOURCES
		${THE_SOURCES}
		src/DeviceManagerVK.cpp )
endif()

## Folder organisation
source_group( TREE ${ELR_ROOT} FILES ${THE_SOURCES} )

## The library
## Should this be a DLL? Most likely not
add_library( ElegyRhi STATIC ${THE_SOURCES} )

## Include dirs
target_include_directories( ElegyRhi PUBLIC
	${ELR_ROOT}/include
	nvrhi/include )

if ( NVRHI_WITH_VULKAN )
	target_include_directories( ElegyRhi PRIVATE ${Vulkan_INCLUDE_DIR} )
endif()

## Link against SDL2 libs
target_link_libraries( ElegyRhi PRIVATE ${SDL2_LIBRARIES} nvrhi )

set( ELR_DEFINES "" )
if ( WIN32 )
	## Vulkan headers include winmindef.h which messes with std::min and std::max
	set( ELR_DEFINES ${ELR_DEFINES} NOMINMAX VK_USE_PLATFORM_WIN32_KHR=1 )
elseif( UNIX )
	if ( ELR_USE_WAYLAND )
		set( ELR_DEFINES ${ELR_DEFINES} VK_USE_PLATFORM_WAYLAND_KHR=1 )
	else()
		set( ELR_DEFINES ${ELR_DEFINES} VK_USE_PLATFORM_XLIB_KHR=1 )
	endif()
endif()

if ( NVRHI_WITH_DX11 )
	set( ELR_DEFINES ${ELR_DEFINES} USE_DX11=1 )
	target_link_libraries( ElegyRhi PRIVATE nvrhi_d3d11 )
endif()
if ( NVRHI_WITH_DX12 )
	set( ELR_DEFINES ${ELR_DEFINES} USE_DX12=1 )
	target_link_libraries( ElegyRhi PRIVATE nvrhi_d3d12 )
endif()
if ( NVRHI_WITH_VULKAN )
	set( ELR_DEFINES ${ELR_DEFINES} USE_VK=1 )
	target_link_libraries( ElegyRhi PRIVATE nvrhi_vk Vulkan::Vulkan )
endif()

target_compile_definitions( nvrhi PRIVATE ${ELR_DEFINES} )
target_compile_definitions( ElegyRhi PRIVATE ${ELR_DEFINES} )

if ( NVRHI_WITH_DX11 )
	target_compile_definitions( nvrhi_d3d11 PRIVATE ${ELR_DEFINES} )
endif()
if ( NVRHI_WITH_DX12 )
	target_compile_definitions( nvrhi_d3d12 PRIVATE ${ELR_DEFINES} )
endif()
if ( NVRHI_WITH_VULKAN )
	target_compile_definitions( nvrhi_vk PRIVATE ${ELR_DEFINES} )
endif()
