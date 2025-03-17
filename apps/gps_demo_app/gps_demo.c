/**
 * @file gps_demo.c
 * @brief Basic demo application for MAX-M10S GPS module
 */

#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include <stdio.h>
 
// Include GPS driver headers
#include "driver/max_m10s.h"
#include "protocols/ubx/ubx.h"

#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)

// Global callback function pointer
static void (*g_delay_callback)(void*) = NULL;
static void* g_delay_context = NULL;
 
// Function prototypes
static HAL_StatusTypeDef timer_delay_ms_it(TIM_HandleTypeDef *htim, uint32_t delay_ms, 
                                            void (*callback)(void*), void* context);

static void print_status(const char* message, status_t status);

// Global variables
extern I2C_HandleTypeDef hi2c1;
extern TIM_HandleTypeDef htim2;
static max_m10s_dev_s gps_dev;
static max_m10s_init_s gps_init;
 
/**
  * @brief Main application entry point
  */
void gps_demo_run(void) {
  status_t status;
   
  printf("GPS Demo Application Starting...\r\n");
   
  // Initialize GPS configuration
  gps_init.hi2c = &hi2c1;
  //gps_init.htim = &htim2;
  gps_init.device_address = 0x42;
  gps_init.timeout_ms = 1000;
  //gps_init.use_interrupts = 0;  // Blocking mode FIXME:
  // Set up function pointers - no casts needed
  gps_init.transmit_blocking = HAL_I2C_Master_Transmit;
  gps_init.receive_blocking = HAL_I2C_Master_Receive;
  gps_init.transmit_it = HAL_I2C_Master_Transmit_IT;
  gps_init.receive_it = HAL_I2C_Master_Receive_IT;
  gps_init.delay_it = timer_delay_ms_it;  
   
  printf("Initializing GPS device...\r\n");
   
  // Initialize the GPS device
  status = max_m10s_init(&gps_dev, &gps_init);
  print_status("GPS initialization", status);
   
  if (status == 0x00) {  // Success status
    printf("GPS device initialized successfully!\r\n");
    printf("GPS configuration completed. Device is in UBX protocol mode.\r\n");
  } else {
    printf("GPS initialization failed with status: 0x%02lX\r\n", status);
    return;
  }
   
  // Main application loop
  printf("Entering main loop...\r\n");
  while (1) {
    // Add your application code here
    // For example:
    // - Periodically read position data
    // - Process navigation information
    // - etc.
     
    HAL_Delay(1000);  // Delay 1 second between operations
     
    // For demo purposes, just toggle an LED to show the application is running
    HAL_GPIO_TogglePin(User_LED_GPIO_Port, User_LED_Pin);
  }
}


/**
 * @brief Starts a non-blocking delay using TIM2 with interrupt
 * @param htim Pointer to TIM2 handle
 * @param delay_ms Delay in milliseconds (up to 1000ms)
 * @param callback Function to call when delay completes
 * @param context User context pointer passed to callback
 * @return HAL_StatusTypeDef HAL_OK on success, error code otherwise
 */
static HAL_StatusTypeDef timer_delay_ms_it(TIM_HandleTypeDef *htim, uint32_t delay_ms, 
                                          void (*callback)(void*), void* context)
{
  // Validate parameters
  if (htim == NULL || delay_ms > 1000 || callback == NULL) {
    return HAL_ERROR;
  }
  
  // Store callback and context
  g_delay_callback = callback;
  g_delay_context = context;
  
  // Get timer clock frequency (Hz)
  uint32_t timer_freq = HAL_RCC_GetPCLK1Freq();
  if ((RCC->CFGR & RCC_CFGR_PPRE) != 0) {
    timer_freq *= 2; // Timer clock is 2x APB1 clock when APB1 prescaler != 1
  }
  
  // Calculate ticks per millisecond
  uint32_t ticks_per_ms = timer_freq / 1000;
  uint32_t ticks = delay_ms * ticks_per_ms;
  
  // Configure timer for one-pulse mode
  __HAL_TIM_SET_COUNTER(htim, 0);
  __HAL_TIM_SET_AUTORELOAD(htim, ticks);
  
  // Clear any pending interrupts
  __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_UPDATE);
  
  // Enable timer interrupt
  __HAL_TIM_ENABLE_IT(htim, TIM_IT_UPDATE);
  
  // Start timer
  return HAL_TIM_Base_Start_IT(htim);
}

/**
 * @brief Timer interrupt handler (to be called from TIM2_IRQHandler)
 * @param htim Timer handle
 */
void app_timer_elapsed_hook(TIM_HandleTypeDef *htim) {
  // Check if this is our timer
  if (htim->Instance == TIM2) {
    // Stop the timer
    HAL_TIM_Base_Stop_IT(htim);
    
    // Call the callback if set
    if (g_delay_callback != NULL) {
      void (*callback)(void*) = g_delay_callback;
      void* context = g_delay_context;
      
      // Clear callback pointers before calling the callback
      // This prevents issues if the callback tries to start a new delay
      g_delay_callback = NULL;
      g_delay_context = NULL;
      
      // Call the callback
      callback(context);
    }
  }
}
 
 /**
  * @brief Print status message
  * @param message Operation description
  * @param status Operation status code
  */
 static void print_status(const char* message, status_t status)
 {
   const char* status_str;
   
   switch (status) {
     case 0x00:
       status_str = "OK";
       break;
     case 0x01:
       status_str = "ERROR";
       break;
     case 0x02:
       status_str = "BUSY";
       break;
     case 0x03:
       status_str = "TIMEOUT";
       break;
     default:
       status_str = "UNKNOWN";
       break;
   }
   
   printf("%s: %s (0x%02lX)\r\n", message, status_str, status); 
 }


// Redirect printf to UART
PUTCHAR_PROTOTYPE {
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);
    return ch;
}