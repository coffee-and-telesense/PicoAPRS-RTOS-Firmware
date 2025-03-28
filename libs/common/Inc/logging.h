/*******************************************************************************
 * @file: logging.h
 * @brief: Simple debug printing utility for STM32 projects using printf redirection
 *
 * @note: Requires UART to be initialized before use
 ******************************************************************************/
#pragma once

#include "stm32u0xx_hal.h"
#include "usart.h"
#include <stdio.h>

// Define printf redirection prototype
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)

// UART handle to be registered by the application
extern UART_HandleTypeDef *debug_uart_handle;

// Function to initialize the debug logger with a specific UART handle
void debug_init(UART_HandleTypeDef *huart);

// Debug logging macros
#ifdef DEBUG
    #include <string.h>
    #include <stdarg.h>
    
    // Main debug print macro that will be active in DEBUG builds
    #define DEBUG_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
    
    // Additional debug macros with log levels if needed
    #define DEBUG_INFO(fmt, ...)  printf("[INFO] " fmt, ##__VA_ARGS__)
    #define DEBUG_WARN(fmt, ...)  printf("[WARN] " fmt, ##__VA_ARGS__)
    #define DEBUG_ERROR(fmt, ...) printf("[ERROR] " fmt, ##__VA_ARGS__)

    // Conditional debug printing (only prints if condition is true)
    #define DEBUG_IF(cond, fmt, ...) do { if (cond) printf(fmt, ##__VA_ARGS__); } while(0)
    
    // Debug printing with function name and line number information
    #define DEBUG_TRACE(fmt, ...) printf("[%s:%d] " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
    
#else
    // Empty macros when DEBUG is not defined
    #define DEBUG_PRINT(fmt, ...) ((void)0)
    #define DEBUG_INFO(fmt, ...)  ((void)0)
    #define DEBUG_WARN(fmt, ...)  ((void)0)
    #define DEBUG_ERROR(fmt, ...) ((void)0)
    #define debug_print(fmt, ...) ((void)0)
#endif

// Debug logger initialization function is always defined
// but only does something when DEBUG is defined
void debug_init(UART_HandleTypeDef *huart);

// *********************Example usage:************************************************
/**
void initialize_sensors(void) {
    DEBUG_TRACE("Starting sensor initialization\r\n");
    // Function body...
}
// Output: 
// [initialize_sensors:3] Starting sensor initialization

void read_sensor_data(void) {
    DEBUG_INFO("Reading sensor data\r\n");
    // Function body...
}
// Output:
// [INFO] Reading sensor data


DEBUG_IF(temperature > 30, "Temperature alert: %d°C\r\n", temperature);
// Output: (Only if temperature > 30)
// Temperature alert: 35°C 


**********************************************************************************************/