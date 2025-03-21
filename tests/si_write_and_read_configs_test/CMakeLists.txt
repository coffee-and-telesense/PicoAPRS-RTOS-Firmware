add_executable(test_si_configs
    test_si_configs.c
)

target_sources(test_si_configs PRIVATE
        test_si_configs.c
)

target_link_libraries(test_si_configs PRIVATE
    stm32cubemx
)