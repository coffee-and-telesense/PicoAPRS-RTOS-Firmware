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
void si4463_test_run(void);

#define SI_CS_PIN GPIO_PIN_6
#define SI_CS_PORT GPIOB
 
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
    si4463_test_run();
    
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
void si4463_test_run(void) {
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
    
 
 