/**
 * @file si4463.c
 */
#include "si4463.h"
#include "radio_config_si4463.h"
#include "si4463_cmd_maps.h"


// Forward declarations
si4463_status_t radio_config_init(si4463_dev_t *dev);
uint8_t check_command_error(si4463_dev_t *dev);
si4463_status_t radio_comm_sendcmd_getresp(si4463_dev_t *dev, uint8_t cmd, uint8_t *cmd_args, uint8_t *resp_buf, uint8_t trx_len);

/**
 * @brief Check if Clear-To-Send signal is active
 * 
 * Uses either GPIO pin or SPI polling based on configuration
 * 
 * Note: TODO: If passing cmd_args, caller must understand only the first two bytes are
 *       used for the CTS check. Read the documentation on polling CTS to understand 
 *       why this is the case.
 * 
 * TODO: The changing between GPIO and SPI is not ideal, we should probably
 *       have a single method we use. Keeping for now since we don't know how 
 *       many pins we have available on the MCU or which is the best way to do this. 
 *      Also, some commands require CTS within the response stream, and some don't so 
 *      it seems we will need to be able to poll for CTS in some cases regardless 
 *      of GPIO use. 
 */
uint8_t si4463_check_cts(si4463_dev_t *dev)
{
#ifdef SI4463_USE_GPIO_CTS
    // Use GPIO pin for CTS
    GPIO_PinState state = HAL_GPIO_ReadPin(dev->config.cts_port, dev->config.cts_pin);
    return (state == GPIO_PIN_SET) ? 1 : 0;
#else

    HAL_StatusTypeDef status;
    uint8_t tx_buffer[2] = {CMD_READ_CMD_BUFF, 0};
    uint8_t rx_buffer[2] = {0, 0};
    
    SI4463_CS_LOW(dev);
    status = dev->config.transmit_receive(dev->config.hspi, tx_buffer, rx_buffer, 2, 100);
    SI4463_CS_HIGH(dev);
    
    if (status != HAL_OK) {
        return 0;  // Error in communication
    }
    
    // The second byte received contains the CTS value
    return (rx_buffer[1] == 0xFF) ? 1 : 0;
#endif
}

/**
 * @brief Wait for Clear-To-Send signal with timeout
 */
si4463_status_t si4463_wait_for_cts(si4463_dev_t *dev, uint16_t timeout)
{
    uint32_t start_tick = HAL_GetTick();
    
    while (HAL_GetTick() - start_tick < timeout) {
        if (si4463_check_cts(dev)) {
            return SI4463_STATUS_SUCCESS;  // CTS is high
        }
        dev->config.delay_blocking(1);  // Small delay before checking again
    }
    
    return SI4463_STATUS_TIMEOUT;  // Timeout waiting for CTS
}

/**
 * @brief Initialize the SI4463 RF Transceiver
 */
si4463_status_t si4463_init(si4463_dev_t *dev, const si4463_init_t *init)
{
    // Copy configuration
    dev->config = *init;
    // Set CS pin high (inactive) initially
    SI4463_CS_HIGH(dev);

    // Initialize device state
    dev->initialized = 1;

    // add dev delay for device to power up
    dev->config.delay_blocking(10000); // 10 seconds delay for power up

    // Initialize the radio with configured settings
    if(radio_config_init(dev) != SI4463_STATUS_SUCCESS) {
        dev->initialized = 0; // Mark device as uninitialized
        return SI4463_STATUS_ERROR; // Error in radio configuration
    }

    return SI4463_STATUS_SUCCESS;
}


/**
 * @brief Checks the interrupt cmd error status of the Chip Interrupt Group
 * 
 * @note: CTS should be checked before calling this function, also this function 
 *          is not responsible for clearing Interrupt flags
 * 
 * @param dev 
 * @return 0x00 if no error, otherwise the command ID that caused the error
 */
uint8_t check_command_error(si4463_dev_t *dev)
{
    uint8_t hal_status; 
    uint8_t cmd_buff[2] = {CMD_GET_CHIP_STATUS, 0xFF}; // 0xFF doesn't clear interrupt flags
                                                       // i.e. NOP

    // Send command to get chip status
    SI4463_CS_LOW(dev);
    hal_status = dev->config.transmit(dev->config.hspi, cmd_buff, 2, 100);
    SI4463_CS_HIGH(dev);

    if (hal_status != HAL_OK) {
        return SI4463_STATUS_ERROR; // Error in communication
    }

    uint8_t tx_buffer[6] = {CMD_READ_CMD_BUFF, 0, 0, 0, 0, 0};
    uint8_t reply_buffer[6] = {0}; 

    // Wait for CTS
    if (si4463_wait_for_cts(dev, 100) != SI4463_STATUS_SUCCESS) {
        return SI4463_STATUS_TIMEOUT; // Timeout waiting for CTS
    }

    // Check for command error
    SI4463_CS_LOW(dev);
    hal_status = dev->config.transmit_receive(dev->config.hspi, tx_buffer, reply_buffer, 6, 100);
    SI4463_CS_HIGH(dev);

    if (hal_status != HAL_OK) {
        return SI4463_STATUS_ERROR; // Error in communication
    }

    struct si446x_reply_GET_CHIP_STATUS_map *reply = (struct si446x_reply_GET_CHIP_STATUS_map *)&reply_buffer[2];
    if(reply->CMD_ERR_STATUS != 0) {
        return reply->CMD_ERR_CMD_ID;
    }
    return 0;
}


