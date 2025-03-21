// In main.c
#include "main.h"
#include "usart.h"
#include "gpio.h"
#include "spi.h"
#include <stdio.h>
#include <string.h>

#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#define SI4463_CS_PIN GPIO_PIN_6
#define SI4463_CS_PORT GPIOB

// Si4463 Command IDs
#define CMD_POWER_UP 0x02
#define CMD_READ_CMD_BUFF 0x44
#define CMD_FIFO_INFO 0x15
#define CMD_WRITE_TX_FIFO 0x66

// FIFO_INFO flags
#define FIFO_INFO_RX_RESET 0x02  // Bit 1 for RX reset
#define FIFO_INFO_TX_RESET 0x01  // Bit 0 for TX reset

typedef struct {
    uint8_t rxFifoCount; // Number of bytes in RX FIFO
    uint8_t txFifoSpace; // Number of bytes available in TX FIFO (when empty this is 64 bytes)
} Si4463_FifoInfo;

// Declare extern private functions from CubeMX
extern void SystemClock_Config(void);

// Global variables
extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart2;

// Function prototypes
void Si4463_CS_Low(void);
void Si4463_CS_High(void);
HAL_StatusTypeDef Si4463_SendCommand(uint8_t cmd, uint8_t *data, uint8_t len);
HAL_StatusTypeDef Si4463_ReadCommandResponse(uint8_t *response, uint8_t len);
HAL_StatusTypeDef Si4463_WaitForCTS(uint16_t timeout);
HAL_StatusTypeDef Si4463_PowerUp(void);
HAL_StatusTypeDef Si4463_GetFifoInfo(Si4463_FifoInfo *fifoInfo, uint8_t resetFlags);
HAL_StatusTypeDef Si4463_WriteTxFifo(uint8_t *data, uint8_t len);
void TestFifoOperations(void);

int main(void)
{
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();
  
  /* Configure the system clock */
  SystemClock_Config();
  
  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init(); // For printf output
  MX_SPI1_Init();        // For SPI communication
 
  printf("Si4463 Initialization Starting...\r\n");
  
  // Make sure CS is initially high (deselected)
  Si4463_CS_High();
  
  // Small delay after power on
  HAL_Delay(20);
  
  // Check if the device is ready (CTS)
  printf("Checking CTS before sending POWER_UP...\r\n");
  if (Si4463_WaitForCTS(1000) == HAL_OK) {
    printf("CTS is high, device is ready\r\n");
    
    // Send power up command to the Si4463
    if (Si4463_PowerUp() == HAL_OK) {
      printf("Si4463 Power Up command sent successfully\r\n");
      
      // Wait for Power-up to complete (CTS goes high again)
      if (Si4463_WaitForCTS(1000) == HAL_OK) {
        printf("Power-up completed successfully\r\n");
        printf("\n---- Testing FIFO Operations ---\n");
        TestFifoOperations();  // Test both reading and writing to FIFO
      } else {
        printf("Timeout waiting for power-up to complete\r\n");
      }
    } else {
      printf("Error sending Power Up command\r\n");
    }
  } else {
    printf("Timeout waiting for CTS, device not ready\r\n");
  }
 
  /* Infinite loop */
  while (1)
  {
    HAL_Delay(1000);
  }
}
 
// Redirect printf to UART
PUTCHAR_PROTOTYPE {
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);
    return ch;
}

// Set chip select pin low (select the device)
void Si4463_CS_Low(void)
{
    HAL_GPIO_WritePin(SI4463_CS_PORT, SI4463_CS_PIN, GPIO_PIN_RESET);
}

// Set chip select pin high (deselect the device)
void Si4463_CS_High(void)
{
    HAL_GPIO_WritePin(SI4463_CS_PORT, SI4463_CS_PIN, GPIO_PIN_SET);
}

// Check if CTS is high (device ready to accept commands)
uint8_t Si4463_CheckCTS(void)
{
    HAL_StatusTypeDef status;
    uint8_t tx_buffer[2] = {CMD_READ_CMD_BUFF, 0x00}; // Command and NOP byte
    uint8_t rx_buffer[2] = {0, 0};
    
    Si4463_CS_Low();
    status = HAL_SPI_TransmitReceive(&hspi1, tx_buffer, rx_buffer, 2, 100);
    Si4463_CS_High();
    
    if (status != HAL_OK) {
        return 0;  // Error in communication
    }
    
    // The second byte received contains the CTS value
    return (rx_buffer[1] == 0xFF) ? 1 : 0;
}

