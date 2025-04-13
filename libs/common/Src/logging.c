/*******************************************************************************
 * @file: logging.c
 * @brief: Implementation of debug printing utility for STM32 projects
 ******************************************************************************/
#include "logging.h"

// UART handle storage
UART_HandleTypeDef *debug_uart_handle = NULL;

// Initialize the debug logger with a specific UART handle
void debug_init(UART_HandleTypeDef *huart) {
#ifdef DEBUG
    debug_uart_handle = huart;
    printf("Debug logger initialized\r\n");
#else
    // Prevent unused parameter warning
    (void)huart;
#endif
}

#ifdef DEBUG
// Implementation of printf redirection
PUTCHAR_PROTOTYPE {
    // Check if UART handle is initialized
    if (debug_uart_handle != NULL) {
        HAL_UART_Transmit(debug_uart_handle, (uint8_t *)&ch, 1, 0xFFFF);
    }
    return ch;
}
#endif