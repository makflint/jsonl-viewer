# Wrapper toolchain for Emscripten.
#
# Emscripten scripts require Python >= 3.10. This file locates a suitable
# Python, sets EMSDK_PYTHON so Emscripten's em++/emcc scripts pick it up,
# then delegates to the real Emscripten toolchain via $EMSDK.
#
# Usage:
#   cmake -B build_wasm -DCMAKE_TOOLCHAIN_FILE=cmake/emscripten_toolchain.cmake
#
# Requires the EMSDK environment variable to point to your emsdk root.

find_package(Python3 3.10 REQUIRED)
set(ENV{EMSDK_PYTHON} "${Python3_EXECUTABLE}")

# Inject EMSDK_PYTHON into every compiler invocation during the build phase.
# cmake -E env sets environment variables for a single command, so make
# picks it up even though it runs in a separate process from cmake.
set(_emsdk_python_env "${CMAKE_COMMAND};-E;env;EMSDK_PYTHON=${Python3_EXECUTABLE}")
set(CMAKE_CXX_COMPILER_LAUNCHER  ${_emsdk_python_env})
set(CMAKE_C_COMPILER_LAUNCHER    ${_emsdk_python_env})
set(CMAKE_CXX_LINKER_LAUNCHER    ${_emsdk_python_env})
set(CMAKE_C_LINKER_LAUNCHER      ${_emsdk_python_env})

set(_emscripten_toolchain
    "$ENV{EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"
)
if(NOT EXISTS "${_emscripten_toolchain}")
    message(FATAL_ERROR
        "Emscripten toolchain not found at: ${_emscripten_toolchain}\n"
        "Set the EMSDK environment variable to your emsdk root directory."
    )
endif()
include("${_emscripten_toolchain}")
