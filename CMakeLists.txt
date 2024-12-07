cmake_minimum_required(VERSION 3.30)
project(silva LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(thirdparty/reflect-cpp)
add_subdirectory(thirdparty/cxxopts)

# find_package(Threads REQUIRED)
# find_package(Boost REQUIRED)
find_package(Fmt REQUIRED)

add_subdirectory(src)
