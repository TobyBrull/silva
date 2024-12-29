file(GLOB_RECURSE REFLECT_CPP_FILES "src/*.cpp" "src/*.c")
add_library(reflect-cpp SHARED ${REFLECT_CPP_FILES})
target_include_directories(reflect-cpp PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/rfl/thirdparty"
)
