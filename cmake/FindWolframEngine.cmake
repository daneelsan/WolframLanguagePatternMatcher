# cmake/FindWolframEngine.cmake
#
# Use this module via:
#   find_package(WolframEngine REQUIRED)
# It sets:
#   WOLFRAM_ENGINE_LIB
#   WOLFRAM_APP_DIR
#
# Internally uses:
#   find_wolfram_engine()
#-----------------------------------------------------------------------------

function(find_wolfram_engine)

    # Skip if already found
    if(WOLFRAM_ENGINE_LIB)
        return()
    endif()

    if(NOT WOLFRAM_APP_DIR)
        # Try to guess installation path
        if(APPLE)
            set(DEFAULT_WOLFRAM_APP_DIR "/Applications/Wolfram.app/")
        elseif(UNIX)
            # TODO: Add Linux detection (e.g., /usr/local/Wolfram)
        elseif(WIN32)
            # TODO: Add Windows detection (e.g., C:/Program Files/Wolfram Research/)
        endif()

        if(EXISTS "${DEFAULT_WOLFRAM_APP_DIR}")
            set(WOLFRAM_APP_DIR ${DEFAULT_WOLFRAM_APP_DIR} CACHE PATH "Path to Mathematica installation directory" FORCE)
            message(STATUS "Auto-detected Mathematica installation directory: ${WOLFRAM_APP_DIR}")
        else()
            message(FATAL_ERROR "Unsupported SYSTEMID: ${SYSTEMID}. Please set -DWOLFRAM_APP_DIR=/path/to/Mathematica.")
        endif()
    endif()

    # Try to locate the WolframEngine library
    find_library(_WOLFRAM_ENGINE_LIB
        NAMES WolframEngine
        PATHS ${WOLFRAM_APP_DIR}/Contents/SystemFiles/Libraries/${SYSTEMID}
        NO_DEFAULT_PATH)

    if(EXISTS "${_WOLFRAM_ENGINE_LIB}")
        set(WOLFRAM_ENGINE_LIB "${_WOLFRAM_ENGINE_LIB}" CACHE FILEPATH "Path to WolframEngine library (auto-detected)" FORCE)
        message(STATUS "Found WolframEngine library at: ${WOLFRAM_ENGINE_LIB}")
    else()
        message(FATAL_ERROR "Could not find WolframEngine library. Please specify with -DWOLFRAM_ENGINE_LIB=/path/to/WolframEngine.[so|dylib|dll]")
    endif()
endfunction()

# Automatically invoke the function when using find_package(WolframEngine)
find_wolfram_engine()