// Wait for CTS to be high with timeout
HAL_StatusTypeDef Si4463_WaitForCTS(uint16_t timeout)
{
    uint32_t startTick = HAL_GetTick();
    
    while (HAL_GetTick() - startTick < timeout) {
        if (Si4463_CheckCTS()) {
            return HAL_OK;  // CTS is high
        }
        HAL_Delay(1);  // Small delay before checking again
    }
    
    return HAL_TIMEOUT;  // Timeout waiting for CTS
}

// Send a command to the Si4463
HAL_StatusTypeDef Si4463_SendCommand(uint8_t cmd, uint8_t *data, uint8_t len)
{
    HAL_StatusTypeDef status;
    uint8_t tx_buffer[32];  // Buffer for command and data (adjust size if needed)
    
    // Wait for CTS before sending any command (except READ_CMD_BUFF)
    if (cmd != CMD_READ_CMD_BUFF) {
        status = Si4463_WaitForCTS(1000);
        if (status != HAL_OK) {
            printf("CTS timeout before sending command 0x%02X\r\n", cmd);
            return status;
        }
    }
    
    // Fill buffer with command and data
    tx_buffer[0] = cmd;
    if (data != NULL && len > 0) {
        memcpy(&tx_buffer[1], data, len);
    }
    
    Si4463_CS_Low();
    status = HAL_SPI_Transmit(&hspi1, tx_buffer, len + 1, 100);
    Si4463_CS_High();
    
    return status;
}

// Read response after sending a command
HAL_StatusTypeDef Si4463_ReadCommandResponse(uint8_t *response, uint8_t len)
{
    HAL_StatusTypeDef status;
    uint8_t tx_buffer[32] = {0};  // NOP bytes to clock in response (adjust size if needed)
    uint8_t rx_buffer[32] = {0};  // Buffer for response (adjust size if needed)
    
    // First byte should be CMD_READ_CMD_BUFF
    tx_buffer[0] = CMD_READ_CMD_BUFF;
    
    Si4463_CS_Low();
    
    // Send READ_CMD_BUFF command and receive response in a single transaction
    status = HAL_SPI_TransmitReceive(&hspi1, tx_buffer, rx_buffer, len + 1, 100);
    
    Si4463_CS_High();
    
    if (status != HAL_OK) {
        return status;
    }
    
    // Check if CTS is valid (first byte should be 0xFF)
    if (rx_buffer[1] != 0xFF) {
        printf("Invalid CTS in response: 0x%02X\r\n", rx_buffer[0]);
        return HAL_ERROR;
    }
    
    // Copy response data (skip the CTS byte)
    if (response != NULL) {
        memcpy(response, &rx_buffer[2], len);
    }
    
    return HAL_OK;
}

// Send the Power Up command to the Si4463
HAL_StatusTypeDef Si4463_PowerUp(void)
{
    uint8_t powerUpData[6];
    
    powerUpData[0] = 0x01;        // Normal power up
    powerUpData[1] = 0x00;        // Crystal oscillator (not TCXO)
    
    // XO_FREQ: 30 MHz (30,000,000 Hz) - adjust according to your crystal frequency
    uint32_t xoFreq = 30000000;
    powerUpData[2] = (uint8_t)(xoFreq >> 24);
    powerUpData[3] = (uint8_t)(xoFreq >> 16);
    powerUpData[4] = (uint8_t)(xoFreq >> 8);
    powerUpData[5] = (uint8_t)(xoFreq);
    
    return Si4463_SendCommand(CMD_POWER_UP, powerUpData, 6);
}

