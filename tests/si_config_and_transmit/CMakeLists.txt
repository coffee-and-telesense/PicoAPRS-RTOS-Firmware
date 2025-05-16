add_executable(si_config_and_transmit test_si.c)

target_sources(si_config_and_transmit PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/test_si.c
)

target_link_libraries(si_config_and_transmit PRIVATE 
    stm32cubemx
    transceiver
    common
)