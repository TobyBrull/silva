cmake_minimum_required(VERSION 3.30)
project(silva LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(thirdparty/reflect-cpp)
add_subdirectory(thirdparty/cxxopts)

# find_package(Threads REQUIRED)
# find_package(Boost REQUIRED)
find_package(Fmt REQUIRED)

file(GLOB SILVA_LIB_FILES "src/*.hpp" "src/*.cpp")
file(GLOB SILVA_TEST_FILES "src/*.tpp")
file(GLOB SILVA_MAIN_FILES "src/*.mpp")

foreach(file ${SILVA_TEST_FILES} ${SILVA_MAIN_FILES})
  set_source_files_properties("${file}" PROPERTIES LANGUAGE CXX)
endforeach()

# Library

add_library(silva ${SILVA_LIB_FILES})
target_link_libraries(silva PUBLIC
  fmt::fmt
  reflect-cpp
)

# Command-line tools

foreach(main_file ${SILVA_MAIN_FILES})
  get_filename_component(basename "${main_file}" NAME_WE)
  add_executable("silva_${basename}" ${main_file})
  target_link_libraries("silva_${basename}"
    silva
    cxxopts
  )
endforeach()

# Tests

include(CTest)

add_executable(silva_test "${SILVA_TEST_FILES}")
target_link_libraries(silva_test PRIVATE
  silva
  Catch2::Catch2WithMain
)

find_package(Catch2 3 REQUIRED)
include(Catch)

catch_discover_tests(silva_test)

