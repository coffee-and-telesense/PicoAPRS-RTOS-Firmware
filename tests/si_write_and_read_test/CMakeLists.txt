add_executable(test_rw
    test_si_read_write.c
)

target_sources(test_rw PRIVATE
        test_si_read_write.c
)

target_link_libraries(test_rw PRIVATE
    stm32cubemx
)