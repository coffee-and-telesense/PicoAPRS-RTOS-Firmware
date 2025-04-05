/**
 * @file si4463.c
 */
#include "si4463.h"

//=============================================================================
// Private Function Prototypes
//=============================================================================
si4463_status_t radio_config_init(si4463_dev_t *dev);
si4463_status_t radio_apply_patch(si4463_dev_t *dev);
si4463_status_t radio_reset(uint16_t GPIO_SDN_Pin, GPIO_TypeDef* GPIOSDN_Port);
uint8_t check_command_error(si4463_dev_t *dev);
si4463_status_t radio_comm_sendcmd_getresp(si4463_dev_t *dev, uint8_t *cmd_data, uint8_t cmd_len, uint8_t *resp_buf, uint8_t resp_buf_len);
si4463_status_t radio_comm_send_cmd(si4463_dev_t *dev, uint8_t *data, uint8_t len);
si4463_status_t radio_comm_read_response(si4463_dev_t *dev, uint8_t *resp_buff, uint8_t resp_len);
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


// Private reset method, not part of the public API
// TODO: is this correct??? 
si4463_status_t radio_reset(uint16_t GPIO_SDN_Pin, GPIO_TypeDef* GPIOSDN_Port) {
    if(GPIOSDN_Port == NULL) {
        return SI4463_STATUS_INVALID_PARAM; // Invalid GPIO port
    }
    // Set SDN pin low to reset the device
    HAL_GPIO_WritePin(GPIOSDN_Port, GPIO_SDN_Pin, GPIO_PIN_SET);
    HAL_Delay(1); // Wait for 1ms
    HAL_GPIO_WritePin(GPIOSDN_Port, GPIO_SDN_Pin, GPIO_PIN_RESET);
    HAL_Delay(10); // Wait for 10ms for dev to power up again

    return SI4463_STATUS_SUCCESS; // Reset successful
}

/**
 * @brief Initialize the SI4463 RF Transceiver
 */
