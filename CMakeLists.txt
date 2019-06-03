
cmake_minimum_required (VERSION 2.6)
project (fcsim)

set(CMAKE_CXX_STANDARD 14)
if (WIN32)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MT")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    message("force /MT")
endif()
set(CMAKE_BUILD_TYPE "Debug")
set("USE_MSVC_RUNTIME_LIBRARY_DLL" OFF)

add_definitions("-DCPP_SRC_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/\"")


add_subdirectory(modules)
add_subdirectory(src)

