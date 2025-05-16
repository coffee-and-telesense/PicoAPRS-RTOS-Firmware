add_executable(test_si_aprs
    aprs_transmission.c
)

target_sources(test_si_aprs PRIVATE
        aprs_transmission.c
)

target_link_libraries(test_si_aprs PRIVATE
    stm32cubemx
    transceiver
    common
)
