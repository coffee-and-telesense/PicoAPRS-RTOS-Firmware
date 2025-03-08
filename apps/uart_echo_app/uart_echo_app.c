/* uart_echo_app.c */
#include "main.h"
#include "tx_api.h"
#include "app_azure_rtos.h"
#include "app_threadx.h"
#include "usart.h"
#include "gpio.h"
#include "weak_functions.h"
//#include "app_hooks.h"
#include <stdio.h>

/* Private define ------------------------------------------------------------*/
#define TX_APP_STACK_SIZE                 1024
#define UART_ECHO_THREAD_STACK_SIZE       512
#define UART_ECHO_THREAD_PRIORITY         5
#define TX_APP_THREAD_PRIO                5


#define LED_TOGGLE_INTERVAL               100  // 1 second in ticks (assuming 100 ticks/sec)

TX_THREAD tx_app_thread;
/* USER CODE BEGIN PV */
TX_THREAD uart_echo_thread;
TX_MUTEX uart_mutex;

extern UART_HandleTypeDef huart2; 
uint8_t rx_data;  // Buffer for received character 
void uart_echo_thread_entry(ULONG thread_input);
void MainThread_Entry(ULONG thread_input);

// Forward declaration of the init function
UINT UartEchoApp_Init(VOID *memory_ptr);

UINT app_init_hook(VOID *memory_ptr) {
  return UartEchoApp_Init(memory_ptr);
}

int main(void)
{
  //register_app_init();
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_ThreadX_Init();

  /* We should never get here as control is now taken by the scheduler */
 
  while (1) {
    Error_Handler();
  }
}
/**
  * @brief  Application ThreadX Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT UartEchoApp_Init(VOID *memory_ptr)
{
  UINT ret = TX_SUCCESS;
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;

  /* USER CODE BEGIN App_ThreadX_MEM_POOL */
  /* USER CODE END App_ThreadX_MEM_POOL */
  CHAR *pointer;

  /* Allocate the stack for MainThread  */
  if (tx_byte_allocate(byte_pool, (VOID**) &pointer,
                       TX_APP_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }
  /* Create MainThread.  */
  if (tx_thread_create(&tx_app_thread, "MainThread", MainThread_Entry, 0, pointer,
                       TX_APP_STACK_SIZE, TX_APP_THREAD_PRIO, 5,
                       TX_NO_TIME_SLICE, TX_AUTO_START) != TX_SUCCESS)
  {
    return TX_THREAD_ERROR;
  }

  /* USER CODE BEGIN App_ThreadX_Init */
  // Allocate memory for the UART echo thread stack
  if(tx_byte_allocate(byte_pool, (VOID**) &pointer, 
      UART_ECHO_THREAD_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS) {
    Error_Handler();
  }

  if(tx_thread_create(&uart_echo_thread, "UART Echo Thread", uart_echo_thread_entry, 0, 
                      pointer, UART_ECHO_THREAD_STACK_SIZE, UART_ECHO_THREAD_PRIORITY, UART_ECHO_THREAD_PRIORITY, 
                      TX_NO_TIME_SLICE, TX_AUTO_START) != TX_SUCCESS)
  {
    Error_Handler();
  }

  if(tx_mutex_create(&uart_mutex, "UART Mutex", TX_NO_INHERIT) != TX_SUCCESS)
  {
    Error_Handler();
  }

  HAL_UART_Receive_IT(&huart2, &rx_data, 1);
  /* USER CODE END App_ThreadX_Init */

  return ret;
}
/**
  * @brief  Function implementing the MainThread_Entry thread.
  * @param  thread_input: Hardcoded to 0.
  * @retval None
  */
void MainThread_Entry(ULONG thread_input)
{
  /* USER CODE BEGIN MainThread_Entry */
  (void) thread_input;

  for(;;) {
    /* Toggle the User LED */
    HAL_GPIO_TogglePin(User_LED_GPIO_Port, User_LED_Pin);
     
    /* Sleep for the defined interval */
    tx_thread_sleep(LED_TOGGLE_INTERVAL);
  }
  /* USER CODE END MainThread_Entry */
}


/* USER CODE BEGIN 1 */
/**
  * @brief  UART Echo Thread function
  * @param  thread_input: ULONG user argument
  * @retval None
  */
void uart_echo_thread_entry(ULONG thread_input) {
  (void) thread_input;
  uint8_t echo_data;
   
  for(;;) {
    /* Wait for UART reception to complete (triggered by interrupt) */
    if (HAL_UART_GetState(&huart2) == HAL_UART_STATE_READY) {
      /* Get mutex to safely access UART */
      if (tx_mutex_get(&uart_mutex, TX_WAIT_FOREVER) == TX_SUCCESS) {
        echo_data = rx_data;  // Save received data
         
        /* Echo the received character back */
        HAL_UART_Transmit(&huart2, &echo_data, 1, HAL_MAX_DELAY);
         
        /* Start another reception */
        HAL_UART_Receive_IT(&huart2, &rx_data, 1);
         
        /* Release the mutex */
        tx_mutex_put(&uart_mutex);
      }
    }
     
    /* Yield to allow LED thread to run */
    tx_thread_sleep(1);
  }
}
 
 /**
   * @brief  UART Rx Transfer completed callback
   * @param  huart: UART handle
   * @retval None
   */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  /* This function is called when UART reception is complete */
  if (huart->Instance == USART2) {
    /* Wake up the UART thread to process the received data */
    tx_thread_resume(&uart_echo_thread);
  }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number */
}
#endif /* USE_FULL_ASSERT */