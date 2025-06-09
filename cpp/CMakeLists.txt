add_subdirectory(thirdparty/reflect-cpp)
add_subdirectory(thirdparty/date-3.0.3)

# find_package(Threads REQUIRED)
# find_package(Boost REQUIRED)
find_package(Fmt REQUIRED)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in"
  "${CMAKE_CURRENT_BINARY_DIR}/config.hpp"
  )

file(GLOB SILVA_LIB_FILES
  "canopy/*.cpp"
  "syntax/*.cpp"
  "zoo/fern/*.cpp"
  "zoo/lox/*.cpp"
  "*.cpp"
)
file(GLOB_RECURSE SILVA_TEST_FILES "*.tpp")
file(GLOB_RECURSE SILVA_MAIN_FILES "*.mpp")

foreach(file ${SILVA_TEST_FILES} ${SILVA_MAIN_FILES})
  set_source_files_properties("${file}" PROPERTIES LANGUAGE CXX)
endforeach()

# Library

add_library(silvalib ${SILVA_LIB_FILES})
target_include_directories(silvalib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(silvalib PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(silvalib PUBLIC
  fmt::fmt
  reflect-cpp
  hh-date
)
target_link_libraries(silvalib PUBLIC TracyClient)

# Command-line tools

foreach(main_file ${SILVA_MAIN_FILES})
  get_filename_component(basename "${main_file}" NAME_WE)
  if(basename STREQUAL "silva")
    set(executable_name "silva")
  else()
    set(executable_name "silva_${basename}")
  endif()
  add_executable("${executable_name}" "${main_file}")
  target_link_libraries("${executable_name}" silvalib)
endforeach()

# Tests

include(CTest)

add_executable(silva_test "${SILVA_TEST_FILES}")
target_link_libraries(silva_test PRIVATE
  silvalib
  Catch2::Catch2WithMain
)

find_package(Catch2 3 REQUIRED)
include(Catch)

catch_discover_tests(silva_test)

