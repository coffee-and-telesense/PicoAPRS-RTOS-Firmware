add_executable(uart_echo_app uart_echo_app.c)

target_include_directories(uart_echo_app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(uart_echo_app PRIVATE 
stm32cubemx
)