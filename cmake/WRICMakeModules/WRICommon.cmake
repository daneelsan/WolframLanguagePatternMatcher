# Sets ${RESULT_VAR} to C, CXX, or None, depending on which is active, preferring C to CXX if both are active.
function(wri_get_test_language RESULT_VAR)
	if(CMAKE_C_COMPILER_LOADED)
		set(${RESULT_VAR} C PARENT_SCOPE)
		if(CMAKE_CXX_COMPILER_LOADED) # Both C and C++ enabled
			if((NOT CMAKE_C_COMPILER_ID STREQUAL CMAKE_CXX_COMPILER_ID) AND (NOT WRI_ALLOW_MIXED_COMPILERS))
				message(FATAL_ERROR "C and C++ compilers do not match.")
			endif()
		endif() # Both C and C++ enabled
	elseif(CMAKE_CXX_COMPILER_LOADED)
		set(${RESULT_VAR} CXX PARENT_SCOPE)
	else()
		set(${RESULT_VAR} None PARENT_SCOPE)
	endif()
endfunction(wri_get_test_language)



# Get the word size in bytes targeted by the enabled language ${TEST_LANGUAGE} and store the result
# in ${RESULT_VAR}.  If  the language isn't enabled, issue a warning and store the result 8 in the
# variable, as that's what most developers are using and targeting.  If the language is None, give
# the answer 8 silently for the same reason.  We can't depend on CMAKE_SIZEOF_VOID_P to be defined
# or correct if we don't have a compiler, hence the need for a test language.
function(wri_get_language_word_size TEST_LANGUAGE RESULT_VAR)
	if(CMAKE_${TEST_LANGUAGE}_SIZEOF_DATA_PTR)
		set(${RESULT_VAR} ${CMAKE_${TEST_LANGUAGE}_SIZEOF_DATA_PTR} PARENT_SCOPE)
	elseif(NOT CMAKE_${TEST_LANGUAGE}_COMPILER_LOADED)
		set(${RESULT_VAR} 8 PARENT_SCOPE)
		if(NOT TEST_LANGUAGE STREQUAL "None")
			message(AUTHOR_WARNING "The ${TEST_LANGUAGE} compiler has not been enabled yet.  Assuming word size of 8.")
		endif()
	else()
		message(FATAL_ERROR "The ${TEST_LANGUAGE} compiler does not define a word size!")
	endif()
endfunction(wri_get_language_word_size)


