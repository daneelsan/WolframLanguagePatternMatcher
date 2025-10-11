include_guard()


# hack to get TEAMCITY_SYSTEMID which is mangled ${SYSTEMID} in artifacts path
# assuming SYSTEMID is defined.
function(init_TEAMCITY_SYSTEMID)
	if(DEFINED TEAMCITY_SYSTEMID)
		return()
	endif()
	if(NOT SYSTEMID)
		message(FATAL_ERROR "This is developer build, please specify SYSTEMID to automatically download artifacts from Teamcity")
	endif()

	if(SYSTEMID STREQUAL "MacOSX-ARM64")
		set(TEAMCITY_SYSTEMID "MacOSXArm64"  CACHE STRING "")
	elseif(SYSTEMID STREQUAL "MacOSX-x86-64")
		set(TEAMCITY_SYSTEMID "MacOSXX8664"  CACHE STRING "")
	elseif(SYSTEMID STREQUAL "Windows-x86-64")
		set(TEAMCITY_SYSTEMID "WindowsX8664"  CACHE STRING "")
	elseif(SYSTEMID STREQUAL "Linux-x86-64")
		set(TEAMCITY_SYSTEMID "LinuxX8664"  CACHE STRING "")
	elseif(SYSTEMID STREQUAL "Linux-ARM64")
		set(TEAMCITY_SYSTEMID "LinuxArm64"  CACHE STRING "")
	elseif(SYSTEMID STREQUAL "Linux-ARM")
		set(TEAMCITY_SYSTEMID "LinuxArm"  CACHE STRING "")
	elseif(SYSTEMID STREQUAL "iOS")
		set(TEAMCITY_SYSTEMID "IOS"  CACHE STRING "")
	elseif(SYSTEMID STREQUAL "visionOS")
		set(TEAMCITY_SYSTEMID "VisionOS"  CACHE STRING "")
	else()
		#Android Windows Linux
		set(TEAMCITY_SYSTEMID ${SYSTEMID}  CACHE STRING "")
	endif()
	message("TEAMCITY_SYSTEMID : ${TEAMCITY_SYSTEMID}")
endfunction(init_TEAMCITY_SYSTEMID)


# Download a a tar or zip from URL and unzip the result to LOCATION.  The file is
# stored in LOCAL_FILE_NAME during execution.  It is deleted on a successful unzip
# but left for inspection on failure.  If the optional 4th argument is given, the
# download / unzip status is stored in it (0 on succeed, ${code};${text on failure).
# If the 4th argument is _not_ given, a FATAL_ERROR is generated when download or
# unzip encounter an error 
function(wri_download_and_unzip URL LOCATION LOCAL_FILE_NAME)
	set(WRI_TLS_VERIFY "YES")
	if (URL MATCHES "nexus.wolfram.com" OR URL MATCHES "buildmonitor.wolfram.com" OR URL MATCHES "teamcity.wolfram.com")
		set(WRI_TLS_VERIFY "NO")
	endif()
	message("downloading ${URL} to ${LOCAL_FILE_NAME}")
	file(DOWNLOAD "${URL}" ${LOCAL_FILE_NAME} STATUS DOWNLOAD_STATUS TLS_VERIFY ${WRI_TLS_VERIFY})
	if(DOWNLOAD_STATUS AND NOT DOWNLOAD_STATUS EQUAL 0)
		if(ARGC LESS 4)
			message(FATAL_ERROR "Download FAILED with status ${DOWNLOAD_STATUS}")
		else()
			set(${ARGV3} ${DOWNLOAD_STATUS} PARENT_SCOPE)
			return()
		endif()
	endif()

	message("unziping ${LOCAL_FILE_NAME}")
	execute_process(
		COMMAND ${CMAKE_COMMAND} -E tar -xf ${LOCAL_FILE_NAME}
		WORKING_DIRECTORY ${LOCATION}
		RESULT_VARIABLE UNZIP_STATUS
	)
	if(UNZIP_STATUS AND NOT UNZIP_STATUS EQUAL 0)
		if(ARGC LESS 4)
			message(FATAL_ERROR "Unzip FAILED with status ${UNZIP_STATUS}")
		else()
			set(${ARGV3} ${UNZIP_STATUS} PARENT_SCOPE)
		endif()
	endif()

	message(STATUS "Download and unzip SUCCEEDED.")
	if(ARGC GREATER 3)
		set(${ARGV3} 0 PARENT_SCOPE)
	endif()
	file(REMOVE ${LOCAL_FILE_NAME})
