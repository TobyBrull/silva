cmake_minimum_required(VERSION 3.30)
project(silva LANGUAGES CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS On CACHE BOOL "Always write compile_commands.json" FORCE)

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
option(USE_TRACY "Build using tracy" OFF)
include(cmake/tracy.cmake)
include(cmake/fmtlib.cmake)

add_subdirectory(cpp)
