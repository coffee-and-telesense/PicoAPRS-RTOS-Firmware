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
void validate_si4463_part(si4463_dev_t *dev);

#define SI_CS_PIN GPIO_PIN_6
#define SI_CS_PORT GPIOB

#define SI_SDN_PIN GPIO_PIN_8
#define SI_SDN_PORT GPIOA
 
 /**
  * @brief Main application entry point
  */
int main(void) {
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();
    
    /* Configure the system clock */
    SystemClock_Config();
    
    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_SPI1_Init();  // Assuming SPI1 is used for SI4463
    MX_USART2_UART_Init(); // For debug output
    
    /* Initialize debug logging */
    debug_init(&huart2);
    
    /* SI4463 Test Application */
    si4463_test_init();

    /* Valid Part Info */
    validate_si4463_part(&si4463_device);

    si4463_test_transmit(); 
    
    /* Infinite loop */
    while (1)
    {
        /* Main loop can be used for other tasks */
        HAL_Delay(1000);  // Just a delay for demonstration
        HAL_GPIO_TogglePin(User_LED_GPIO_Port, User_LED_Pin);  // Toggle LED
    }
}
 
 /**
  * @brief Main test function for SI4463 configuration
  */
void si4463_test_init(void) {
    si4463_status_t status;
    si4463_init_t init_config;
    
    /* Initialize the LED for status indication */
    HAL_GPIO_WritePin(User_LED_GPIO_Port, User_LED_Pin, GPIO_PIN_RESET);
    
    /* Print startup message */
    DEBUG_INFO("SI4463 Configuration Test Starting...\r\n");
    HAL_Delay(100);
    
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
   
    #ifdef SI4463_USE_GPIO_CTS
        init_config.cts_port = SI_CTS_GPIO_Port;
        init_config.cts_pin = SI_CTS_Pin;
    #endif
    
    /* Initialize the SI4463 device */
    DEBUG_INFO("Initializing SI4463...\r\n");
    status = si4463_init(&si4463_device, &init_config);
    
    if (status == SI4463_STATUS_SUCCESS) {
        DEBUG_INFO("SI4463 initialization successful!\r\n");
        HAL_GPIO_WritePin(User_LED_GPIO_Port, User_LED_Pin, GPIO_PIN_SET);
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
    uint8_t curr_state = 0;
    uint8_t current_channel = 0;
    struct si446x_reply_FIFO_INFO_map fifoInfo;
    
    /* Create alternating pattern (0xAA = 10101010 in binary) */
    uint8_t tx_data[50];
    for (size_t i = 0; i < sizeof(tx_data); i++) {
        tx_data[i] = (i % 2 == 0) ? 0xAA : 0x55; /* Alternates between 10101010 and 01010101 */
    }

    /* Initial FIFO reset */
    status = si4463_get_fifo_info(&si4463_device, &fifoInfo, 1, 0); // Reset TX FIFO
    if (status != SI4463_STATUS_SUCCESS) {
        DEBUG_ERROR("Failed to reset FIFO with status: %d\r\n", status);
        return;
    }
    
    /* Write initial pattern to TX FIFO */
    status = si4463_write_tx_fifo(&si4463_device, tx_data, sizeof(tx_data), 0);
    if (status != SI4463_STATUS_SUCCESS) {
        DEBUG_ERROR("Failed to write to TX FIFO with status: %d\r\n", status);
        return;
    }
    DEBUG_INFO("Pattern data written to TX FIFO successfully.\r\n");
    
    /* Start initial transmission */
    DEBUG_INFO("Starting continuous pattern transmission...\r\n");
    status = si4463_start_tx(&si4463_device, 0, sizeof(tx_data), 0, 0);
    if (status != SI4463_STATUS_SUCCESS) {
        DEBUG_ERROR("Failed to start TX with status: %d\r\n", status);
        return;
    }

    /* Continuous transmission control loop */
    DEBUG_INFO("Entering continuous transmission control loop...\r\n");
    while(1) {
        /* Check device state */
        status = si4463_get_device_state(&si4463_device, &curr_state, &current_channel);
        if (status != SI4463_STATUS_SUCCESS) {
            DEBUG_ERROR("Failed to get device state with status: %d\r\n", status);
            HAL_Delay(500);
            continue;
        }

        /* Print current state */
        DEBUG_INFO("Device State: %d, Channel: %d\r\n", curr_state, current_channel);

        /* Get and print current FIFO status */
        status = si4463_get_fifo_info(&si4463_device, &fifoInfo, 0, 0); // Don't reset, just check
        if (status == SI4463_STATUS_SUCCESS) {
            DEBUG_INFO("FIFO Status: TX Space: %d, RX Count: %d\r\n", 
                      fifoInfo.TX_FIFO_SPACE, fifoInfo.RX_FIFO_COUNT);
        } else {
            DEBUG_ERROR("Failed to get FIFO info: %d\r\n", status);
        }

        /* If not in TX state (7) AND in READY state (3 or 4), restart transmission */
        if (curr_state != 7 && (curr_state == 3 || curr_state == 4)) {
            DEBUG_INFO("Device in READY state, restarting transmission...\r\n");
            
            /* Reset TX FIFO */
            status = si4463_get_fifo_info(&si4463_device, &fifoInfo, 1, 0);
            if (status != SI4463_STATUS_SUCCESS) {
                DEBUG_ERROR("Failed to reset TX FIFO with status: %d\r\n", status);
                HAL_Delay(500);
                continue;
            }
            
            /* Write pattern data to TX FIFO again */
            status = si4463_write_tx_fifo(&si4463_device, tx_data, sizeof(tx_data), 0);
            if (status != SI4463_STATUS_SUCCESS) {
                DEBUG_ERROR("Failed to write to TX FIFO with status: %d\r\n", status);
                HAL_Delay(500);
                continue;
            }
            
            /* Start transmission again */
            status = si4463_start_tx(&si4463_device, 0, sizeof(tx_data), 0, 0);
            if (status != SI4463_STATUS_SUCCESS) {
                DEBUG_ERROR("Failed to restart TX with status: %d\r\n", status);
                HAL_Delay(500);
                continue;
            }
            status = si4463_get_device_state(&si4463_device, &curr_state, &current_channel);
            if (status != SI4463_STATUS_SUCCESS) {
                DEBUG_ERROR("Failed to get device state with status: %d\r\n", status);
                HAL_Delay(500);
                continue;
            }
            DEBUG_INFO("Device State: %d, Channel: %d\r\n", curr_state, current_channel);
            
            DEBUG_INFO("Pattern transmission restarted.\r\n");
        }
        
        /* Small delay between state checks */
        HAL_Delay(500);
    }
}


void validate_si4463_part(si4463_dev_t *dev) {
    struct si446x_reply_PART_INFO_map part_info;
    si4463_status_t status;
    
    DEBUG_INFO("Validating SI4463 part number...\r\n");
    
    status = si4463_get_part_info(dev, &part_info);
    if (status != SI4463_STATUS_SUCCESS) {
        DEBUG_ERROR("Failed to get part info with status: %d\r\n", status);
        return;
    }
    
    DEBUG_INFO("SI4463 Part Info:\r\n");
    DEBUG_INFO("  Chip Rev: 0x%02X\r\n", part_info.CHIPREV);
    DEBUG_INFO("  Part Number: 0x%04X\r\n", part_info.PART);
    DEBUG_INFO("  Build: 0x%02X\r\n", part_info.PBUILD);
    DEBUG_INFO("  ID: 0x%04X\r\n", part_info.ID);
    DEBUG_INFO("  Customer: 0x%02X\r\n", part_info.CUSTOMER);
    DEBUG_INFO("  ROM ID: 0x%02X\r\n", part_info.ROMID);
    
    // Validate against expected values
    if (part_info.PART == 0x4463) {
        DEBUG_INFO("Valid SI4463 part detected!\r\n");
    } else {
        DEBUG_ERROR("Invalid part number! Expected 0x4463, got 0x%04X\r\n", part_info.PART);
    }
}
 
 