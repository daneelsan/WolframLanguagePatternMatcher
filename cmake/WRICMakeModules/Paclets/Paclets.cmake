
include(${CMAKE_CURRENT_LIST_DIR}/../WRICommon.cmake)

# os_default_settings must be called before project(...) for iOS build. It
# invokes ExternalProject_Add in ios_build_two_platforms and it sets iOS 
# CMake variables CMAKE_OSX_DEPLOYMENT_TARGET; CMAKE_OSX_SYSROOT,
macro(os_default_settings)

	#this functions sits in ../WRICommon.cmake
	wri_autodetect_systemid()
	message("build for SYSTEMID : ${SYSTEMID}")
	
	if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT OR NOT CMAKE_INSTALL_PREFIX)
		set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE STRING "")
	endif()
	message("CMAKE_INSTALL_PREFIX : ${CMAKE_INSTALL_PREFIX}")

	# If building for iOS, invoking ExternalProject_Add if IOS_INTERNAL_BUILD is not set.
	if(SYSTEMID STREQUAL "iOS" AND NOT IOS_INTERNAL_BUILD)
		ios_build_two_platforms()
	elseif(SYSTEMID STREQUAL "visionOS" AND NOT VISIONOS_INTERNAL_BUILD)
		visionos_build_two_platforms()
	endif()

	#set WRI_LIB_INSTALL_DIR for libraries install location
	if(SYSTEMID STREQUAL "iOS" OR SYSTEMID STREQUAL "visionOS")
		set(WRI_LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}" CACHE STRING "")
		if(DEFINED DEBUG_FILES_INSTALL_PREFIX)
			set(DEBUG_FILES_INSTALL_DIR "${DEBUG_FILES_INSTALL_PREFIX}" CACHE STRING "")
		endif()
	else()
		set(WRI_LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/${PACLET_NAME}/LibraryResources/${SYSTEMID}" CACHE STRING "")
		if(DEFINED DEBUG_FILES_INSTALL_PREFIX)
			set(DEBUG_FILES_INSTALL_DIR "${DEBUG_FILES_INSTALL_PREFIX}/${PACLET_NAME}/LibraryResources/${SYSTEMID}" CACHE STRING "")
		endif()
	endif()
	message("WRI_LIB_INSTALL_DIR : ${WRI_LIB_INSTALL_DIR}")

	if(SYSTEMID STREQUAL "iOS")
		set(CMAKE_OSX_DEPLOYMENT_TARGET 15.0 CACHE STRING "")
		set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Default OSX Architecture.")
		if(BUILD_PLATFORM MATCHES "iossimulator.*")
			set(CMAKE_OSX_SYSROOT /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk CACHE STRING "")
		else()
			set(CMAKE_OSX_SYSROOT /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk CACHE STRING "")
		endif()
	elseif(SYSTEMID STREQUAL "visionOS")
		set(CMAKE_OSX_DEPLOYMENT_TARGET 1.0 CACHE STRING "")
		if(BUILD_PLATFORM MATCHES "visionossimulator.*")
			# IMPORTANT: Building for visionOS requires Xcode 15.2 or later, and a Mac with an M1 or later processor.
			set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Default visionOS Architecture.")
			set(CMAKE_OSX_SYSROOT /Applications/Xcode.app/Contents/Developer/Platforms/XRSimulator.platform/Developer/SDKs/XRSimulator.sdk CACHE STRING "")
		else()
			set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Default visionOS Architecture.")
			set(CMAKE_OSX_SYSROOT /Applications/Xcode.app/Contents/Developer/Platforms/XROS.platform/Developer/SDKs/XROS.sdk CACHE STRING "")
		endif()
	elseif(SYSTEMID MATCHES "MacOSX")
		if(DEFINED ENV{MACOSX_DEPLOYMENT_TARGET})
			set(_macosx_target "$ENV{MACOSX_DEPLOYMENT_TARGET}")
		else()
			set(_macosx_target 11.0)
		endif()
		set(CMAKE_OSX_DEPLOYMENT_TARGET "${_macosx_target}" CACHE STRING "")
		if(SYSTEMID STREQUAL "MacOSX-x86-64")
			set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "Default OSX Architecture.")
		elseif(SYSTEMID STREQUAL "MacOSX-ARM64")
			set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Default OSX Architecture.")
		endif()
	endif()
endmacro()


macro(set_paclet_library_properties target)
	if(SYSTEMID STREQUAL "iOS" OR SYSTEMID STREQUAL "visionOS")
		set_target_properties(${target} PROPERTIES 
			FRAMEWORK TRUE
			MACOSX_FRAMEWORK_INFO_PLIST "${WRICMakeModules_DIR}/Paclets/${SYSTEMID}FrameworkInfo.plist"
			MACOSX_FRAMEWORK_IDENTIFIER com.wolfram.${target}
			MACOSX_FRAMEWORK_BUNDLE_VERSION "1.0"
			MACOSX_FRAMEWORK_SHORT_VERSION_STRING "1.0"
		)
	endif()

	if(SYSTEMID STREQUAL "Linux-ARM")
		target_compile_definitions(${target} PUBLIC MINT_32)
	endif()

	if(WIN32)
		set_target_properties(${target} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
		LINK_FLAGS "/INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF"
		COMPIILE_OPTIONS "/Zi")
		target_sources(${target} PRIVATE "${WRICMakeModules_DIR}/Paclets/WindowsLibraryVersion.rc")
	else()
		set_target_properties(${target} PROPERTIES
		COMPIILE_OPTIONS "-g")
	endif()
