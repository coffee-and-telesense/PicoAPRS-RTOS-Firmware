cmake_minimum_required(VERSION 3.22)
project(transceiver)

# Set default config type
# TODO: create preset or easy way to set this
# Doing it manually for now
if(NOT DEFINED RADIO_CONFIG_TYPE)
    set(RADIO_CONFIG_TYPE "aprs" CACHE STRING "Type of radio configuration")
endif()

# Set valid options for configuration based on your available config files
# Update this list if you add more configuration types
set_property(
    CACHE RADIO_CONFIG_TYPE
    PROPERTY STRINGS
        "aprs"
        "aprs_3k_dev"
        "aprs_fsk_3k_dev"
        "aprs_fsk_3k_9600b"
        "aprs_fsk_3k_0883b"
        "aprs_fsk_3k_0883b_preamble"
        "aprs_gfsk_0883b"
        "aprs_3k_dev_w_preamble"
)
message(STATUS "Using radio configuration type: ${RADIO_CONFIG_TYPE}")

# Configure the radio_config.h file
# This processes the template file radio_config.h.in and generates radio_config.h
# The @RADIO_CONFIG_TYPE@ in the template will be replaced with the actual value
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Inc/radio_config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/radio_config.h
    @ONLY
)

add_library(transceiver STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/si4463.c
)

target_include_directories(transceiver PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Inc
    ${CMAKE_CURRENT_BINARY_DIR}/include   # Include the generated header file
)

target_compile_definitions(transceiver PUBLIC
    $<$<BOOL:${USE_MODEM_RANDOM_TX_SRC}>:USE_MODEM_RANDOM_TX_SRC>
)

target_link_libraries(transceiver PUBLIC
    stm32cubemx
    common
)

if(USE_MODEM_RANDOM_TX_SRC)
    message(WARNING "
    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    !!! WARNING: MODEM RANDOM TX SOURCE IS ENABLED             !!!
    !!! This is intended for testing purposes only.            !!!
    !!! Do not use in production builds.                       !!!
    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    ")
endif()


message(STATUS "Added Transceiver library")
