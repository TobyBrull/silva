cmake_minimum_required(VERSION 3.30)
project(silva LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)

add_compile_options(-fsanitize=address,undefined)
add_link_options(-fsanitize=address,undefined)

set(CMAKE_EXPORT_COMPILE_COMMANDS On CACHE BOOL "Always write compile_commands.json" FORCE)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

add_subdirectory(thirdparty/reflect-cpp)
add_subdirectory(cpp)
