#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define DEBUG_UART huart2

// BME68x driver code
#include "bme68x_driver.h"

void debug_print(const char *fmt, ...);
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

  if (HAL_I2C_IsDeviceReady(&hi2c1, BME68X_ADDR, 3, HAL_MAX_DELAY) == HAL_OK)
  {
    debug_print("Sensor is ready\r\n");
  }
  else
  {
    debug_print("Sensor not responding\r\n");
  }

  // Create bme interface struct and initialize it
  bme68x_sensor_t bme;
  bme_init(&bme, &hi2c1, &delay_us_timer);
  // Check status, should be 0 for OK
  int bme_status = bme_check_status(&bme);
  {
    if (bme_status == BME68X_ERROR)
    {
      debug_print("Sensor error:" + bme_status);
      return BME68X_ERROR;
    }
    else if (bme_status == BME68X_WARNING)
    {
      debug_print("Sensor Warning:" + bme_status);
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
  debug_print("Received sensor ID: 0x%X\r\n", sensor_id);

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
      debug_print("Temperature: %d.%02d°C, ",
                  bme.sensor_data.temperature / 100,
                  (bme.sensor_data.temperature % 100));
      debug_print("Pressure: %d Pa, ", bme.sensor_data.pressure);
      debug_print("Humidity: %d.%03d%%, ",
                  bme.sensor_data.humidity / 1000,
                  (bme.sensor_data.humidity % 1000));
      debug_print("Gas Resistance: %d.%03d kΩ, ",
                  bme.sensor_data.gas_resistance / 1000,
                  (bme.sensor_data.gas_resistance % 1000));
      debug_print("Status: 0x%X\r\n", bme.sensor_data.status);
    }

    // The "blink" code is a simple verification of program execution,
    // separate from the BME68x sensor testing above
    // HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
    HAL_Delay(1000);

  }
}

void debug_print(const char *fmt, ...)
{
  char buffer[128]; // Adjust size as needed
  va_list args;
  va_start(args, fmt);

  // Format the string
  int result = vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  if (result < 0)
  {
    // Handle snprintf error
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"vsnprintf error\n", 16, 100);
  }
  else
  {
    // Ensure null termination
    buffer[sizeof(buffer) - 1] = '\0';

    // Get the actual length
    size_t len = (size_t)result;
    if (len > sizeof(buffer))
    {
      len = sizeof(buffer) - 1;
    }

    // Transmit the formatted message
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)buffer, len, 100);
  }
}
