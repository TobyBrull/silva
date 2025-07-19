include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY  https://github.com/catchorg/Catch2.git
  GIT_TAG         v3.8.1
  GIT_SHALLOW     TRUE
  GIT_PROGRESS    TRUE
)
FetchContent_MakeAvailable(Catch2)
