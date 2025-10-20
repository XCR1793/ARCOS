# CMake configuration for ConfigReader test
cmake_minimum_required(VERSION 3.16)

# Create executable for config reader test
add_executable(processing_ConfigReader
  ConfigReader.cpp
)

# Link required libraries
target_link_libraries(processing_ConfigReader
  ${ARCOS_LIBRARY_NAME}
)

# Set target properties
set_target_properties(processing_ConfigReader PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
)