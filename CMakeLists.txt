cmake_minimum_required(VERSION 3.22)

# Use the existing toolchain file
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    message(STATUS "Setting toolchain file")
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/CubeMX/cmake/gcc-arm-none-eabi.cmake)
endif()

project(PicoAPRS-RTOS-Firmware C CXX ASM)


# Add the CubeMX library
add_subdirectory(CubeMX/cmake/stm32cubemx)

# Add your test app
add_subdirectory(apps/uart_echo_app)

# Add tests
add_subdirectory(tests/si_write_and_read_test)