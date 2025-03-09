#include "max_m10s.h"

//forward declaration
static void max_m10s_delay_complete_cb(void* context);


/* Function to initialize the GPS device */
gps_status_e max_m10s_init(max_m10s_dev_s *dev, const max_m10s_init_s *init)
{
    gps_status_e result = UBLOX_OK; 
    uint16_t msg_size; // Size of the config message to be sent
    // Validate parameters
    if (dev == NULL || init == NULL || init->hi2c == NULL || init->htim == NULL) {
        return UBLOX_INVALID_PARAM; // Error status code (avoiding HAL dependency)
        // Error introspection can be done by checking error codes of dev struct state
    }
    
    // Check that all required function pointers are provided
    if (init->transmit_blocking == NULL || 
        init->receive_blocking == NULL ||
        init->delay_it == NULL) {
        return UBLOX_INVALID_PARAM; // Error status code
        // Error introspection can be done by checking error codes of dev struct state
    }
    
    // Copy the initialization structure
    dev->configs = *init;
    // Adjust address to 7-bit format
    dev->configs.device_address = (init->device_address << 1);
    
    // Initialize device state
    dev->initialized = 0;
    dev->flags.all = 0;
    dev->tx_size = 0;
    dev->rx_size = 0;
    
    // Step 1: Initialize UBX protocol for I2C communication
    msg_size = ubx_prepare_config_cmd(dev->tx_buffer, UBX_CFG_I2C_UBX_ENABLE, 1);
    dev->tx_size = msg_size;

    HAL_StatusTypeDef hal_status;

    hal_status = dev->configs.transmit_blocking(
        dev->configs.hi2c,
        dev->configs.device_address,
        dev->tx_buffer,
        dev->tx_size,
        dev->configs.timeout_ms
    );

    if(hal_status != HAL_OK) {
        return UBLOX_ERROR; // Error status code
    }
    // Step 2: Ensure delay time is completed, this requires one second delay, I'm not sure why. 
    hal_status = dev->configs.delay_it(
        dev->configs.htim, 
        1000,  // 1 second delay
        max_m10s_delay_complete_cb,
        dev
    );


    while(!dev->flags.bits.delay_done) {
        // Wait for delay to complete, delay complete code should set delay to done
    }
    dev->flags.bits.delay_done = 0; // Reset delay flag
    // Step 3: Check for ACK response
    hal_status = dev->configs.receive_blocking(
        dev->configs.hi2c,
        dev->configs.device_address,
        dev->rx_buffer,
        10, // Expecting 10 bytes for ACK response
        dev->configs.timeout_ms
    );
    
    // Step 4: Disable NMEA output
    msg_size = ubx_prepare_config_cmd(dev->tx_buffer, UBX_CFG_I2C_NMEA_DISABLE, 0);
    dev->tx_size = msg_size;
    dev->configs.transmit_blocking(
        dev->configs.hi2c,
        dev->configs.device_address,
        dev->tx_buffer,
        dev->tx_size,
        dev->configs.timeout_ms
    );

    // Step 5: Ensure delay time is completed
    hal_status = dev->configs.delay_it(
        dev->configs.htim, 
        1000,  // 1 second delay
        max_m10s_delay_complete_cb,
        dev
    );
    
    while(!dev->flags.bits.delay_done) {
        // Wait for delay to complete
    }
    dev->flags.bits.delay_done = 0; // Reset delay flag
    // Step 6: Check for ACK response
    hal_status = dev->configs.receive_blocking(
        dev->configs.hi2c,
        dev->configs.device_address,
        dev->rx_buffer,
        10, // Expecting 10 bytes for ACK response
        dev->configs.timeout_ms
    );

    dev->initialized = true;
    
    return 0x00; // Success status code
}


// Callback that will be called by the timer when delay completes
static void max_m10s_delay_complete_cb(void* context) {
    max_m10s_dev_s* dev = (max_m10s_dev_s*)context;
    if (dev) {
        dev->flags.bits.delay_done = 1;
    }
}


