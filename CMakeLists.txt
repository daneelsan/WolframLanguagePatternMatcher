cmake_minimum_required(VERSION 3.20)

#-----------------------------------------------------------------------------
# Project Definition
#-----------------------------------------------------------------------------
project(PatternMatcherLibrary
    DESCRIPTION "Wolfram Pattern Matcher implemented in C++"
    LANGUAGES CXX C)

#-----------------------------------------------------------------------------
# Paclet and Runtime Setup (must be included before dependencies)
#-----------------------------------------------------------------------------
include(cmake/WRICMakeModules/Paclets/Paclets.cmake)
os_default_settings()

#-----------------------------------------------------------------------------
# External Dependencies
#-----------------------------------------------------------------------------
include(cmake/FindWolframEngine.cmake)

#-----------------------------------------------------------------------------
# WolframLibrary
#-----------------------------------------------------------------------------
# Add Wolfram system include directory
set(WOLFRAM_INCLUDE_DIR
    "${WOLFRAM_APP_DIR}/Contents/SystemFiles/IncludeFiles/C"
    CACHE PATH "Wolfram system include directory"
)

if(NOT IS_DIRECTORY "${WOLFRAM_INCLUDE_DIR}")
    message(FATAL_ERROR "WOLFRAM_INCLUDE_DIR not found: ${WOLFRAM_INCLUDE_DIR}")
endif()

#-----------------------------------------------------------------------------
# WSTP
#-----------------------------------------------------------------------------
# Construct WSTP path inside Wolfram.app
set(WOLFRAM_WSTP_PATH
    "${WOLFRAM_APP_DIR}/Contents/SystemFiles/Links/WSTP/DeveloperKit/${SYSTEMID}/CompilerAdditions"
    CACHE PATH "Path to Wolfram WSTP CompilerAdditions"
)

if(NOT IS_DIRECTORY "${WOLFRAM_WSTP_PATH}")
    message(FATAL_ERROR "WOLFRAM_WSTP_PATH not found: ${WOLFRAM_WSTP_PATH}")
endif()

# Load WSTP configuration
include(${WOLFRAM_WSTP_PATH}/WSTP.cmake)

# Export a variable with the actual library target
set(WSTP_LIBRARIES WSTP::DYNAMIC_LIBRARY CACHE STRING "WSTP libraries")


#-----------------------------------------------------------------------------
# Build Configuration
#-----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Force Debug/Release only (multi-config safe)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Available build types" FORCE)

#-----------------------------------------------------------------------------
# Install Directories
#-----------------------------------------------------------------------------
# Default install prefix = project source dir
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}" CACHE PATH "Install path prefix" FORCE)
endif()

set(INSTALL_DIR PatternMatcher CACHE STRING "Output install directory")

set(LIBRARY_DESTINATION "${CMAKE_INSTALL_PREFIX}/${INSTALL_DIR}/LibraryResources/${SYSTEMID}")
set(EXECUTABLE_DESTINATION "${CMAKE_INSTALL_PREFIX}/${INSTALL_DIR}/ExecutableResources/${SYSTEMID}")
set(KERNEL_DESTINATION "${CMAKE_INSTALL_PREFIX}/${INSTALL_DIR}/Kernel")

message(STATUS "Library resources will be installed to: ${LIBRARY_DESTINATION}")
message(STATUS "Executable resources will be installed to: ${EXECUTABLE_DESTINATION}")
message(STATUS "Kernel resources will be installed to: ${KERNEL_DESTINATION}")

#-----------------------------------------------------------------------------
# Library Target
#-----------------------------------------------------------------------------
add_library(${PROJECT_NAME} SHARED
    src/Expr.cpp
    src/LibraryLink.cpp
    src/ObjectFactory.cpp
    src/AST/MExpr.cpp
    src/AST/MExprNormal.cpp
    src/AST/MExprLiteral.cpp
    src/AST/MExprSymbol.cpp
    src/AST/MExprEnvironment.cpp
    src/AST/MExprPatternTools.cpp
    src/VM/VirtualMachine.cpp
    src/VM/PatternBytecode.cpp
    src/VM/Opcode.cpp
    src/VM/CompilePatternToBytecode.cpp
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${WOLFRAM_INCLUDE_DIR}
    ${WOLFRAM_WSTP_PATH}
)

# Link required libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${WOLFRAM_ENGINE_LIB}
    ${WSTP_LIBRARIES}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:PM_LOG_DEBUG>
    $<$<CONFIG:Debug>:PM_LOG_LEVEL_TRACE>
)

# Disable platform-specific "lib" prefix for shared library
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

#-----------------------------------------------------------------------------
# Build-type specific compile options & definitions (multi-config safe)
#-----------------------------------------------------------------------------
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:-g3 -O0 -fno-omit-frame-pointer>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:DEBUG _DEBUG>
)

#-----------------------------------------------------------------------------
# Install Rules
#-----------------------------------------------------------------------------
install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}
    RUNTIME DESTINATION ${LIBRARY_DESTINATION}
    LIBRARY DESTINATION ${LIBRARY_DESTINATION}
)
