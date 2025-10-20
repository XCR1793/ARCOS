# CMakeLists.txt for core math tests

cmake_minimum_required(VERSION 3.10)

# Find Google Test
find_package(GTest REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${GTEST_INCLUDE_DIRS})

# Create test executable
add_executable(test_fast_trig
    test_fast_trig.cpp
)

# Link against Google Test and any required libraries
target_link_libraries(test_fast_trig
    ${GTEST_LIBRARIES}
    ${GTEST_MAIN_LIBRARIES}
    pthread
)

# Set C++17 standard
set_property(TARGET test_fast_trig PROPERTY CXX_STANDARD 17)

# Add test to CTest
add_test(NAME FastTrigTest COMMAND test_fast_trig)

# Add compiler flags for better debugging and optimization
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(test_fast_trig PRIVATE -Wall -Wextra -O2)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(test_fast_trig PRIVATE /W4 /O2)
endif()