si4463_status_t si4463_init(si4463_dev_t *dev, const si4463_init_t *init)
{
    si4463_status_t status; // Add this line
    // Copy configuration
    dev->config = *init;
    // Set CS pin high (inactive) initially
    SI4463_CS_HIGH(dev);

    // Reset the device
    if (radio_reset(dev->config.sdn_pin, dev->config.sdn_port) != SI4463_STATUS_SUCCESS) {
        return SI4463_STATUS_ERROR; // Error in device reset
    }

    // Apply patch before power up command
    status = radio_apply_patch(dev);
    if (status != SI4463_STATUS_SUCCESS) {
        return status;
    }

    // Initialize device state
    dev->initialized = 1;

    // add dev delay for device to power up
    dev->config.delay_blocking(10); // 10 seconds delay for power up

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
 * @brief Send a command to the SI4463 device that also expects a response. Its essentially a wrapper
 *         around the send_cmd and read_response functions.
 * 
 */
si4463_status_t radio_comm_sendcmd_getresp(si4463_dev_t *dev, uint8_t *cmd_data, uint8_t cmd_len, uint8_t *resp_buf, uint8_t resp_buf_len)
{
    // Check for valid parameters
    if (dev == NULL || cmd_data == NULL || resp_buf == NULL) {
        return SI4463_STATUS_INVALID_PARAM;
    }

    // Wait for cts
    if(si4463_wait_for_cts(dev, dev->config.timeout_ms) != SI4463_STATUS_SUCCESS) {
        return SI4463_STATUS_TIMEOUT; // Fatal timeout waiting for CTS 
    }
    // Send command to the radio
    if(radio_comm_send_cmd(dev, cmd_data, cmd_len) != SI4463_STATUS_SUCCESS) {
        return SI4463_STATUS_ERROR;
    }

    // Wait for CTS signal
    if(si4463_wait_for_cts(dev, dev->config.timeout_ms) != SI4463_STATUS_SUCCESS) {
        return SI4463_STATUS_TIMEOUT; // Fatal timeout waiting for CTS 
    }
    // Read response from the radio, reply buffer is filled with the response
    // The caller is responsible for checking the response contents
    if(radio_comm_read_response(dev, resp_buf, resp_buf_len) != SI4463_STATUS_SUCCESS) {
        return SI4463_STATUS_ERROR; // Error in reading response
    }
    
    return SI4463_STATUS_SUCCESS;
}

/**
 * @brief Send a command to the SI4463 device
 * 
 * @param dev Pointer to SI4463 device structure
 * @param cmd Command byte
 * @param data Pointer to command data (can be NULL if no data)
 * @param len Length of command data
 * @return si4463_status_t Status code
 */
si4463_status_t radio_comm_send_cmd(si4463_dev_t *dev, uint8_t *data, uint8_t len) {
    // Check for valid parameters
    if (dev == NULL) {
        return SI4463_STATUS_INVALID_PARAM;
    }
    HAL_StatusTypeDef hal_status;
    // Send command to the radio
    SI4463_CS_LOW(dev);
    hal_status = dev->config.transmit(dev->config.hspi, data, len, 100);
    SI4463_CS_HIGH(dev);
    if (hal_status != HAL_OK) {
        return SI4463_STATUS_ERROR; // Error in communication
    }
    return SI4463_STATUS_SUCCESS;
}

/**
 * @brief Read response from the SI4463 device which is read from the command buffer. Note that not all
 *          commands will have a response stream.    
 * 
 * @param dev: Pointer to SI4463 device structure
 * @param data: Pointer to buffer to store response data
 * @param resp_length: Length of the response, which should be exclusive of the command and cts bytes
 *          (i.e. the length of the actual response data) since this is handled internally.
 * @return si4463_status_t: Status code OK or error
 */
si4463_status_t radio_comm_read_response(si4463_dev_t *dev, uint8_t *resp_buff, uint8_t resp_len){
    uint8_t hal_status;
    uint8_t cmd_buff[resp_len + 2];     // +2 for command and cts 
    uint8_t temp_resp_buff[resp_len + 2]; // Temporary buffer for response

    cmd_buff[0] = CMD_READ_CMD_BUFF;    // Command to read command buffer
    memset(&cmd_buff[1], 0x00, resp_len + 1); // Fill with 0x00 (i.e. NOP)

    SI4463_CS_LOW(dev);
    hal_status = dev->config.transmit_receive(dev->config.hspi, cmd_buff, temp_resp_buff, resp_len + 2, 100);
    SI4463_CS_HIGH(dev);
    if (hal_status != HAL_OK) {
        return SI4463_STATUS_ERROR; // Error in communication
    }

    if(temp_resp_buff[1] != 0xFF) {
        return SI4463_STATUS_ERROR; // CTS not received, therefore data is invalid
    }
    //Copy the response data to the provided buffer
    memcpy(resp_buff, &temp_resp_buff[2], resp_len); // Skip the first two bytes (command and CTS)

    return SI4463_STATUS_SUCCESS;
}


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


/**
 * @brief Get the current FIFO buffer info status (i.e. tx space and rx count)
 *
 * @param dev Pointer to SI4463 device structure
 * @param fifoInfo Pointer to structure to store FIFO information
 * @param reset_tx_fifo Flag to reset TX FIFO (1 to reset, 0 to not reset) (useful for resetting at beginning of new tx)
 * @param reset_rx_fifo Flag to reset RX FIFO (1 to reset, 0 to not reset) (useful for resetting at end of current rx)
 * @return si4463_status_t Status code
 */
si4463_status_t si4463_get_fifo_info(si4463_dev_t *dev, struct si446x_reply_FIFO_INFO_map *fifoInfo, uint8_t reset_tx_fifo, uint8_t reset_rx_fifo) {
    uint8_t cmd_buff[2];
    union si446x_cmd_reply_union reply;
    si4463_status_t status;
    
    if (dev == NULL || fifoInfo == NULL) {
        return SI4463_STATUS_INVALID_PARAM;
    }
    
    cmd_buff[0] = CMD_FIFO_INFO;  /* Command code for FIFO_INFO */
    // Don't reset == 0x00, Reset TX FIFO == 0x01, Reset RX FIFO == 0x02, Reset both == 0x03
    cmd_buff[1] = (uint8_t)((reset_tx_fifo ? 1U : 0U) | ((reset_rx_fifo ? 1U : 0U) << 1));
    
    status = radio_comm_sendcmd_getresp(dev, cmd_buff, sizeof(cmd_buff), (uint8_t *)&reply, sizeof(reply.FIFO_INFO));

    if (status != SI4463_STATUS_SUCCESS) {
        return status; // Error in sending command
    }
    
    // Response is in the reply buffer, copy it to the provided structure
    memcpy(fifoInfo, &reply.FIFO_INFO, sizeof(reply.FIFO_INFO));
    
    return SI4463_STATUS_SUCCESS;
}


/**
 * @brief Write data to the TX FIFO
 * 
 * @param dev Pointer to SI4463 device structure
 * @param data Pointer to data to write
 * @param len Length of data
 * @return si4463_status_t Status code
 */
si4463_status_t si4463_write_tx_fifo(si4463_dev_t *dev, uint8_t *data, uint8_t len, uint8_t reset_tx_fifo) {
    si4463_status_t status;
    // Check for valid parameters
    if (dev == NULL || data == NULL || len == 0) {
        return SI4463_STATUS_INVALID_PARAM;
    }
    if (len > 64) {
        return SI4463_STATUS_INVALID_PARAM;  // Max TX FIFO size is 64 bytes
                                             // Only 64 bytes can be written at one time
    }
    // Check if FIFO needs to be reset
    if (reset_tx_fifo) {
        struct si446x_reply_FIFO_INFO_map fifoInfo;
        status = si4463_get_fifo_info(dev, &fifoInfo, (uint8_t) 1,(uint8_t) 0); // Reset TX FIFO
        if (status != SI4463_STATUS_SUCCESS) {
            return status;  // Error in getting FIFO info
        }
    }

    // Create command buffer
    uint8_t cmd_buff[65];  // 1 byte for command + 64 bytes for data
    cmd_buff[0] = CMD_WRITE_TX_FIFO;  // Command code for writing to TX FIFO
    memcpy(&cmd_buff[1], data, len);  // Copy data to command buffer
    
    // Wait for CTS before sending command
    if (si4463_wait_for_cts(dev, dev->config.timeout_ms) != SI4463_STATUS_SUCCESS) {
        return SI4463_STATUS_TIMEOUT;  // Fatal timeout waiting for CTS 
    }

    status = radio_comm_send_cmd(dev, cmd_buff, len + 1);

    if (status != SI4463_STATUS_SUCCESS) {
        return status;  // Error in sending command
    }
    
    return SI4463_STATUS_SUCCESS;
}

si4463_status_t si4463_start_tx(si4463_dev_t *dev, 
                                uint8_t conditions, 
                                uint16_t tx_len, 
                                uint8_t tx_delay, 
                                uint8_t num_repeat) 
{
    uint8_t cmd_buff[7] = {0}; // // Command buffer for START_TX command

    // Check for valid parameters
    if (dev == NULL || tx_len == 0 || tx_len > 8192) {
        return SI4463_STATUS_INVALID_PARAM;
    }

    uint8_t tx_len_upper = (uint8_t)(tx_len >> 8); // Upper byte of tx_len
    uint8_t tx_len_lower = (uint8_t)(tx_len & 0xFF); // Lower byte of tx_len

    cmd_buff[0] = CMD_START_TX;  
    cmd_buff[1] = RADIO_CONFIGURATION_DATA_CHANNEL_NUMBER;
    cmd_buff[2] = conditions;  
    cmd_buff[3] = tx_len_upper;  
    cmd_buff[4] = tx_len_lower;  
    cmd_buff[5] = tx_delay;
    cmd_buff[6] = num_repeat;  

    // Wait for CTS before sending command
    if (si4463_wait_for_cts(dev, dev->config.timeout_ms) != SI4463_STATUS_SUCCESS) {
        return SI4463_STATUS_TIMEOUT;  // Fatal timeout waiting for CTS 
    }
    // Send command to start transmission
    si4463_status_t status = radio_comm_send_cmd(dev, cmd_buff, sizeof(cmd_buff));

    if (status != SI4463_STATUS_SUCCESS) {
        return status;  // Error in sending command
    }
    return SI4463_STATUS_SUCCESS;
}


/**
 * @brief Get the current device state
 * 
 * @param dev Pointer to SI4463 device structure
 * @param curr_state Pointer to store the current state value
 * @param current_channel Pointer to store the current channel (can be NULL if not needed)
 * @return si4463_status_t Status code
 */
si4463_status_t si4463_get_device_state(si4463_dev_t *dev, uint8_t *curr_state, uint8_t *current_channel)
{
    uint8_t cmd_buff[1] = {CMD_REQUEST_DEVICE_STATE};
    union si446x_cmd_reply_union reply; 

    si4463_status_t status;
    
    // Check for valid parameters
    if (dev == NULL || curr_state == NULL) {
        return SI4463_STATUS_INVALID_PARAM;
    }
    
    // Send command to request device state and get response
    status = radio_comm_sendcmd_getresp(dev, cmd_buff, sizeof(cmd_buff), 
                                        (uint8_t *)&reply, sizeof(reply.REQUEST_DEVICE_STATE));
    
    if (status != SI4463_STATUS_SUCCESS) {
        return status; // Error in sending command or receiving response
    }
    
    // Extract current state from response
    *curr_state = reply.REQUEST_DEVICE_STATE.CURR_STATE;
    
    // If current_channel pointer is provided, store the channel value
    if (current_channel != NULL) {
        *current_channel = reply.REQUEST_DEVICE_STATE.CURRENT_CHANNEL; // CURRENT_CHANNEL is at index 2
    }
    
    return SI4463_STATUS_SUCCESS;
}

/**
 * @brief Apply patch commands to the SI4463 device
 * 
 * This function applies the patch to the SI4463 device, sending each 8-byte command
 * and waiting for CTS after each command. Must be called before POWER_UP command.
 * 
 * @param dev Pointer to SI4463 device structure
 * @return si4463_status_t Status code (SI4463_STATUS_SUCCESS if successful)
 */
si4463_status_t radio_apply_patch(si4463_dev_t *dev) {
    // Define the patch commands array from the macro
    static const uint8_t patch_data[] = SI446X_PATCH_CMDS;
    uint16_t index = 0;
    
    // Process each command until we reach the end (0x00)
    while (patch_data[index] != 0x00) {
        // Each command is 8 bytes, first byte is the length (should always be 0x08)
        if (patch_data[index] != 0x08) {
            // Invalid patch command format - this shouldn't happen with the predefined array
            return SI4463_STATUS_ERROR;
        }
        
        // Wait for CTS before sending the command
        if (si4463_wait_for_cts(dev, dev->config.timeout_ms) != SI4463_STATUS_SUCCESS) {
            return SI4463_STATUS_TIMEOUT;
        }
        
        // Send the 8-byte command (skip the length byte)
        SI4463_CS_LOW(dev);
        HAL_StatusTypeDef hal_status = dev->config.transmit(
            dev->config.hspi,
            (uint8_t*)&patch_data[index + 1], // Skip the length byte
            8, // Always 8 bytes
            100
        );
        SI4463_CS_HIGH(dev);
        
        if (hal_status != HAL_OK) {
            return SI4463_STATUS_ERROR;
        }
        
        // Move to the next command (9 bytes: 1 for length + 8 for command)
        index += 9;
    }
    
    return SI4463_STATUS_SUCCESS;
}

/**
 * @brief Get part information from the SI4463 device
 * 
 * Returns Part Number, Part Version, ROM ID, etc.
 * 
 * @param dev Pointer to SI4463 device structure
 * @param part_info Pointer to store part information
 * @return si4463_status_t Status code
 */
si4463_status_t si4463_get_part_info(si4463_dev_t *dev, struct si446x_reply_PART_INFO_map *part_info) {
    uint8_t cmd_buff[1] = {CMD_PART_INFO};  // Command code for PART_INFO (0x01)
    union si446x_cmd_reply_union reply;
    si4463_status_t status;
    
    // Check for valid parameters
    if (dev == NULL || part_info == NULL) {
        return SI4463_STATUS_INVALID_PARAM;
    }
    
    // Send command to request part info and get response
    status = radio_comm_sendcmd_getresp(dev, cmd_buff, sizeof(cmd_buff), 
                                      (uint8_t *)&reply, sizeof(reply.PART_INFO));
    
    if (status != SI4463_STATUS_SUCCESS) {
        return status; // Error in sending command or receiving response
    }
    
    // Copy the response to the provided structure
    memcpy(part_info, &reply.PART_INFO, sizeof(reply.PART_INFO));
    
    return SI4463_STATUS_SUCCESS;
}