/**
 * @brief Send a command to the SI4463 device that also expects a response
 * 
 * @note This function uses the SPI transmit/receive function so the length (trx_len) should be the combined size of the command and response

si4463_status_t radio_comm_sendcmd_getresp(si4463_dev_t *dev, uint8_t cmd, uint8_t *cmd_args, uint8_t *resp_buf, uint8_t trx_len)
{
    HAL_StatusTypeDef hal_status;

    // Per STM32 docs, transmit and receive buffers should be the combined size of the command and response
    uint8_t tx_buffer[trx_len]; // Command buffer
    uint8_t rx_buffer[trx_len]; // Response buffer

    memset(tx_buffer, 0, trx_len);
    memset(rx_buffer, 0, trx_len);
    
    // Fill command buffer
    tx_buffer[0] = cmd;
    if (cmd_args != NULL) {
        // TODO: If using cmd_args we should make sure they don't exceed size of buff
        memcpy(&tx_buffer[1], cmd_args, trx_len - 1);
    }
    
    // Send command
    SI4463_CS_LOW(dev);
    hal_status = dev->config.transmit_receive(dev->config.hspi, tx_buffer, rx_buffer, trx_len, 100);
    SI4463_CS_HIGH(dev);
    
    if (hal_status != HAL_OK) {
        return SI4463_STATUS_ERROR;
    }
    
    // Copy response data
    if (resp_buf != NULL) {
        // TODO: This might be an issue, how big is the buffers and the response_len
        memcpy(resp_buf, rx_buffer, trx_len);
    }
    
    return SI4463_STATUS_SUCCESS;
}
*/

/**
 * @brief Initialize radio with configuration data from array
 * 
 * @param dev Pointer to SI4463 device structure
 * @return si4463_status_t SI4463_STATUS_SUCCESS if successful, otherwise error code 
 */
si4463_status_t radio_config_init(si4463_dev_t *dev)
{
    // Radio configuration data array defined in radio_config.h
    static const uint8_t radio_config_data[] = RADIO_CONFIGURATION_DATA_ARRAY;
    uint16_t index = 0;
    si4463_status_t status;

    // Maximum number of retries for a command
    const uint8_t MAX_RETRIES = 3;
    
    // Iterate over all commands in the array
    // Encountering a 0 length byte indicates end of configuration data
    while (radio_config_data[index] != 0) {
        
        uint8_t cmd_len = radio_config_data[index];
        uint8_t retry_count = 0; 
        uint8_t cmd_success = 0; // Flag to indicate successful command
        while(retry_count < MAX_RETRIES && !cmd_success) {
            // Wait for CTS each time
            if(si4463_wait_for_cts(dev, dev->config.timeout_ms) != SI4463_STATUS_SUCCESS) {
                return SI4463_STATUS_TIMEOUT; // Fatal timeout waiting for CTS 
            }
            // Send command to the radio
            SI4463_CS_LOW(dev);
            HAL_StatusTypeDef hal_status = dev->config.transmit(
                dev->config.hspi, 
                (uint8_t*)&radio_config_data[index + 1], // Skip the length byte
                cmd_len, 
                100
            );

            SI4463_CS_HIGH(dev);
            
            if (hal_status != HAL_OK) {
                return SI4463_STATUS_ERROR;
            }
            
            // Wait for CTS signal
            status = si4463_wait_for_cts(dev, dev->config.timeout_ms);
            if (status != SI4463_STATUS_SUCCESS) {
                return status; // Return CTS timeout error
            }
            
            // Check for chip errors after each command
            uint8_t error_cmd = check_command_error(dev);
            if(error_cmd != 0){
                // Command error occured
                #ifdef SI_DEBUG
                    dev->last_error_cmd = error_cmd;
                    dev->error_cmd_count++;
                #endif
                retry_count++;
                continue;
            }
            cmd_success = 1;
        }
            
        // Move to next command
        index += cmd_len + 1; // +1 for the length byte
    }
    return SI4463_STATUS_SUCCESS;
}




