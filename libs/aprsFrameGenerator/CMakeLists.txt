# Aprs Frame Generator Library CMakeLists.txt
cmake_minimum_required(VERSION 3.22)
project(aprsFrameGenerator)

# Create the GPS library
add_library(aprsFrameGenerator STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/aprs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ax25.c
)

# Define include directories
target_include_directories(aprsFrameGenerator PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Inc
)

# Link with required dependencies
target_link_libraries(aprsFrameGenerator PUBLIC
    stm32cubemx
    # Add dependencies here
)

message(STATUS "Reese is a weiner")