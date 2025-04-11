add_executable(bme68x_test test_bme68x.c)

target_sources(bme68x_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/test_bme68x.c
)

target_link_libraries(bme68x_test PRIVATE 
    stm32cubemx
    bme68x
)
