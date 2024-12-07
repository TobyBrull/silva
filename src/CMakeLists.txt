file(GLOB_RECURSE SILVA_LIB_FILES "*.hpp" "*.cpp")
file(GLOB_RECURSE SILVA_TEST_FILES "*.tpp")
file(GLOB_RECURSE SILVA_MAIN_FILES "*.mpp")

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