endfunction()


# A small helper to convert normal branches into URL-encoded names that are 
# pleasing to Teamcity.  I'm not sure it is useful outside the functions in
# this file that use it.
# For master and release branches, get build from buildmonitor.
function(get_URL_BRANCH_NAME FROM_BUILD_MONITOR URL_BRANCH_NAME BUILD_TYPE BRANCH_NAME)

	if(${BRANCH_NAME} STREQUAL "dev/PrototypeStaging/auto")
		set(FROM_BUILD_MONITOR_TMP ON)
		set(URL_BRANCH_NAME_TMP "")
		set(BUILD_TYPE_TMP "build_type:prototype-build")
	elseif(${BRANCH_NAME} STREQUAL "master")
		set(FROM_BUILD_MONITOR_TMP ON)
		set(URL_BRANCH_NAME_TMP "")
		set(BUILD_TYPE_TMP "build_type:null")
	elseif(${BRANCH_NAME} MATCHES "^release/.*")
		set(FROM_BUILD_MONITOR_TMP ON)
		string(REPLACE "_" "." URL_BRANCH_NAME_TMP ${BRANCH_NAME})
		string(REPLACE "release/B" "\\\;version:" URL_BRANCH_NAME_TMP ${URL_BRANCH_NAME_TMP})
		set(BUILD_TYPE_TMP "build_type:null")
	else()
		set(FROM_BUILD_MONITOR_TMP OFF)
		string(REPLACE "/" "%2F" URL_BRANCH_NAME_TMP ${BRANCH_NAME})
		set(BUILD_TYPE_TMP "")
	endif()

	set(${FROM_BUILD_MONITOR} ${FROM_BUILD_MONITOR_TMP} PARENT_SCOPE)
	set(${URL_BRANCH_NAME} ${URL_BRANCH_NAME_TMP} PARENT_SCOPE)
	set(${BUILD_TYPE} ${BUILD_TYPE_TMP} PARENT_SCOPE)

endfunction()

#download and unzip latest teamcity ${BRANCH_NAME} mathlink artifacts to ${LOCATION}
#the BRANCH_NAME input form: "release/B12_3_1" or "master".  If optional 3rd argument
#is given, store the result of the download and unzip in it.
function(download_MathLink LOCATION BRANCH_NAME)
	init_TEAMCITY_SYSTEMID()
	if(WIN32)
		set(url_file_extension ".zip")
	else()
		set(url_file_extension ".tar.gz")
	endif()

	get_URL_BRANCH_NAME(FROM_BUILD_MONITOR URL_BRANCH_NAME BUILD_TYPE ${BRANCH_NAME})

	if(SYSTEMID STREQUAL "iOS" AND BUILD_PLATFORM MATCHES "iossimulator.*")
		set(Simulator "Simulator")
	elseif(SYSTEMID STREQUAL "visionOS" AND BUILD_PLATFORM MATCHES "visionossimulator.*")
		set(Simulator "Simulator")
	endif()

	if(${FROM_BUILD_MONITOR})
		set(MATHLINK_URL "https://buildmonitor.wolfram.com/api/guestAuth/v3/builds/(build_system:MathLink\;system_id:${SYSTEMID}\;${BUILD_TYPE}\;status:OK\;lock:!Deleted${URL_BRANCH_NAME})/artifacts/(artifact:*_${Simulator}Files${url_file_extension})/content")
	else()
		set(MATHLINK_URL "http://teamcity.wolfram.com/guestAuth/repository/download/WolframLanguage_Core_${TEAMCITY_SYSTEMID}_MathLink_MathLink/.lastSuccessful/WolframLanguage_Core_${TEAMCITY_SYSTEMID}_MathLink_MathLink_${Simulator}Files${url_file_extension}?branch=${URL_BRANCH_NAME}")
	endif()

	set(LOCAL_MATHLINK_ZIP_FILE "${LOCATION}/MathLink.${SYSTEMID}${url_file_extension}")
	wri_download_and_unzip(${MATHLINK_URL} ${LOCATION} ${LOCAL_MATHLINK_ZIP_FILE} ${ARGV2})
	set(${ARGV2} ${${ARGV2}} PARENT_SCOPE)
