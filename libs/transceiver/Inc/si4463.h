/**
 * @file si4463.h
 * @brief Driver for the SI4463 RF Transceiver with support for both polling and
 *        GPIO-based CTS (Clear To Send) signal handling.
 */
#pragma once
#include <stdint.h>
#include <string.h>
#include "spi.h"
#include "gpio.h"

//=============================================================================
// Configuration Options
//=============================================================================
// Comment out to use polling method instead of GPIO pin for CTS
//#define SI4463_USE_GPIO_CTS

// Command definitions
#define CMD_READ_CMD_BUFF       0x44
#define CMD_GET_CHIP_STATUS     0x23

//=============================================================================
// Type definitions
//=============================================================================

// Status enumeration
typedef enum {
    SI4463_STATUS_SUCCESS = 0x00,    // Operation completed successfully
    SI4463_STATUS_ERROR   = 0x01,    // General error occurred
    SI4463_STATUS_TIMEOUT = 0x02,    // Operation timed out
    SI4463_STATUS_BUSY    = 0x03,    // Device is busy
    SI4463_STATUS_CTS_TIMEOUT = 0x04, // CTS signal timeout
    SI4463_STATUS_FIFO_ERROR = 0x05, // TX/RX FIFO error
    SI4463_STATUS_INVALID_PARAM = 0x06 // Invalid parameter provided
} si4463_status_t;

// Function pointer for blocking delay
typedef void (*delay_blocking_fn)(uint32_t delay);

// Function pointer typedefs using direct STM32 HAL types
typedef HAL_StatusTypeDef (*spi_transmit)(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout);
typedef HAL_StatusTypeDef (*spi_receive)(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout);
typedef HAL_StatusTypeDef (*spi_transmit_receive)(SPI_HandleTypeDef *hspi, uint8_t *pTxData, uint8_t *pRxData, uint16_t Size, uint32_t Timeout);

/**
 * @brief Configuration structure for the SI4463 RF Transceiver driver.
 */
typedef struct {
    SPI_HandleTypeDef* hspi;
    
    /* Function Pointers */
    spi_transmit transmit;
    spi_receive receive;
    spi_transmit_receive transmit_receive;
    delay_blocking_fn delay_blocking;
    
    /* Basic Configurations */
    uint32_t timeout_ms;
    
    /* GPIO Chip Select Pin */
    GPIO_TypeDef* cs_port;
    uint16_t cs_pin;
    
#ifdef SI4463_USE_GPIO_CTS
    /* GPIO Clear To Send Pin (CTS) (only used when SI4463_USE_GPIO_CTS is defined) */
    GPIO_TypeDef* cts_port;
    uint16_t cts_pin;
#endif
} si4463_init_t;

/**
 * @brief Device structure for SI4463 RF Transceiver
 */
typedef struct {
    uint8_t initialized;
    si4463_init_t config;
    uint8_t radio_tx_buffer[256];  // Command/TX buffer
    uint8_t radio_rx_buffer[256];  // Response/RX buffer
    uint16_t tx_size;
    uint16_t rx_size;
    #ifdef SI_DEBUG
        uint8_t last_error_cmd; 
        uint8_t error_cmd_count; 
    #endif
} si4463_dev_t;

//=============================================================================
// Macros for internal use
//=============================================================================

// CS control macros
#define SI4463_CS_LOW(dev)  HAL_GPIO_WritePin((dev)->config.cs_port, (dev)->config.cs_pin, GPIO_PIN_RESET)
#define SI4463_CS_HIGH(dev) HAL_GPIO_WritePin((dev)->config.cs_port, (dev)->config.cs_pin, GPIO_PIN_SET)

//=============================================================================
// Public API Functions
//=============================================================================

/**
 * @brief Initialize the SI4463 RF Transceiver
 * 
 * @param dev Pointer to device structure to initialize
 * @param init Pointer to initialization configuration
 * @return si4463_status_t Status code
 */
si4463_status_t si4463_init(si4463_dev_t *dev, const si4463_init_t *init);

/**
 * @brief Check if Clear-To-Send signal is active
 * 
 * @param dev Pointer to SI4463 device structure
 * @return uint8_t 1 if CTS is high (ready), 0 if CTS is low (busy)
 */
uint8_t si4463_check_cts(si4463_dev_t *dev);

/**
 * @brief Wait for Clear-To-Send signal with timeout
 * 
 * @param dev Pointer to SI4463 device structure
 * @param timeout Maximum time to wait in milliseconds
 * @return si4463_status_t SI4463_STATUS_SUCCESS if CTS received, SI4463_STATUS_TIMEOUT on timeout
 */
si4463_status_t si4463_wait_for_cts(si4463_dev_t *dev, uint16_t timeout);

/**
 * @brief Send a command to the SI4463 device
 * 
 * @param dev Pointer to SI4463 device structure
 * @param cmd Command byte
 * @param data Pointer to command data (can be NULL if no data)
 * @param len Length of command data
 * @return si4463_status_t Status code
 */
si4463_status_t si4463_send_command(si4463_dev_t *dev, uint8_t cmd, uint8_t *data, uint8_t len);

/**
 * @brief Read response from the SI4463 device
 * 
 * @param dev Pointer to SI4463 device structure
 * @param data Pointer to buffer to store response data
 * @param max_len Maximum length of data to read
 * @param actual_len Pointer to store actual length read (can be NULL)
 * @return si4463_status_t Status code
 */
si4463_status_t si4463_read_response(si4463_dev_t *dev, uint8_t *data, uint8_t max_len, uint8_t *actual_len);