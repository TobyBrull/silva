if(USE_TRACY)
  include(FetchContent)
  FetchContent_Declare(
    tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy.git
    GIT_TAG v0.12.1
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE)
  FetchContent_MakeAvailable(tracy)
else()
  add_library(TracyClient INTERFACE)
endif()