// Function to read FIFO information and optionally reset FIFOs
HAL_StatusTypeDef Si4463_GetFifoInfo(Si4463_FifoInfo *fifoInfo, uint8_t resetFlags)
{
    HAL_StatusTypeDef status;
    uint8_t cmdData[1] = {resetFlags};  // 0x00 for no reset, or FIFO_INFO_RX/TX_RESET
    uint8_t response[2] = {0};  // RX_FIFO_COUNT + TX_FIFO_SPACE
    
    // Send FIFO_INFO command
    status = Si4463_SendCommand(CMD_FIFO_INFO, cmdData, 1);
    if (status != HAL_OK) {
        printf("Error sending FIFO_INFO command\r\n");
        return status;
    }

    if (Si4463_WaitForCTS(1000) != HAL_OK) {
        printf("Timeout waiting for CTS after FIFO_INFO command\r\n");
        return HAL_TIMEOUT;
    }
    
    // Read response (2 bytes: CTS + RX_FIFO_COUNT + TX_FIFO_SPACE)
    status = Si4463_ReadCommandResponse(response, 3);
    
    if (status != HAL_OK) {
        printf("Error reading FIFO_INFO response\r\n");
        return status;
    }
    
    // Fill the fifoInfo structure with the response data
    if (fifoInfo != NULL) {
        fifoInfo->rxFifoCount = response[0];  // RX_FIFO_COUNT
        fifoInfo->txFifoSpace = response[1];  // TX_FIFO_SPACE
    }
    
    return HAL_OK;
}

// Function to write data to the TX FIFO
HAL_StatusTypeDef Si4463_WriteTxFifo(uint8_t *data, uint8_t len)
{
    // According to the documentation, WRITE_TX_FIFO doesn't need CTS check
    // and doesn't have a response
    
    HAL_StatusTypeDef status;
    uint8_t tx_buffer[64];  // Adjust size as needed
    
    // Prepare command and data
    tx_buffer[0] = CMD_WRITE_TX_FIFO;
    
    if (data != NULL && len > 0) {
        memcpy(&tx_buffer[1], data, len);
    } else {
        return HAL_ERROR;  // No data to write
    }
    
    Si4463_CS_Low();
    status = HAL_SPI_Transmit(&hspi1, tx_buffer, len + 1, 100);
    Si4463_CS_High();
    
    return status;
}

// Test function for FIFO operations
void TestFifoOperations(void)
{
    Si4463_FifoInfo fifoInfo;
    HAL_StatusTypeDef status;
    
    // 1. First, read initial FIFO info
    printf("Reading initial FIFO information...\r\n");
    
    status = Si4463_GetFifoInfo(&fifoInfo, FIFO_INFO_RX_RESET | FIFO_INFO_TX_RESET);  // Reset everything
    
    if (status == HAL_OK) {
        printf("Initial FIFO Info read success!\r\n");
        printf("Initial RX FIFO Count: %d bytes\r\n", fifoInfo.rxFifoCount);
        printf("Initial TX FIFO Space: %d bytes\r\n", fifoInfo.txFifoSpace);
    } else {
        printf("Failed to read initial FIFO information\r\n");
        return;
    }
    
    //status = Si4463_GetFifoInfo(&fifoInfo, FIFO_INFO_RX_RESET | FIFO_INFO_TX_RESET);
    // wait for CTS
    if(Si4463_WaitForCTS(1000) != HAL_OK) {
        printf("Timeout waiting for CTS after FIFO_INFO command\r\n");
        return;
    }
    // Write some test data to TX FIFO
    printf("\nWriting test data to TX FIFO...\r\n");
    
    // Test data payload - 5 bytes
    uint8_t testData[3] = {0x06, 0x10, 0xF3};
    
    status = Si4463_WriteTxFifo(testData, sizeof(testData));
    
    if (status == HAL_OK) {
        printf("Successfully wrote %d bytes to TX FIFO\r\n", sizeof(testData));
    } else {
        printf("Failed to write to TX FIFO\r\n");
        return;
    }
    
    // Wait for CTS after writing to TX FIFO
    if(Si4463_WaitForCTS(1000) != HAL_OK) {
        printf("Timeout waiting for CTS after writing to TX FIFO\r\n");
        return;
    }
    // 4. Read FIFO info again to check if TX space decreased
    printf("\nReading FIFO information after writing to TX FIFO...\r\n");
    
    status = Si4463_GetFifoInfo(&fifoInfo, 0x00);  // No reset
    
    if (status == HAL_OK) {
        printf("FIFO Info read success!\r\n");
        printf("After writing - RX FIFO Count: %d bytes\r\n", fifoInfo.rxFifoCount);
        printf("After writing - TX FIFO Space: %d bytes\r\n", fifoInfo.txFifoSpace);
        
        // Verify the TX FIFO space has decreased by the amount of data we wrote
        int expectedSpace = fifoInfo.txFifoSpace + sizeof(testData);
        printf("Expected TX FIFO Space if no data was written: around %d bytes\r\n", expectedSpace);
    } else {
        printf("Failed to read FIFO information after writing\r\n");
    }
}