endmacro()

# For iOS build, ExternalProject_Add is invoked twice to build for device and 
# simulator versions. Could specify optional variable IOS_EXTRA_CMAKE_ARGS for 
# additional cmake args.
macro(ios_build_two_platforms)

	if(NOT CMAKE_BUILD_TYPE)
		set(CMAKE_BUILD_TYPE "Release" CACHE STRING "")
	endif()
	message("iOS CMAKE_BUILD_TYPE : ${CMAKE_BUILD_TYPE}")

	set(IOS_BUILD_PLATFORM ios9.2-libc++-clang7.0 CACHE STRING "")
	set(IOSSIMULATOR_BUILD_PLATFORM iossimulator-libc++-clang12.0  CACHE STRING "")

	message("IOS_BUILD_PLATFORM : ${IOS_BUILD_PLATFORM}")
	message("IOSSIMULATOR_BUILD_PLATFORM : ${IOSSIMULATOR_BUILD_PLATFORM}")
	if(DEFINED DEBUG_FILES_INSTALL_PREFIX)
		set(DEBUG_FILES_INSTALL_DIR "${DEBUG_FILES_INSTALL_PREFIX}" CACHE STRING "")
	endif()
	project(dummy)
	include(ExternalProject)
	ExternalProject_Add(iosbuild
		SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
		CMAKE_ARGS
		-DBUILD_PLATFORM=${IOS_BUILD_PLATFORM}
		-DSYSTEMID=${SYSTEMID}
		-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
		-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}/${CMAKE_BUILD_TYPE}-iphoneos
		-DDEBUG_FILES_INSTALL_DIR=${DEBUG_FILES_INSTALL_DIR}/${CMAKE_BUILD_TYPE}-iphoneos
		-DIOS_INTERNAL_BUILD=ON
		-DRE_BUILD=${RE_BUILD}
		${IOS_EXTRA_CMAKE_ARGS}
	)

	ExternalProject_Add(iossimulatorbuild
		SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
		CMAKE_ARGS
		-DBUILD_PLATFORM=${IOSSIMULATOR_BUILD_PLATFORM}
		-DSYSTEMID=${SYSTEMID}
		-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
		-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}/${CMAKE_BUILD_TYPE}-iphonesimulator
		-DDEBUG_FILES_INSTALL_DIR=${DEBUG_FILES_INSTALL_DIR}/${CMAKE_BUILD_TYPE}-iphonesimulator
		-DIOS_INTERNAL_BUILD=ON
		-DRE_BUILD=${RE_BUILD}
		${IOS_EXTRA_CMAKE_ARGS}
	)

	#generate dummy install target
	install(FILES dummy DESTINATION ${CMAKE_INSTALL_PREFIX} OPTIONAL)

	return()
endmacro()

# As above, but for visionOS, which also must invoke ExternalProject_Add() twice.
macro(visionos_build_two_platforms)

	if(NOT CMAKE_BUILD_TYPE)
		set(CMAKE_BUILD_TYPE "Release" CACHE STRING "")
	endif()
	message("visionOS CMAKE_BUILD_TYPE : ${CMAKE_BUILD_TYPE}")

	set(VISIONOS_BUILD_PLATFORM visionos1.0-libc++-clang15.0 CACHE STRING "")
	set(VISIONOSSIMULATOR_BUILD_PLATFORM visionossimulator-libc++-clang15.0  CACHE STRING "")

	message("VISIONOS_BUILD_PLATFORM : ${VISIONOS_BUILD_PLATFORM}")
	message("VISIONOSSIMULATOR_BUILD_PLATFORM : ${VISIONOSSIMULATOR_BUILD_PLATFORM}")
	if(DEFINED DEBUG_FILES_INSTALL_PREFIX)
		set(DEBUG_FILES_INSTALL_DIR "${DEBUG_FILES_INSTALL_PREFIX}" CACHE STRING "")
	endif()
	project(dummy)
	include(ExternalProject)
	ExternalProject_Add(visionosbuild
		SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
		CMAKE_ARGS
		-DBUILD_PLATFORM=${VISIONOS_BUILD_PLATFORM}
		-DSYSTEMID=${SYSTEMID}
		-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
		-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}/${CMAKE_BUILD_TYPE}-xros
		-DDEBUG_FILES_INSTALL_DIR=${DEBUG_FILES_INSTALL_DIR}/${CMAKE_BUILD_TYPE}-xros
		-DVISIONOS_INTERNAL_BUILD=ON
		-DRE_BUILD=${RE_BUILD}
		${IOS_EXTRA_CMAKE_ARGS}
	)

	ExternalProject_Add(visionossimulatorbuild
		SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
		CMAKE_ARGS
		-DBUILD_PLATFORM=${VISIONOSSIMULATOR_BUILD_PLATFORM}
		-DSYSTEMID=${SYSTEMID}
		-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
		-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}/${CMAKE_BUILD_TYPE}-xrsimulator
		-DDEBUG_FILES_INSTALL_DIR=${DEBUG_FILES_INSTALL_DIR}/${CMAKE_BUILD_TYPE}-xrsimulator
		-DVISIONOS_INTERNAL_BUILD=ON
		-DRE_BUILD=${RE_BUILD}
		${IOS_EXTRA_CMAKE_ARGS}
	)

	#generate dummy install target
	install(FILES dummy DESTINATION ${CMAKE_INSTALL_PREFIX} OPTIONAL)

	return()
endmacro()
