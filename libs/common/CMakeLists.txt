cmake_minimum_required(VERSION 3.22)

# Create the common library
add_library(common STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/logging.c
)


# Set include directories
target_include_directories(common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Inc
)


# Link against required HAL libraries
target_link_libraries(common PUBLIC
    stm32cubemx
)

message(STATUS "Added common library")
