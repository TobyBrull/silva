file(GLOB_RECURSE HH_DATE_FILES "src/*.cpp" "src/*.mm")
add_library(hh-date SHARED ${HH_DATE_FILES})
target_include_directories(hh-date PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_definitions(hh-date PRIVATE AUTO_DOWNLOAD=0 HAS_REMOTE_API=0)
