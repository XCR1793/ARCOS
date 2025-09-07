cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(ARCOS_library)


option(BUILD_SYSTEM "Off by default to prevent platformio espidf issues" OFF)
if(BUILD_SYSTEM)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED True)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

  # Detect host vs embedded build
  # PlatformIO automatically defines ESP_PLATFORM for ESP32 targets
  if(NOT DEFINED ESP_PLATFORM)
    set(HOST_BUILD ON)
  else()
    set(HOST_BUILD OFF)
  endif()

  # Library sources (common to both host and embedded)
  file(GLOB_RECURSE LIBRARY_SOURCES "src/*.cpp")
  add_library(${PROJECT_NAME} ${LIBRARY_SOURCES})

  target_include_directories(${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
  )

  # Option to enable/disable googles unit tests (host only)
  option(BUILD_TESTS "Build Google unit tests" ON) # Overridden by: -DBUILD_TESTS=OFF
  if(BUILD_TESTS AND HOST_BUILD)
    enable_testing()

    if(MSVC)
      set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    endif()

    include(FetchContent)
    message(STATUS "Fetch Google Tests")
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(googletest)

    file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")

    foreach(TEST_UNIT ${TEST_SOURCES})
      get_filename_component(TEST_FILE_NAME ${TEST_UNIT} NAME_WE) # Extract filename without extension
      get_filename_component(TEST_DIR ${TEST_UNIT} DIRECTORY) # Extract directory from Test Unit
      get_filename_component(TEST_DIR_NAME ${TEST_DIR} NAME) # Extract name from Test Unit
      set(TEST_NAME "${TEST_DIR_NAME}_${TEST_FILE_NAME}")
      message(STATUS "Adding test executable: ${TEST_NAME} -> ${TEST_UNIT}")

      add_executable(${TEST_NAME} ${TEST_UNIT})
      target_link_libraries(${TEST_NAME} PRIVATE ${PROJECT_NAME} gtest gtest_main)
      add_test(
        NAME ${TEST_NAME}
        COMMAND $<TARGET_FILE:${TEST_NAME}> # Windows test registration compatibility
      )
    endforeach()
  endif()
endif()