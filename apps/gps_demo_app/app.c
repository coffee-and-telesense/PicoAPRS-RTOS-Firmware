// In main.c
#include "gps_demo.h"

int main(void)
{
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_USART2_UART_Init(); // For printf output
  
  /* Redirect printf to UART */
  // Add your printf redirection code here if needed
  
  /* Run the GPS demo application */
  gps_demo_run();
  
  /* Infinite loop */
  while (1)
  {
    // This code won't be reached as gps_demo_run has its own infinite loop
  }
}