/**
 * @file     test_si.c
 * @brief    This application demonstrates and validates the use of the SI Driver Configuration array, and
 *            the ability to transmit at the configured frequency.
 * @author   Reece Wayt
 * @date     2025-03-28
 */


#include "main.h"
#include "spi.h"
#include "gpio.h"
#include "usart.h"
#include "si4463.h"
#include "logging.h"


/* Global variables */
static si4463_dev_t si4463_device;
extern void SystemClock_Config(void);
extern UART_HandleTypeDef huart2;
void si4463_test_init(void);
void si4463_test_transmit(void);
void poll_fifo_and_wait_loop(void);

#define SI_CS_PIN GPIO_PIN_6
#define SI_CS_PORT GPIOB

#define SI_SDN_PIN GPIO_PIN_8
#define SI_SDN_PORT GPIOA

uint8_t tx_data[] = {
    0x01, 0x01, 0x4C, 0xCB, 0x13, 0x4C, 0xA9, 0x56, 0xAE,
    0xE4, 0xE4, 0xF1, 0x33, 0x33, 0x2C, 0x51, 0xD5,
    0x5F, 0x66, 0x2D, 0x5D, 0x5D, 0x5D, 0x72, 0xA2,
    0xA2, 0x5D, 0x72, 0xA2, 0xA2, 0xDD, 0x72, 0xA2,
    0xA2, 0x22, 0x8D, 0x5D, 0x5D, 0x62, 0x8D, 0x5D,
    0x5D, 0x9D, 0x72, 0xA2, 0xA2, 0xA2, 0xA2, 0x5D,
    0xA2, 0xA2, 0xA2, 0xB1, 0x91, 0x71, 0x71, 0xF1,
    0xAD, 0x09, 0x51, 0x01, 0x01
};

 /**
  * @brief Main application entry point
  */
int main(void) {
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_USART2_UART_Init(); // For debug output

    /* Initialize debug logging */
    debug_init(&huart2);
    /* Verifies config works */
    si4463_test_init();
    /* Continuous transmission loop*/
    si4463_test_transmit();

    /* Infinite loop */
    while (1)
    {
        /* Should never reach here, but if it does, blink the LED */
        HAL_Delay(1000);  // Just a delay for demonstration
        HAL_GPIO_TogglePin(User_LED_GPIO_Port, User_LED_Pin);  // Toggle LED
    }
}


// @brief Initializes the device and validates config works
void si4463_test_init(void) {
    si4463_status_t status;
    si4463_init_t init_config;
    /* Configure SI4463 initialization structure */
    init_config.hspi = &hspi1;
    init_config.transmit = HAL_SPI_Transmit;
    init_config.receive = HAL_SPI_Receive;
    init_config.transmit_receive = HAL_SPI_TransmitReceive;
    init_config.delay_blocking = HAL_Delay;  // Using HAL_Delay instead of ThreadX delay
    init_config.timeout_ms = 1000;
    /* Configure GPIO pins */
    init_config.cs_port = SI_CS_PORT;
    init_config.cs_pin = SI_CS_PIN;
    init_config.sdn_port = SI_SDN_PORT;
    init_config.sdn_pin = SI_SDN_PIN;
    // If CTS is configured for GPIO pin
    #ifdef SI4463_USE_GPIO_CTS
        init_config.cts_port = SI_CTS_GPIO_Port;
        init_config.cts_pin = SI_CTS_Pin;
    #endif
    /* Initialize the SI4463 device */
    DEBUG_INFO("Initializing SI4463...\r\n");
    status = si4463_init(&si4463_device, &init_config);

    if (status == SI4463_STATUS_SUCCESS) {
        DEBUG_INFO("SI4463 initialization successful!\r\n");
    } else {
        DEBUG_ERROR("SI4463 initialization failed with status: %d\r\n", status);
        /* Blink LED rapidly to indicate error */
        while (1) {
            HAL_GPIO_TogglePin(User_LED_GPIO_Port, User_LED_Pin);
            HAL_Delay(100);
        }
    }
}

void si4463_test_transmit(void){
    DEBUG_INFO("Starting SI4463 Continuous Pattern Transmit Test...\r\n");
    si4463_status_t status;
    // For polling FIFO info
    // Polling this validates the FIFO is loaded and can be monitored to
    // confirm FIFO bytes are being sent out
    struct si446x_reply_FIFO_INFO_map fifoInfo;

    /* Initial FIFO reset */
    status = si4463_get_fifo_info(&si4463_device, &fifoInfo, 1, 0); // Reset TX FIFO
    if (status != SI4463_STATUS_SUCCESS) {
        DEBUG_ERROR("Failed to reset FIFO with status: %d\r\n", status);
        return; // Exit if FIFO reset fails
    }
    if(fifoInfo.TX_FIFO_SPACE != 64) {
        DEBUG_ERROR("TX FIFO space not as expected after reset: %d\r\n", fifoInfo.TX_FIFO_SPACE);
        return; // Exit if FIFO space is not as expected
    }
    // Continuous transmission loop
    while(1) {
        status = si4463_write_tx_fifo(&si4463_device, tx_data, sizeof(tx_data), 0);
        if (status != SI4463_STATUS_SUCCESS) {
            DEBUG_ERROR("Failed to write to TX FIFO with status: %d\r\n", status);
            HAL_Delay(500);
            continue;
        }
        DEBUG_INFO("Pattern data written to TX FIFO successfully.\r\n");
        /* Start initial transmission */
        status = si4463_start_tx(&si4463_device, 0, sizeof(tx_data), 0, 0);
        if (status != SI4463_STATUS_SUCCESS) {
            DEBUG_ERROR("Failed to start TX with status: %d\r\n", status);
            HAL_Delay(500);
            continue;
        }
        DEBUG_INFO("Transmission started successfully.\r\n");
        poll_fifo_and_wait_loop();

    }
}


void poll_fifo_and_wait_loop(void) {
    si4463_status_t status;
    struct si446x_reply_FIFO_INFO_map fifoInfo;
    uint8_t curr_state = 0;
    uint8_t current_channel = 0;

    while (1) {
        /* Get FIFO info */
        status = si4463_get_fifo_info(&si4463_device, &fifoInfo, 0, 0);
        if (status == SI4463_STATUS_SUCCESS) {
            DEBUG_INFO("FIFO Status: TX Space: %d, RX Count: %d\r\n",
                      fifoInfo.TX_FIFO_SPACE, fifoInfo.RX_FIFO_COUNT);
        } else {
            DEBUG_ERROR("Failed to get FIFO info: %d\r\n", status);
        }
        // Print fifo info for continuous observation
        DEBUG_INFO("FIFO Status: TX Space: %d, RX Count: %d\r\n",
            fifoInfo.TX_FIFO_SPACE, fifoInfo.RX_FIFO_COUNT);
        if(fifoInfo.TX_FIFO_SPACE == 64) {
            DEBUG_INFO("TX FIFO is empty !\r\n");
            HAL_Delay(1000);
            // Check device state, it should be in READY state
            status = si4463_get_device_state(&si4463_device, &curr_state, &current_channel);
            if(curr_state == 3 || curr_state == 4) {
                DEBUG_INFO("Device in READY state, restarting transmission...\r\n");
                HAL_Delay(1000);
                return; // Exit back to main transmit loop
            }
            else{
                DEBUG_INFO("ERROR IN DEVICE STATE...\r\n");
                while(1) {
                    HAL_Delay(1000);  // Just a delay for demonstration
                    HAL_GPIO_TogglePin(User_LED_GPIO_Port, User_LED_Pin);  // Toggle LED
                }
            }
            return;
        }
    }
}
