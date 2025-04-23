/**
 * @file     test_aprs.c
 * @brief    Application to demonstrate continuous APRS packet transmission using
 *           the SI4463 chip with interrupt-driven FIFO handling.
 * @author   Generated from your requirements
 * @date     2025-04-18
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

/* Function prototypes */
void si4463_aprs_test_init(void);
void si4463_aprs_test_transmit(void);
void validate_si4463_part(si4463_dev_t *dev);
void handle_si4463_error(si4463_dev_t *dev);
void handle_irq_event(void);
void dev_state_one_shot(si4463_dev_t *dev);

volatile uint8_t check_dev_state_once = 1;

/* Pin definitions */
#define SI_CS_PIN GPIO_PIN_6
#define SI_CS_PORT GPIOB

#define SI_SDN_PIN GPIO_PIN_8
#define SI_SDN_PORT GPIOA

/* APRS packet data - predefined valid APRS stream */
static const uint8_t aprs_packet[] = {
    0x07, 0x59, 0x9A, 0x76, 0x59, 0xAB, 0x54, 0xA8, 0x8D, 0x8D,
    0x87, 0x66, 0x66, 0x69, 0xD7, 0x15, 0x50, 0x4C, 0xE9, 0x51,
    0x51, 0x51, 0x46, 0xAE, 0xAE, 0xD1, 0x46, 0xAE, 0xAE, 0x91,
    0x46, 0xAE, 0xAE, 0xEE, 0xB9, 0x51, 0x51, 0x4E, 0xB9, 0x51,
    0x51, 0x31, 0x46, 0xAE, 0xAE, 0xAE, 0xAE, 0xD1, 0x2E, 0xAE,
    0xAE, 0xA9, 0x5B, 0x37, 0x47, 0x47, 0x07, 0x56, 0xF3, 0x07,
    0x6F, 0x47, 0x48, 0xD6, 0xFF, 0x64, 0x7F, 0x00
};

 /**
  * @brief Main application entry point
  */
int main(void) {
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    si4463_status_t status;
    uint32_t tx_start_time = 0;

    HAL_Init();

    /* Configure the system clock */
    SystemClock_Config();

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_SPI1_Init();  // Assuming SPI1 is used for SI4463
    MX_USART2_UART_Init(); // For debug output

    /* Initialize debug logging */
    debug_init(&huart2);

    /* SI4463 APRS Test Application */
    si4463_aprs_test_init();

    /* Start APRS packet transmission test */
    DEBUG_INFO("Starting APRS packet transmission test\r\n");


    /* Infinite loop for continuous transmission */
    while (1) {
        if (si4463_device.trx_info.mode == SI4463_MODE_IDLE) {
            // Device is in IDLE mode, ready to transmit
            status = si4463_transmit_packet(&si4463_device, (uint8_t *)aprs_packet, sizeof(aprs_packet));
            tx_start_time = HAL_GetTick(); // Record the start time of transmission
            if(status != SI4463_STATUS_SUCCESS) {
                DEBUG_ERROR("Failed to transmit APRS packet with status: %d\r\n", status);
                handle_si4463_error(&si4463_device);
            }
            if(check_dev_state_once) {
                // To validate the device is in TX mode on first transmission
                dev_state_one_shot(&si4463_device);
            }
        }
        else {
            if(HAL_GetTick() - tx_start_time > 10000) {
                // Check if the device is still in TX mode after 1 second
                handle_si4463_error(&si4463_device);
            }
        }
    }
}

/**
 * @brief Initialize the SI4463 for APRS transmission
 */
void si4463_aprs_test_init(void) {
    si4463_status_t status;
    si4463_init_t init_config;

    /* Initialize the LED for status indication */
    HAL_GPIO_WritePin(User_LED_GPIO_Port, User_LED_Pin, GPIO_PIN_RESET);

    /* Print startup message */
    DEBUG_INFO("SI4463 APRS Transmission Test Starting...\r\n");
    HAL_Delay(100);

    /* Configure SI4463 initialization structure */
    init_config.hspi = &hspi1;
    init_config.transmit = HAL_SPI_Transmit;
    init_config.receive = HAL_SPI_Receive;
    init_config.transmit_receive = HAL_SPI_TransmitReceive;
    init_config.delay_blocking = HAL_Delay;
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
        handle_si4463_error(&si4463_device);
    }
}

void handle_si4463_error(si4463_dev_t *dev) {
    /* Handle SI4463 error */
    uint8_t curr_state = 0;
    si4463_status_t status;
    DEBUG_ERROR("SI4463 error occurred, polling dev state...\r\n");
    status = si4463_get_device_state(dev, &curr_state, NULL);
    if (status != SI4463_STATUS_SUCCESS) {
        DEBUG_ERROR("Failed to get device state with status: %d\r\n", status);
    }
    DEBUG_ERROR("Device State: %d\r\n", curr_state);

    // Get chip status
    uint8_t command_error_status = check_command_error(dev);
    if (command_error_status != 0) {
        DEBUG_ERROR("Command error status: %d\r\n", command_error_status);
    }

    while(1){
        HAL_GPIO_TogglePin(User_LED_GPIO_Port, User_LED_Pin);
        HAL_Delay(100);
    }
}

void dev_state_one_shot(si4463_dev_t *dev) {
    /* Handle SI4463 device state */
    uint8_t curr_state = 0;
    si4463_status_t status;

    status = si4463_get_device_state(dev, &curr_state, NULL);
    if (status != SI4463_STATUS_SUCCESS) {
        DEBUG_ERROR("Failed to get device state with status: %d\r\n", status);
    } else {
        DEBUG_INFO("Device State: %d\r\n", curr_state);
        check_dev_state_once = 0;
    }
}


/**
  * @brief Handle the NIRQ interrupt event
  *
  * This function is called from the EXTI interrupt handler
  * to process the NIRQ interrupt from the SI4463.
  */
void handle_irq_event(void) {
    si4463_status_t status;

    /* Call the packet handler to refill the TX FIFO */
    status = si4463_irq_pkt_handler(&si4463_device);

    if (status != SI4463_STATUS_SUCCESS) {
        DEBUG_ERROR("IRQ handler error with status: %d\r\n", status);
    }
}

/**
  * @brief EXTI line detection callback
  * @note This function handles the NIRQ interrupt
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == SI_NIRQ_Pin) {
         /* NIRQ pin triggered, handle the interrupt */
        handle_irq_event();
    }
}
