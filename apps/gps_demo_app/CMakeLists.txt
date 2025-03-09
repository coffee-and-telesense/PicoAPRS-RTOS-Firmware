add_executable(gps_demo_app app.c)

target_sources(gps_demo_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/gps_demo.c
)

target_include_directories(gps_demo_app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(gps_demo_app PRIVATE 
    stm32cubemx
    gps
)