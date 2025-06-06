cmake_minimum_required(VERSION 3.22)

# Use the existing toolchain file
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    message(STATUS "Setting toolchain file")
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/CubeMX/cmake/gcc-arm-none-eabi.cmake)
endif()

project(PicoAPRS-RTOS-Firmware C CXX ASM)

# Export compile commands for intellisense and other tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add DEBUG definitions for Debug Builds
# This is a generator expression which will be evaluated during build system generation to produce the specific definitions

add_compile_definitions($<$<CONFIG:Debug>:DEBUG>)

############################ Feature Selections #############################################
# Feature selection for GPS library (NON-BLOCKING if ON or BLOCKING if OFF)
option(GPS_NON_BLOCKING "Enable GPS Blocking/Non-Blocking Feature" ON)
# ADD more options here as needed
##############################################################################################


# Add libraries
add_subdirectory(CubeMX/cmake/stm32cubemx)
add_subdirectory(libs/common)
add_subdirectory(libs/gps)
add_subdirectory(libs/bme68x)
add_subdirectory(libs/transceiver)


# Add test app
add_subdirectory(apps/uart_echo_app)

# Add tests
add_subdirectory(tests/gps_simple_test)
add_subdirectory(tests/si_config_and_transmit)
add_subdirectory(tests/si_write_and_read_test)
add_subdirectory(tests/si_aprs_transmission_test)
add_subdirectory(tests/bme68x_test)
