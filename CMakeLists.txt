cmake_minimum_required(VERSION 3.30)
project(silva LANGUAGES CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS
    On
    CACHE BOOL "Always write compile_commands.json" FORCE)

cmake_path(GET CMAKE_BINARY_DIR PARENT_PATH SILVA_BINARY_PARENT_DIR)
if(SILVA_BINARY_PARENT_DIR STREQUAL CMAKE_SOURCE_DIR)
  cmake_path(GET CMAKE_BINARY_DIR FILENAME SILVA_BINARY_DIR_NAME)
  file(REMOVE "${CMAKE_SOURCE_DIR}/build")
  file(CREATE_LINK "${SILVA_BINARY_DIR_NAME}" "${CMAKE_SOURCE_DIR}/build"
       SYMBOLIC)
endif()

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif()

set(CMAKE_CXX_STANDARD 23)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options(-fsanitize=address,undefined)
  add_link_options(-fsanitize=address,undefined)
  set(SILVA_OPTIMIZED_BUILD OFF)
else()
  set(SILVA_OPTIMIZED_BUILD ON)
endif()

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

include(cmake/tracy.cmake)

find_package(Fmt REQUIRED)
find_package(Catch2 REQUIRED)

include(CTest)
include(Catch)

add_subdirectory(.thirdparty/reflect-cpp)
add_subdirectory(.thirdparty/date-3.0.4)

add_subdirectory(src)
