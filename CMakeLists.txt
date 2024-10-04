cmake_minimum_required(VERSION 3.24)
project(BuilderProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(FETCHCONTENT_QUIET OFF)

# ---- Use CPM to download and install third party packages ----
include(cmake/CPM.cmake)

# ---- Donload and compile Boost ----
cpmaddpackage(
  NAME
  Boost
  VERSION
  1.84.0
  URL
  https://github.com/boostorg/boost/releases/download/boost-1.84.0/boost-1.84.0.tar.xz
  URL_HASH
  SHA256=2e64e5d79a738d0fa6fb546c6e5c2bd28f88d268a2a080546f74e5ff98f29d0e
  OPTIONS
  "BOOST_ENABLE_CMAKE ON"
  # Note the escapes below!
  "BOOST_INCLUDE_LIBRARIES program_options\\\;")

# ---- Create binary ----
add_executable(builder src/builder.cpp src/input.cpp src/heft.cpp)
if(MSVC)
  target_compile_options(builder PRIVATE /W4 /WX)
else()
  target_compile_options(builder PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
target_link_libraries(builder PRIVATE Boost::program_options)

# ---- Donload and compile GTest ----
cpmaddpackage(
  NAME
  googletest
  GITHUB_REPOSITORY
  google/googletest
  GIT_TAG
  release-1.12.1
  VERSION
  1.12.1
  OPTIONS
  "INSTALL_GTEST OFF"
  "gtest_force_shared_crt")

# ---- Create test binary ----
add_executable(builder_test src/test.cpp src/input.cpp src/heft.cpp)
target_link_libraries(builder_test gtest gtest_main gmock)
if(MSVC)
  target_compile_options(builder_test PRIVATE /W4 /WX)
else()
  target_compile_options(builder_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# ---- Enable testing ----
enable_testing()
add_test(builder_test builder_test)