endfunction(download_MathLink)


#download and unzip latest teamcity ${BRANCH_NAME} Qt6 artifacts to ${LOCATION}
#the BRANCH_NAME input form: "release/B12_3_1" or "master".  If optional 3rd argument
#is given, store the result of the download and unzip in it.
function(download_Qt6 LOCATION BRANCH_NAME)
	init_TEAMCITY_SYSTEMID()
	if(WIN32)
		set(url_file_extension ".zip")
	else()
		set(url_file_extension ".tar.gz")
	endif()

	get_URL_BRANCH_NAME(FROM_BUILDMONITOR URL_BRANCH_NAME BUILD_TYPE ${BRANCH_NAME})

	if(SYSTEMID STREQUAL "iOS" AND BUILD_PLATFORM MATCHES "iossimulator.*")
		set(Simulator "Simulator")
	elseif(SYSTEMID STREQUAL "visionOS" AND BUILD_PLATFORM MATCHES "visionossimulator.*")
		set(Simulator "Simulator")
	endif()

	set(Qt6_URL "http://teamcity.wolfram.com/guestAuth/repository/download/WolframLanguage_Libraries_Qt_NativeLibrary_${TEAMCITY_SYSTEMID}_BuildQt6/.lastSuccessful/WolframLanguage_Libraries_Qt_NativeLibrary_${TEAMCITY_SYSTEMID}_BuildQt6_${Simulator}Files${url_file_extension}?branch=${URL_BRANCH_NAME}")

	set(LOCAL_Qt6_ZIP_FILE "${LOCATION}/Qt6.${SYSTEMID}${url_file_extension}")
	wri_download_and_unzip(${Qt6_URL} ${LOCATION} ${LOCAL_Qt6_ZIP_FILE} ${ARGV2})
	set(${ARGV2} ${${ARGV2}} PARENT_SCOPE)
endfunction(download_Qt6)


#download and unzip latest teamcity ${BRANCH_NAME} WSTP artifacts to ${LOCATION}
#the BRANCH_NAME input form: "release/B12_3_1" or "master".  If optional 3rd argument
#is given, store the result of the download and unzip in it.
function(download_WSTP LOCATION BRANCH_NAME)
	init_TEAMCITY_SYSTEMID()
	if(WIN32)
		set(url_file_extension ".zip")
	else()
		set(url_file_extension ".tar.gz")
	endif()

	get_URL_BRANCH_NAME(FROM_BUILD_MONITOR URL_BRANCH_NAME BUILD_TYPE ${BRANCH_NAME})

	if(SYSTEMID STREQUAL "iOS" AND BUILD_PLATFORM MATCHES "iossimulator.*")
		set(Simulator "Simulator")
	elseif(SYSTEMID STREQUAL "visionOS" AND BUILD_PLATFORM MATCHES "visionossimulator.*")
		set(Simulator "Simulator")
	endif()
	if(${FROM_BUILD_MONITOR})
		set(WSTP_URL "https://buildmonitor.wolfram.com/api/guestAuth/v3/builds/(build_system:Component\;component:WSTP\;system_id:${SYSTEMID}\;${BUILD_TYPE}\;status:OK\;lock:!Deleted${URL_BRANCH_NAME})/artifacts/(artifact:*_${Simulator}Files${url_file_extension})/content")
	else()
		set(WSTP_URL "http://teamcity.wolfram.com/guestAuth/repository/download/WolframLanguage_Core_${TEAMCITY_SYSTEMID}_Wstp_Wstp/.lastSuccessful/WolframLanguage_Core_${TEAMCITY_SYSTEMID}_Wstp_Wstp_${Simulator}Files${url_file_extension}?branch=${URL_BRANCH_NAME}")
	endif()

	set(LOCAL_WSTP_ZIP_FILE "${LOCATION}/WSTP.${SYSTEMID}${url_file_extension}")
	wri_download_and_unzip(${WSTP_URL} ${LOCATION} ${LOCAL_WSTP_ZIP_FILE} ${ARGV2})
	set(${ARGV2} ${${ARGV2}} PARENT_SCOPE)
endfunction(download_WSTP)


