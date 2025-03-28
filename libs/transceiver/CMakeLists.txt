cmake_minimum_required(VERSION 3.22)
project(transceiver)

add_library(transceiver STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/si4463.c
)

target_include_directories(transceiver PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Inc
)

target_link_libraries(transceiver PUBLIC
    stm32cubemx
    common
)

message(STATUS "Added Transceiver library")