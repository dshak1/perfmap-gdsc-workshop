cmake_minimum_required(VERSION 3.14)
project(perfmap_10d_starter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)

FetchContent_Declare(
  abseil-cpp
  GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
  GIT_TAG        20240722.0
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest abseil-cpp)

add_library(perfmap_starter INTERFACE)
target_include_directories(perfmap_starter INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(perfmap_starter INTERFACE
  absl::status
  absl::statusor
)

enable_testing()

add_executable(perfmap_starter_tests
  tests/hash_map_test.cc
)
target_link_libraries(perfmap_starter_tests PRIVATE
  perfmap_starter
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(perfmap_starter_tests)