#download and unzip latest teamcity ${BRANCH_NAME} RuntimeLibrary artifacts to ${LOCATION}
#the BRANCH_NAME input form: "release/B12_3_1" or "master".  If optional 3rd argument
#is given, store the result of the download and unzip in it.
function(download_RuntimeLibrary LOCATION BRANCH_NAME)
	init_TEAMCITY_SYSTEMID()
	if(SYSTEMID STREQUAL "iOS" AND BUILD_PLATFORM MATCHES "iossimulator.*")
		set(Simulator "Simulator")
	elseif(SYSTEMID STREQUAL "visionOS" AND BUILD_PLATFORM MATCHES "visionossimulator.*")
		set(Simulator "Simulator")
	endif()

	get_URL_BRANCH_NAME(FROM_BUILD_MONITOR URL_BRANCH_NAME BUILD_TYPE ${BRANCH_NAME})

	if(${FROM_BUILD_MONITOR})
		set(RuntimeLibrary_URL "https://buildmonitor.wolfram.com/api/guestAuth/v3/builds/(build_system:Component\;component:RuntimeLibrary\;system_id:${SYSTEMID}\;${BUILD_TYPE}\;status:OK\;lock:!Deleted${URL_BRANCH_NAME})/artifacts/(artifact:*_${Simulator}CompilerAdditions.zip)/content")
	else()
		set(RuntimeLibrary_URL "http://teamcity.wolfram.com/guestAuth/repository/download/WolframLanguage_Core_${TEAMCITY_SYSTEMID}_RuntimeLibrary_RuntimeLibrary${TEAMCITY_SYSTEMID}/.lastSuccessful/WolframLanguage_Core_${TEAMCITY_SYSTEMID}_RuntimeLibrary_RuntimeLibrary${TEAMCITY_SYSTEMID}_${Simulator}CompilerAdditions.zip?branch=${URL_BRANCH_NAME}")
	endif()

	set(LOCAL_RuntimeLibrary_ZIP_FILE "${LOCATION}/RuntimeLibrary.${SYSTEMID}.zip")
	wri_download_and_unzip(${RuntimeLibrary_URL} ${LOCATION} ${LOCAL_RuntimeLibrary_ZIP_FILE} ${ARGV2})
	set(${ARGV2} ${${ARGV2}} PARENT_SCOPE)
endfunction(download_RuntimeLibrary)


#download and unzip latest teamcity ${BRANCH_NAME} TerminalIO artifacts to ${LOCATION}
#the BRANCH_NAME input form: "release/B12_3_1" or "master".  If optional 3rd argument
#is given, store the result of the download and unzip in it.
function(download_TerminalIO LOCATION BRANCH_NAME)
	init_TEAMCITY_SYSTEMID()

	get_URL_BRANCH_NAME(FROM_BUILD_MONITOR URL_BRANCH_NAME BUILD_TYPE ${BRANCH_NAME})

	if(${FROM_BUILD_MONITOR})
		set(TerminalIO_URL "https://buildmonitor.wolfram.com/api/guestAuth/v3/builds/(build_system:Component\;component:TerminalIO\;system_id:${SYSTEMID}\;${BUILD_TYPE}\;status:OK\;lock:!Deleted${URL_BRANCH_NAME})/artifacts/(artifact:*_CompilerAdditions.zip)/content")
	else()
		set(TerminalIO_URL "http://teamcity.wolfram.com/guestAuth/repository/download/WolframLanguage_Core_${TEAMCITY_SYSTEMID}_TerminalIO_TerminalIO/.lastSuccessful/WolframLanguage_Core_${TEAMCITY_SYSTEMID}_TerminalIO_TerminalIO_CompilerAdditions.zip?branch=${URL_BRANCH_NAME}")
	endif()

	set(LOCAL_TerminalIO_ZIP_FILE "${LOCATION}/TerminalIO.${SYSTEMID}.zip")
	wri_download_and_unzip(${TerminalIO_URL} ${LOCATION} ${LOCAL_TerminalIO_ZIP_FILE} ${ARGV2})
	set(${ARGV2} ${${ARGV2}} PARENT_SCOPE)
endfunction(download_TerminalIO)

# Utility macro to set a variable to given value if it currently has no value
macro(set_if_empty VAR VALUE)
	if(NOT ${VAR})
		set(${VAR} ${VALUE})
	endif()
