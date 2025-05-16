#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "logging.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define DEBUG_UART huart2

// BME68x driver code
#include "bme68x_driver.h"

// Declare extern private functions from CubeMX
extern void SystemClock_Config(void);

int main(void)
{
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();

  debug_init(&huart2);

  if (HAL_I2C_IsDeviceReady(&hi2c1, BME68X_ADDR, 3, HAL_MAX_DELAY) == HAL_OK)
  {
    printf("Sensor is ready\r\n");
  }
  else
  {
    printf("Sensor not responding\r\n");
  }

  // Create bme interface struct and initialize it
  bme68x_sensor_t bme;
  bme_init(&bme, &hi2c1, &delay_us_timer);
  // Check status, should be 0 for OK
  int bme_status = bme_check_status(&bme);
  {
    if (bme_status == BME68X_ERROR)
    {
      printf("Sensor error:" + bme_status);
      return BME68X_ERROR;
    }
    else if (bme_status == BME68X_WARNING)
    {
      printf("Sensor Warning:" + bme_status);
    }
  }
  // Set temp, pressure, humidity oversampling configuration
  // Trying with defaults
  bme_set_TPH_default(&bme);
  // Alternatively, could set each to sample only once:
  // bme_set_TPH(&bme, BME68X_OS_1X, BME68X_OS_1X, BME68X_OS_1X);
  // Set the heater configuration to 300 deg C for 100ms for Forced mode
  bme_set_heaterprof(&bme, 300, 100);

  uint8_t sensor_id;
  bme_read(0xD0, &sensor_id, 4, &hi2c1);
  printf("Received sensor ID: 0x%X\r\n", sensor_id);

  /* USER CODE END 2 */

  /* Infinite loop */
  while (1)
  {
    // Set to forced mode, which takes a single sample and returns to sleep mode
    bme_set_opmode(&bme, BME68X_FORCED_MODE);
    /** @todo: May adjust the specific timing function called here, but it should be based on bme_get_meas_dur */
    delay_us_timer(bme_get_meas_dur(&bme, BME68X_SLEEP_MODE), &hi2c1);
    // Fetch data
    int fetch_success = bme_fetch_data(&bme);
    if (fetch_success)
    {
      printf("Temperature: %d.%02d°C, ",
                  bme.sensor_data.temperature / 100,
                  (bme.sensor_data.temperature % 100));
      printf("Pressure: %lu Pa, ", bme.sensor_data.pressure);
      printf("Humidity: %lu.%03lu%%, ",
                  bme.sensor_data.humidity / 1000,
                  (bme.sensor_data.humidity % 1000));
      printf("Gas Resistance: %lu.%03lu kΩ, ",
                  bme.sensor_data.gas_resistance / 1000,
                  (bme.sensor_data.gas_resistance % 1000));
      printf("Status: 0x%X\r\n", bme.sensor_data.status);
    }

    // The "blink" code is a simple verification of program execution,
    // separate from the BME68x sensor testing above
    // HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
    HAL_Delay(1000);

  }
}
