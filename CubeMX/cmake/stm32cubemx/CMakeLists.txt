cmake_minimum_required(VERSION 3.22)
# Enable CMake support for ASM and C languages
enable_language(C ASM)
# STM32CubeMX generated symbols (macros)
set(MX_Defines_Syms 
	TX_INCLUDE_USER_DEFINE_FILE 
	TX_SINGLE_MODE_NON_SECURE=1 
	USE_HAL_DRIVER 
	STM32U083xx
    $<$<CONFIG:Debug>:DEBUG>
)

# STM32CubeMX generated include paths
set(MX_Include_Dirs
    ${CMAKE_SOURCE_DIR}/Inc
    ${CMAKE_SOURCE_DIR}/Drivers/STM32U0xx_HAL_Driver/Inc
    ${CMAKE_SOURCE_DIR}/Drivers/STM32U0xx_HAL_Driver/Inc/Legacy
    ${CMAKE_SOURCE_DIR}/Middlewares/ST/threadx/common/inc
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32U0xx/Include
    ${CMAKE_SOURCE_DIR}/Middlewares/ST/threadx/ports/cortex_m0/gnu/inc
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Include
)

# STM32CubeMX generated application sources
set(MX_Application_Src
    ${CMAKE_SOURCE_DIR}/Src/tx_initialize_low_level.S
    ${CMAKE_SOURCE_DIR}/Src/main.c
    ${CMAKE_SOURCE_DIR}/Src/app_threadx.c
    ${CMAKE_SOURCE_DIR}/Src/app_azure_rtos.c
    ${CMAKE_SOURCE_DIR}/Src/stm32u0xx_it.c
    ${CMAKE_SOURCE_DIR}/Src/stm32u0xx_hal_msp.c
    ${CMAKE_SOURCE_DIR}/Src/stm32u0xx_hal_timebase_tim.c
    ${CMAKE_SOURCE_DIR}/Src/sysmem.c
    ${CMAKE_SOURCE_DIR}/Src/syscalls.c
    ${CMAKE_SOURCE_DIR}/startup_stm32u083xx.s
)

# STM32 HAL/LL Drivers
set(STM32_Drivers_Src
)

# Drivers Midllewares


set(ThreadX_Src
    ${CMAKE_SOURCE_DIR}/Middlewares/ST/threadx/ports/cortex_m0/gnu/src/tx_thread_context_restore.S
    ${CMAKE_SOURCE_DIR}/Middlewares/ST/threadx/ports/cortex_m0/gnu/src/tx_thread_context_save.S
    ${CMAKE_SOURCE_DIR}/Middlewares/ST/threadx/ports/cortex_m0/gnu/src/tx_thread_interrupt_control.S
    ${CMAKE_SOURCE_DIR}/Middlewares/ST/threadx/ports/cortex_m0/gnu/src/tx_thread_interrupt_disable.S
    ${CMAKE_SOURCE_DIR}/Middlewares/ST/threadx/ports/cortex_m0/gnu/src/tx_thread_interrupt_restore.S
    ${CMAKE_SOURCE_DIR}/Middlewares/ST/threadx/ports/cortex_m0/gnu/src/tx_thread_schedule.S
    ${CMAKE_SOURCE_DIR}/Middlewares/ST/threadx/ports/cortex_m0/gnu/src/tx_thread_stack_build.S
    ${CMAKE_SOURCE_DIR}/Middlewares/ST/threadx/ports/cortex_m0/gnu/src/tx_thread_system_return.S
    ${CMAKE_SOURCE_DIR}/Middlewares/ST/threadx/ports/cortex_m0/gnu/src/tx_timer_interrupt.S
)

# Link directories setup
set(MX_LINK_DIRS

)
# Project static libraries
set(MX_LINK_LIBS 
    STM32_Drivers
    ThreadX	
)
# Interface library for includes and symbols
add_library(stm32cubemx INTERFACE)
target_include_directories(stm32cubemx INTERFACE ${MX_Include_Dirs})
target_compile_definitions(stm32cubemx INTERFACE ${MX_Defines_Syms})

# Create STM32_Drivers static library
add_library(STM32_Drivers OBJECT)
target_sources(STM32_Drivers PRIVATE ${STM32_Drivers_Src})
target_link_libraries(STM32_Drivers PUBLIC stm32cubemx)


# Create ThreadX static library
add_library(ThreadX OBJECT)
target_sources(ThreadX PRIVATE ${ThreadX_Src})
target_link_libraries(ThreadX PUBLIC stm32cubemx)

# Add STM32CubeMX generated application sources to the project
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${MX_Application_Src})

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE ${MX_LINK_DIRS})

# Add libraries to the project
target_link_libraries(${CMAKE_PROJECT_NAME} ${MX_LINK_LIBS})

# Add the map file to the list of files to be removed with 'clean' target
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES ADDITIONAL_CLEAN_FILES ${CMAKE_PROJECT_NAME}.map)

# Validate that STM32CubeMX code is compatible with C standard
if((CMAKE_C_STANDARD EQUAL 90) OR (CMAKE_C_STANDARD EQUAL 99))
    message(ERROR "Generated code requires C11 or higher")
endif()