endmacro()

# Utility macro to stop configuration with a message when a required variable is not set or empty
macro(required_arg VAR MESSAGE)
	if(NOT ${VAR})
		message(FATAL_ERROR ${MESSAGE})
	endif()
endmacro()

# Figure out where to unpack a component given a hint (suggested location).
# Archives downloaded from Nexus are typically of the form SystemID/Platform/<library files> and FetchContent keeps the last directory
# when extracting contents so if the suggested path ends with Platform/ we trim it. Otherwise we return the suggested location unmodified.
function(determine_unpack_location HINT BUILD_PLATFORM UNPACK_LOCATION)
	get_filename_component(_MAYBE_PLATFORM ${HINT} NAME)
	get_filename_component(_TMP_LOCATION ${HINT} DIRECTORY)

	if((_MAYBE_PLATFORM STREQUAL BUILD_PLATFORM))
		set(${UNPACK_LOCATION} ${_TMP_LOCATION} PARENT_SCOPE)
	else()
		set(${UNPACK_LOCATION} ${HINT} PARENT_SCOPE)
	endif()
endfunction()

# download_nexus_component downloads an external library from Nexus and unpacks it in a given location
# Unlike for CVS components, the download actually happens during configure stage, not build stage
function(download_nexus_component COMPONENT)
	set(OPTIONS)
	set(ONE_VALUE_ARGS LOCATION BUILD_PLATFORM VERSION BUILD_ID HASH)
	set(MULTI_VALUE_ARGS)
	cmake_parse_arguments(_COMPONENT "${OPTIONS}" "${ONE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN})
	required_arg(_COMPONENT_LOCATION "A location for ${COMPONENT} checkout must be provided.")

	string(TOUPPER ${COMPONENT} COMPONENT_UPPER)
	set_if_empty(_COMPONENT_BUILD_PLATFORM ${${COMPONENT_UPPER}_BUILD_PLATFORM})
	set_if_empty(_COMPONENT_VERSION ${${COMPONENT_UPPER}_VERSION})
	set_if_empty(_COMPONENT_BUILD_ID ${${COMPONENT_UPPER}_BUILD_ID})
	set_if_empty(_COMPONENT_HASH ${${COMPONENT_UPPER}_SHA256})

	if(WIN32)
		set(FILE_EXT ".zip")
	else()
		set(FILE_EXT ".tar.gz")
	endif()

	set(NEXUSROOT "https://nexus.wolfram.com" CACHE INTERNAL "Base Nexus URL")
	set(NEXUS_REPOSITORY "re-components")
	set(NEXUS_ASSET_PATH "${COMPONENT}/${_COMPONENT_VERSION}/${SYSTEMID}/${_COMPONENT_BUILD_PLATFORM}/${COMPONENT}-${_COMPONENT_BUILD_ID}_Files")
	set(COMPONENT_URL "${NEXUSROOT}/repository/${NEXUS_REPOSITORY}/${NEXUS_ASSET_PATH}${FILE_EXT}")

	determine_unpack_location(${_COMPONENT_LOCATION} ${_COMPONENT_BUILD_PLATFORM} UNPACK_LOCATION)

	# In CMake 3.24+ use timestamps from extraction time and not those from the archive. Older CMake has no such option.
	if(POLICY CMP0135)
		cmake_policy(SET CMP0135 NEW)
	endif()

	include(FetchContent)
	FetchContent_declare(
		${COMPONENT}
		URL ${COMPONENT_URL}
		URL_HASH SHA256=${_COMPONENT_HASH}
		SOURCE_DIR ${UNPACK_LOCATION}
		CONFIGURE_COMMAND ""
		BUILD_COMMAND ""
	)

	message(STATUS "Downloading / verifying ${COMPONENT} from ${COMPONENT_URL}")
	FetchContent_MakeAvailable(${COMPONENT})

	if (EXISTS ${_COMPONENT_LOCATION})
		message(STATUS "Using ${COMPONENT} from ${_COMPONENT_LOCATION}.")
	else()
		string(
			CONCAT
			_msg
			"Could not get ${COMPONENT} or the checkout directory has been removed.\n"
			"If the latter, delete ${FETCHCONTENT_BASE_DIR} and re-run cmake."
		)
		message(FATAL_ERROR ${_msg})
	endif()
endfunction()
