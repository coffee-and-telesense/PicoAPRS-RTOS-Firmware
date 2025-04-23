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
#include "si4463_cmd_maps.h"
#include "radio_config.h"
#include "si4463_patch.h"

//=============================================================================
// Configuration Options
//=============================================================================
// Comment out to use polling method instead of GPIO pin for CTS
//#define SI4463_USE_GPIO_CTS

#define BUFFER_SIZE 256

// Command definitions
#define CMD_READ_CMD_BUFF       0x44
#define CMD_GET_PH_STATUS       0x21
#define CMD_GET_CHIP_STATUS     0x23

#define CMD_FIFO_INFO           0x15
#define CMD_WRITE_TX_FIFO       0x66
#define CMD_START_TX                0x31
#define CMD_REQUEST_DEVICE_STATE    0x33
#define CMD_PART_INFO               0x01

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

// Function pointer typedefs using STM32 HAL types
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

    /* GPIO Shutdown Pin SDN (For performing Power on Reset)*/
    GPIO_TypeDef* sdn_port;
    uint16_t sdn_pin;

#ifdef SI4463_USE_GPIO_CTS
    /* GPIO Clear To Send Pin (CTS) (only used when SI4463_USE_GPIO_CTS is defined) */
    GPIO_TypeDef* cts_port;
    uint16_t cts_pin;
#endif
} si4463_init_t;

typedef enum {
    SI4463_MODE_IDLE,
    SI4463_MODE_TX,
    SI4463_MODE_RX
} si4463_trx_mode_t;

typedef struct {
    // Pointer to place in buffer to start from
    uint8_t index;
    si4463_trx_mode_t mode; // Current mode (TX, RX, or IDLE)
} trx_info_t;

/**
 * @brief Device structure for SI4463 RF Transceiver
 */
typedef struct {
    uint8_t initialized;
    si4463_init_t config;
    uint8_t radio_tx_buffer[BUFFER_SIZE];  // Command/TX buffer
    uint8_t radio_rx_buffer[BUFFER_SIZE];  // Response/RX buffer
    trx_info_t trx_info;                   // Transmit/Receive state tracking
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
 * @brief Get the current FIFO status
 *
 * @param dev Pointer to SI4463 device structure
 * @param fifoInfo Pointer to structure to store FIFO information
 * @return si4463_status_t Status code
 */
si4463_status_t si4463_get_fifo_info(si4463_dev_t *dev, struct si446x_reply_FIFO_INFO_map *fifoInfo, uint8_t reset_tx_fifo, uint8_t reset_rx_fifo);




si4463_status_t si4463_write_tx_fifo(si4463_dev_t *dev, uint8_t *data, uint8_t len, uint8_t reset_tx_fifo);


si4463_status_t si4463_start_tx(si4463_dev_t *dev, uint8_t conditions, uint16_t tx_len, uint8_t tx_delay, uint8_t num_repeat);

si4463_status_t si4463_get_device_state(si4463_dev_t *dev, uint8_t *curr_state, uint8_t *current_channel);

/**
 * @brief Get part information from the SI4463 device
 *
 * Returns Part Number, Part Version, ROM ID, etc.
 *
 * @param dev Pointer to SI4463 device structure
 * @param part_info Pointer to store part information
 * @return si4463_status_t Status code
 */
si4463_status_t si4463_get_part_info(si4463_dev_t *dev, struct si446x_reply_PART_INFO_map *part_info);


si4463_status_t si4463_transmit_packet(si4463_dev_t *dev, uint8_t *data, uint16_t len);

si4463_status_t si4463_irq_pkt_handler(si4463_dev_t *dev);


/*TODO: Delete me*/
uint8_t check_command_error(si4463_dev_t *dev);

/*TODO: Delete me*/
uint8_t check_pkt_handler_status(si4463_dev_t *dev);
