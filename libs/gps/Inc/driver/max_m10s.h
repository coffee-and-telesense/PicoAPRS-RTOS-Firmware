#pragma once
/**
 * @file max_m10s.h
 * @brief Driver for the MAX-M10 GPS module intended for use as a GPS beacon. Configurations 
 * and API calls are minimized to limit the scope of the driver to the basic functionality
 * required for the Pico Ballon project.  
 */
#include <stdint.h>
#include "gps_types.h"
#include "ubx.h"
#include "i2c.h"
#include "tim.h"



// Generic status return type (0 = OK, Otherwise = Error)
typedef uint32_t status_t;

/* Function pointer typedefs using direct STM32 HAL types */
typedef HAL_StatusTypeDef (*i2c_transmit_blocking_fn)(I2C_HandleTypeDef* hi2c, uint16_t addr,
                                                    uint8_t *data, uint16_t size, uint32_t timeout);

typedef HAL_StatusTypeDef (*i2c_receive_blocking_fn)(I2C_HandleTypeDef* hi2c, uint16_t addr,
                                                    uint8_t *data, uint16_t size, uint32_t timeout);

typedef HAL_StatusTypeDef (*i2c_transmit_it_fn)(I2C_HandleTypeDef* hi2c, uint16_t addr,
                                                uint8_t *data, uint16_t size);

typedef HAL_StatusTypeDef (*i2c_receive_it_fn)(I2C_HandleTypeDef* hi2c, uint16_t addr,
                        uint8_t *data, uint16_t size);

typedef HAL_StatusTypeDef (*delay_it_fn)(TIM_HandleTypeDef *htim, uint32_t delay_ms,
                                void (*callback)(void*), void* context);

/**
 * @brief Configuration structure for the MAX-M10S GPS driver. This structure is used to
 *       initialize the driver with the required function pointers and configurations.
 * 
 * @example Here's how to initialize this driver with the required function pointers using stm32 hal 
 *        functions.
 * 
 * max_m10s_init_t gps_init = {
 *   .hi2c = &hi2c1,
 *   .htim = &htim6,
 *   .device_address = 0x42,
 *   .transmit_blocking = HAL_I2C_Master_Transmit,
 *   .receive_blocking = HAL_I2C_Master_Receive,
 *   .transmit_it = HAL_I2C_Master_Transmit_IT,
 *   .receive_it = HAL_I2C_Master_Receive_IT,
 *   .delay_it = my_delay_it_function,
 *   .timeout_ms = 1000,
 *   .use_interrupts = 0
};
 *  @note: Driver comes with a protocol layer for UBX, this init structure could easily be extended 
 *         to include the protocol layer configuration if NMEA is needed. 
 */
typedef struct {
    void* hi2c; 
    void* htim; 

    /* Function Pointers*/
    i2c_transmit_blocking_fn transmit_blocking;
    i2c_receive_blocking_fn receive_blocking;
    i2c_transmit_it_fn transmit_it;
    i2c_receive_it_fn receive_it;
    delay_it_fn delay_it;
    /* Basic Configurations*/
    uint8_t device_address; 
    uint32_t timeout_ms; 
    uint8_t use_interrupts;   // 0 for blocking, 1 for non-blocking
} max_m10s_init_s; 

typedef union {
    uint8_t all;  // Access all flags at once (reduced to 8 bits)
    struct {      // Event flags to indicate state changes
        uint8_t read_done : 1;
        uint8_t write_done : 1;
        uint8_t delay_done : 1;
        uint8_t read_error : 1;
        uint8_t write_error : 1; 
        uint8_t bus_error : 1;
        uint8_t data_ready : 1;    // New data available
        uint8_t needs_reset : 1;   // Reset required flag
    } bits;
} gps_flags_t;

/**
 * @brief Device structure for MAX-M10S GPS module that is created and co-managed by the driver
 *      and application. This structure is used to store device state and configuration. 
 * 
 * @example Here's how you would use hardware interfacing function to do bus ops
 * status_t result = dev->config.transmit_blocking(
 *   dev->config.hi2c,
 *   dev->config.device_address,
 *   dev->tx_buffer,
 *   dev->tx_size,
 *   dev->config.timeout_ms
 *);
 * 
 */
typedef struct {
    uint8_t initialized;
    gps_flags_t flags;
    max_m10s_init_s configs;
    uint8_t tx_buffer[256];
    uint8_t rx_buffer[256];
    uint16_t tx_size; 
    uint16_t rx_size;
} max_m10s_dev_s;


// Initialize the MAX-M10S GPS driver
gps_status_e max_m10s_init(max_m10s_dev_s *dev, const max_m10s_init_s *init);