# Automatically detects SYSTEMID based on the host system, compiler settings, etc. and then stores the
# result in a cache variable SYSTEMID.  Issues a warning if SYSTEMID is already defined and not equal
# to one of the known values.
function(wri_autodetect_systemid)
	if(ARGC LESS 1)
		wri_get_test_language(TEST_LANGUAGE)
	else()
		set(TEST_LANGUAGE ${ARGV0})
	endif()

	if(ARGC LESS 2)
		wri_get_language_word_size(${TEST_LANGUAGE} WORD_SIZE)
	else()
		set(WORD_SIZE ${ARGV1})
	endif()

	# clang is used and cross compiles on multiple platforms.  Let's try to learn what it is targeting.
	if(CMAKE_${TEST_LANGUAGE}_COMPILER_ID STREQUAL "Clang")
		execute_process(
			COMMAND ${CMAKE_${TEST_LANGUAGE}_COMPILER} --version
			OUTPUT_VARIABLE CLANG_DASH_DASH_VERSION # this isn't simply the numeric version, so make the variable name clear.
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)
	endif()
	if(ARGC GREATER 2)
		set(${ARGV2} "${CLANG_DASH_DASH_VERSION}" PARENT_SCOPE)
	endif()

	if(UNIX)
		# On Unix, we might use the host processor for choosing SystemID, but apparently that is not set for whatever reason
		# prior to calling project()
		if(CMAKE_HOST_SYSTEM_PROCESSOR)
			set(processor ${CMAKE_HOST_SYSTEM_PROCESSOR})
		else()
			execute_process(
				COMMAND uname -m
				OUTPUT_VARIABLE processor
				OUTPUT_STRIP_TRAILING_WHITESPACE
			)
		endif()
	elseif(WIN32)
		# If someone was kind enough to set the VS Platform, use it.  CMake by default also sticks the architecture in the
		# linker flags, so check there in case the project hasn't overwritten it.
		if(CMAKE_GENERATOR_PLATFORM STREQUAL "x64" OR CMAKE_STATIC_LINKER_FLAGS MATCHES "machine:x64")
			set(processor AMD64)
		elseif(CMAKE_GENERATOR_PLATFORM STREQUAL "Win32" OR CMAKE_STATIC_LINKER_FLAGS MATCHES "machine:X86")
			set(processor X86)
		elseif(CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64" OR CMAKE_STATIC_LINKER_FLAGS MATCHES [[machine:ARM64([ \t]|$)]])
			set(processor ARM64)
		# CMake seems to convert -A ARM64EC to /machine:ARM64X.  Whatever the differences between these, it won't be relevant
		# unless and until we start actually doing something with them.
		elseif(CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64EC" OR CMAKE_STATIC_LINKER_FLAGS MATCHES [[machine:ARM64(EC|X)([ \t]|$)]])
			set(processor ARM64EC)
		elseif(CMAKE_GENERATOR_PLATFORM STREQUAL "ARM" OR CMAKE_STATIC_LINKER_FLAGS MATCHES [[machine:ARM([ \t]|$)]])
			set(processor ARM)
		# I sure hope this doesn't come up in practice ...
		elseif(CMAKE_STATIC_LINKER_FLAGS MATCHES "machine:EBC")
			set(processor EFI)
		else()
			# If we're here, a compiler hasn't been activated, the user is using NMake and has managed to foil us by changing
			# the linker flags, or yet another ABI has been invented, so we just fall back to the host architecture and hope
			# for the best.
			set(processor "$ENV{PROCESSOR_ARCHITECTURE}")
		endif()
	endif()

	# Determine the build's, which will typically be the current machine's, $SystemID.  This is mostly done with standard
	# CMake variables (possibly indirectly through processor and WORD_SIZE), with the following exceptions:
	#     A) we use Clang triples to detect Android (something that will go away soon)
	#     B) we have fallbacks (see above) for cases where the CMake variables are not defined for whatever reason.
	# SYSTEMID can be specified explicitly on the CMake command line for certain cross-builds (such as iOS), but it is
	# better to set the right CMake or environment variables (such as CMAKE_SYSTEM_NAME or CFLAGS) for a cross-build.
	if(CMAKE_SYSTEM_NAME STREQUAL "Android" OR CLANG_DASH_DASH_VERSION MATCHES "android")
		set(INITIAL_SYSTEMID Android)
	elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" AND processor MATCHES "aarch64")
		set(INITIAL_SYSTEMID Linux-ARM64)
	elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" AND processor MATCHES "arm")
		set(INITIAL_SYSTEMID Linux-ARM)
	elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" AND WORD_SIZE STREQUAL "8")
		set(INITIAL_SYSTEMID Linux-x86-64)
	elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" AND WORD_SIZE STREQUAL "4")
		set(INITIAL_SYSTEMID Linux)
	elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows" AND processor STREQUAL "ARM64")
		set(INITIAL_SYSTEMID Windows-ARM64)
	elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows" AND processor STREQUAL "AMD64")
		set(INITIAL_SYSTEMID Windows-x86-64)
	elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows" AND processor STREQUAL "X86")
		set(INITIAL_SYSTEMID Windows)
	elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
		set(INITIAL_SYSTEMID iOS)
	elseif(CMAKE_SYSTEM_NAME STREQUAL "visionOS")
		set(INITIAL_SYSTEMID visionOS)
	elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
		# First, account for the standard CMake cross-compile technique
		if(CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64")
			set(INITIAL_SYSTEMID MacOSX-x86-64)
		elseif(CMAKE_OSX_ARCHITECTURES MATCHES "^arm64e?$")
			set(INITIAL_SYSTEMID MacOSX-ARM64)
		# then fall back to the host architecture.
		elseif(processor STREQUAL "x86_64")
			set(INITIAL_SYSTEMID MacOSX-x86-64)
		elseif(processor MATCHES "arm64")
			set(INITIAL_SYSTEMID MacOSX-ARM64)
		endif()
	else()
		set(INITIAL_SYSTEMID Indeterminate)
	endif()

	if(NOT DEFINED SYSTEMID AND NOT INITIAL_SYSTEMID STREQUAL "Indeterminate")
		message(STATUS "Autodetected SYSTEMID ${INITIAL_SYSTEMID}")
	elseif(DEFINED SYSTEMID)
		message(STATUS "SYSTEMID manually set to ${SYSTEMID}")
	else()
		message(WARNING "Unable to determine SYSTEMID.")
	endif()

	set(SYSTEMID ${INITIAL_SYSTEMID} CACHE STRING "Manually set or autodetected value of $SystemID")
	# We have this test here and not in the block `elseif(DEFINED SYSTEMID)` a few lines above so that if new
	# auto-detected values of SystemID are added, they generate messages here to keep things in sync.
	if(NOT SYSTEMID MATCHES "^(Android|iOS|Indeterminate|Linux|Linux-ARM|Linux-x86-64|Linux-ARM64|MacOSX-ARM64|MacOSX-x86-64|visionOS|Windows|Windows-x86-64|Windows-ARM64)$")
		message(WARNING "SYSTEMID ${SYSTEMID} is not a recognized value.")
	endif()
endfunction(wri_autodetect_systemid)


# Helper function for wri_install_symbols and wri_strip.  Tests if stripping is globally disabled, so
# we should either not strip, or not try to install non-existent symbol files.  The name is a bit of
# a misnomer, since it could also prevent installation of PDB files (no notion of stripping for MSVC),
# but I couldn't come up with a better name.
function(stripping_disabled RESULT_VAR)
	# A very crude test for build options, typically set globally, that are incompatible with stripping,
	# namely --coverage (for gcov et alia) and -pg (profiling).  We also allow a global variable set by
	# the project to disable stripping.
	if("${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_RELEASE} ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}" MATCHES
		"(^|[^A-Za-z0-9=-])(--coverage|-pg)([^A-Za-z0-9=-]|$)" OR WRI_DISABLE_STRIPPING)
		set(${RESULT_VAR} TRUE PARENT_SCOPE)
	else()
		set(${RESULT_VAR} FALSE PARENT_SCOPE)
	endif()
endfunction(stripping_disabled)


# Installs debug information of NAME into LOCATION, if it exists in a separate file.  This could be MSVC
# PDB files, Mach-O dSYM files or ELF .debug files.  This function can be used to install the file next
# to the binary itself--to ease debugging of binaries that really want to be in a full layout into order
# to tested--or to install all debug files in a central location to be collected by RE.
function(wri_install_symbols NAME LOCATION)
	# is stripping globally disabled, in which case there will not be any symbol files to install?
	stripping_disabled(isDisabled)
	get_target_property(targetType ${NAME} TYPE)
	# Static libraries don't have normal shared library symbols.  In MSVC they only have "compiler PDB", not
	# "linker PDB" files of the type handled by TARGET_PDB_FILE.  On Unix, there doesn't seem to be a way to
	# separate the info of a static archive into a separate library.
	if(isDisabled OR targetType STREQUAL "STATIC_LIBRARY" OR WRI_DISABLE_SYMBOLS_INSTALL)
		return()
	endif()
	if(WIN32 AND NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang") AND NOT (CMAKE_C_COMPILER_ID MATCHES "Clang"))
		install(FILES $<TARGET_PDB_FILE:${NAME}> DESTINATION ${LOCATION})
	elseif(APPLE)
		get_target_property(isAppBundle ${NAME} MACOSX_BUNDLE)
		get_target_property(isFramework ${NAME} FRAMEWORK)
		get_target_property(isLoadableBundle ${NAME} BUNDLE)
		if(isAppBundle OR isFramework OR (isLoadableBundle AND targetType STREQUAL "MODULE_LIBRARY"))
			install(DIRECTORY $<TARGET_BUNDLE_DIR:${NAME}>.dSYM  DESTINATION ${LOCATION} CONFIGURATIONS Release)
		# Strip does not currently deal with the case of a non-bundle module library
		elseif(NOT targetType STREQUAL "MODULE_LIBRARY")
			install(DIRECTORY $<TARGET_FILE:${NAME}>.dSYM  DESTINATION ${LOCATION} CONFIGURATIONS Release)
		endif()
	elseif(UNIX)
		install(FILES $<TARGET_FILE:${NAME}>.debug  DESTINATION ${LOCATION} CONFIGURATIONS Release)
	endif()
endfunction(wri_install_symbols)


# Strip a library/executable in the release configuration, and save the debug symbols in a platform-appropriate
# debug file.  Does nothing in other configurations.
function(wri_strip NAME)
	# is stripping globally disabled, in which case there will not be any symbol files to install?
	stripping_disabled(isDisabled)
	if(isDisabled)
		return()
	endif()
	# If we're doing a cross-build, we may need to use a non-native strip command specified by the project.
	# It is named STRIP_CMD so as to not conflict with the built-in cmake keyword STRIP (string subcommand).
	if(NOT STRIP_CMD)
		set(STRIP_CMD "${CMAKE_STRIP}")
	endif()

	# As above, except we have no conflict to worry about.
	if(NOT OBJCOPY)
		set(OBJCOPY "${CMAKE_OBJCOPY}")
	endif()

	get_target_property(targetType ${NAME} TYPE)
	get_target_property(isAppBundle ${NAME} MACOSX_BUNDLE)
	get_target_property(isFramework ${NAME} FRAMEWORK)
	get_target_property(isLoadableBundle ${NAME} BUNDLE)
	# Only real executables/libraries, not things like object and import libraries, should be stripped.
	if (NOT targetType STREQUAL "EXECUTABLE" AND
			NOT targetType STREQUAL "SHARED_LIBRARY" AND
			NOT targetType STREQUAL "STATIC_LIBRARY" AND
			# Handle macOS loadable bundles
			NOT (targetType STREQUAL "MODULE_LIBRARY" AND isLoadableBundle))
		return()
	endif()

	if(UNIX AND targetType STREQUAL "STATIC_LIBRARY")
		add_custom_command(TARGET ${NAME} POST_BUILD
			COMMAND  $<$<CONFIG:Release>:${STRIP_CMD}>$<$<NOT:$<CONFIG:Release>>:true> -S $<TARGET_FILE:${NAME}>
		)
	elseif(APPLE AND (isAppBundle OR isFramework OR isLoadableBundle))
		add_custom_command(TARGET ${NAME} POST_BUILD
			COMMAND  $<$<CONFIG:Release>:dsymutil>$<$<NOT:$<CONFIG:Release>>:true> -o $<TARGET_BUNDLE_DIR:${NAME}>.dSYM $<TARGET_FILE:${NAME}>
			COMMAND  $<$<CONFIG:Release>:${STRIP_CMD}>$<$<NOT:$<CONFIG:Release>>:true> -u -r $<TARGET_FILE:${NAME}>
		)
	elseif(APPLE)
		add_custom_command(TARGET ${NAME} POST_BUILD
			COMMAND  $<$<CONFIG:Release>:dsymutil>$<$<NOT:$<CONFIG:Release>>:true> $<TARGET_FILE:${NAME}>
			COMMAND  $<$<CONFIG:Release>:${STRIP_CMD}>$<$<NOT:$<CONFIG:Release>>:true> -u -r $<TARGET_FILE:${NAME}>
		)
	elseif(UNIX)
		add_custom_command(TARGET ${NAME} POST_BUILD
			COMMAND  $<$<CONFIG:Release>:${OBJCOPY}>$<$<NOT:$<CONFIG:Release>>:true>  --only-keep-debug $<TARGET_FILE:${NAME}> $<TARGET_FILE:${NAME}>.debug
			COMMAND  $<$<CONFIG:Release>:${OBJCOPY}>$<$<NOT:$<CONFIG:Release>>:true>  --add-gnu-debuglink=$<TARGET_FILE:${NAME}>.debug $<TARGET_FILE:${NAME}>
			COMMAND  $<$<CONFIG:Release>:${STRIP_CMD}>$<$<NOT:$<CONFIG:Release>>:true>  $<TARGET_FILE:${NAME}>
		)
	endif()
endfunction(wri_strip)


# temporary compatibility shim while the kernel build system is updated
function(install_target_pdb NAME LOCATION)
	wri_install_symbols(${NAME} ${LOCATION})
endfunction(install_target_pdb)

function(strip NAME)
	wri_strip(${NAME})
endfunction(strip)
