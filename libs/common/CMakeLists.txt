# Shared Libraries CMakeLists.txt
cmake_minimum_required(VERSION 3.22)
project(common)

# Create the common library
add_library(common STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/logging.c
)

# Define include directories
target_include_directories(common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Inc
)


message(STATUS "Building common library")

# Link with required dependencies
target_link_libraries(common PUBLIC
    stm32cubemx
    # Add dependencies here
)

message(STATUS "Added common library")
