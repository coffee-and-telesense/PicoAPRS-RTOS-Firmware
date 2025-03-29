/**
 * @file ax25.c
 * @brief AX.25 Encoding Implementation for APRS Transmissions
 * 
 * This file contains the implementation of the AX.25 encoding protocol used for APRS (Automatic Packet
 * Reporting System) transmissions. AX.25 is a data link layer protocol used in amateur radio for
 * communication between radio stations. The functions in this file handle the construction, encoding, 
 * and transmission of AX.25 frames, including frame creation, CRC generation, NRZI encoding, and other 
 * related functionalities specific to APRS data transfer.
 * 
 * The implementation includes the following key functionalities:
 * - AX.25 frame creation and CRC (Cyclic Redundancy Check) generation
 * - Data encoding in NRZI (Non-Return to Zero Inverted) format
 * - Handling of the AX.25 frame structure (header, payload, FCS, etc.)
 * - Support for APRS-specific features like SSID (Secondary Station Identifier) and PID (Protocol Identifier)
 * 
 * This implementation is intended for use in embedded systems, particularly in applications related to
 * amateur radio APRS systems.
 * 
 * @note The AX.25 encoding methods in this file follow the official specification for AX.25 frame
 *       structures and transmission formats as defined by the AX.25 standard.
 * 
 * @author Arie Jorritsma
 * @date September 2024
 */


#include "ax25.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// CRC table for CRC-16-CCITT
// https://www.ax25.net/AX25.2.2-Jul%2098-2.pdf AX.25 Link Access Protocol for Amateur Packet Radio
// The Frame-Check Sequence is calculated in accordance with
// recommendations in the HDLC reference document, ISO 3309.
// The CRC-16-CCITT polynomial is x^16 + x^12 + x^5 + 1

uint16_t crc16_table[] = {
    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
    0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
    0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
    0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
    0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
    0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
    0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
    0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
    0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
    0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
    0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
    0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
    0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
    0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
    0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
    0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
    0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
    0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
    0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
    0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
    0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
    0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
    0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
    0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
    0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
    0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
    0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
    0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
    0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
    0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
    0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
    0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78};

/**
 * @brief Processes the input AX.25 frame and returns an NRZI-encoded frame.
 *
 * This function processes an AX.25 frame, which includes:
 * - Shifting bits for source and destination addresses
 * - Concatenating CRC data
 * - Generating CRC16
 * - Concatenating the AX.25 frame
 * - Converting hex to binary
 * - Performing bit-stuffing
 * - Generating NRZI bitstream
 *
 * @param frame A pointer to the input AX.25 frame.
 *
 * @return A structure containing the NRZI bitstream and its size.
 */
hdlcFrame processFrame(iFrame *frame)
{

  // Shift bits for destination and source addresses
  shiftBits(frame);

  // Concatenate CRC frame
  concatCrcFrame(frame);

  // Generate CRC
  makeCRC(frame);

  // Concatenate AX.25 frame
  concatAx25Frame(frame);

  // Convert hex to binary
  hextobin_rev(frame);

  // Perform bit stuffing
  bitStuff(frame);

  // Generate NRZI bitstream
  uint8_t *nrziBinHdlcFrame = genNRZI(frame);

  // Create NRZIFrame structure to hold the NRZI bitstream and its size
  hdlcFrame result;
  result.nrziBinHdlcFrame = nrziBinHdlcFrame;
  result.size = frame->nrziBinHdlcFrameSize;

  // Free allocated memory for the frame but not the NRZI bitstream
  cleanFrame(frame);

  // Return the NRZIFrame structure
  return result;
}

/**
 * @brief Processes the input AX.25 frame and returns an NRZI-encoded frame.
 *
 * This function processes an AX.25 frame, which includes:
 * - Shifting bits for source and destination addresses
 * - Concatenating CRC data
 * - Generating CRC16
 * - Concatenating the AX.25 frame
 * - Converting hex to binary
 * - Performing bit-stuffing
 * - Generating NRZI bitstream
 *
 * This function also prints the intermediate steps of the process.
 *
 * @param frame A pointer to the input AX.25 frame.
 *
 * @return A structure containing the NRZI bitstream and its size.
 */
hdlcFrame processFrameVerbose(iFrame *frame)
{

  shiftBits(frame);
  printHex("Destination", frame->dest, CALL_LEN);
  printHex("Source", frame->src, CALL_LEN);

  concatCrcFrame(frame);
  makeCRC(frame);
  concatAx25Frame(frame);
  hextobin_rev(frame);

  printHex("CRC", frame->fcs, 2);

  printHex("Initial Hex String", frame->ax25Frame, frame->ax25FrameSize);

  printBin("Initial Bitstream", frame->binAx25Frame, frame->binAx25FrameSize);

  bitStuff(frame);
  printBin("BitStuffed Bitstream", frame->binHdlcFrame,
           frame->binHdlcFrameSize);

  uint8_t *nrziBinHdlcFrame = genNRZI(frame);
  printBin("NRZI Bitstream", nrziBinHdlcFrame, frame->nrziBinHdlcFrameSize);

  hdlcFrame result;
  result.nrziBinHdlcFrame = nrziBinHdlcFrame;
  result.size = frame->nrziBinHdlcFrameSize;

  cleanFrame(frame);

  return result;
}

/**
 * @brief Initializes an AX.25 frame with the given information.
 *
 * This function initializes an AX.25 frame with the given information.
 *
 * @param info A pointer to the information to be included in the frame.
 * @param infoSize The size of the information.
 *
 * @return A pointer to the initialized AX.25 frame.
 */
iFrame *initFrame(uint8_t *info, size_t infoSize)
{
  iFrame *frame = (iFrame *)malloc(sizeof(iFrame));
  if (frame == NULL)
  {
    return NULL; // todo: error handling currently returns scary null pointer if malloc fails
  }

  frame->src = (uint8_t *)malloc(6 * sizeof(uint8_t));
  frame->dest = (uint8_t *)malloc(6 * sizeof(uint8_t));
  frame->fcs = (uint8_t *)malloc(2 * sizeof(uint8_t));

  if (frame->src == NULL || frame->dest == NULL || frame->fcs == NULL)
  {
    return NULL;
  }

  memcpy(frame->src, CALL, CALL_LEN);
  memcpy(frame->dest, DEST, CALL_LEN);
  frame->ssidDest = SSID_DEST;
  frame->ssidSrc = SSID_SRC;
  frame->cntrl = CONTROL;
  frame->pid = PID;
  frame->infoSize = infoSize;
  frame->info = info;
  frame->crcFrame = NULL;
  frame->ax25Frame = NULL;
  frame->binAx25Frame = NULL;
  frame->binHdlcFrame = NULL;
  frame->flag = FLAG;

  return frame;
}

/**
 * @brief Shifts the bits for the source and destination addresses in the AX.25 frame.
 *
 * This function performs a left shift of each byte in the source and destination addresses by 1 bit.
 *
 * @param frame A pointer to the AX.25 frame.
 */
void shiftBits(iFrame *frame)
{

  for (size_t i = 0; i < CALL_LEN; i++)
  {
    frame->dest[i] <<= 1;
  }

  for (size_t i = 0; i < CALL_LEN; i++)
  {
    frame->src[i] <<= 1;
  }
}

/**
 * @brief Prints the hexadecimal representation of data.
 *
 * This function prints the provided data as a hexadecimal string.
 *
 * @param label The label to print before the data.
 * @param data Pointer to the data to print.
 * @param len The length of the data in bytes.
 */
void printHex(const char *label, uint8_t *data, size_t len)
{
  int newLineIndex = 0;
  printf("%s:\n", label);
  printf("--------------------------------------------\n");
  for (size_t i = 0; i < len; i++)
  {
    printf("%x", data[i]);
    if ((i + 1) % 2 == 0)
    {
      printf(" ");
    }
    if ((i + 1) % 16 == 15)
    {
      printf("\n");
      newLineIndex = 0;
    }
    else
    {
      newLineIndex++;
    }
  }
  if ((newLineIndex < 15) && (newLineIndex != 0))
  {
    printf("\n");
  }
  printf("--------------------------------------------\n");
}

/**
 * @brief Converts a hexadecimal AX.25 frame to binary in reverse order.
 *
 * This function converts each byte in the AX.25 frame to its binary representation, with the least-significant bit first.
 *
 * @param frame A pointer to the AX.25 frame.
 */
void hextobin_rev(iFrame *frame)
{
  size_t size = frame->ax25FrameSize;
  uint8_t *bin = (uint8_t *)malloc((size * 8) * sizeof(uint8_t));
  if (bin == NULL)
  {
    return; //todo: error handling
  }

  // Process all bytes
  for (size_t i = 0; i < (size) * 8; i++)
  {
    size_t index = i / 8;
    uint8_t mask = 0x01 << (i % 8); // Start from LSB
    bin[i] = (frame->ax25Frame[index] & mask) ? 1 : 0;
  }

  frame->binAx25FrameSize = size * 8;
  frame->binAx25Frame = bin;
}

/**
 * @brief Generates the CRC16 for the given frame.
 *
 * This function calculates the CRC16 checksum for the frame and stores it in the frame's fcs array.
 *
 * @param frame A pointer to the AX.25 frame.
 */
void makeCRC(iFrame *frame)
{
  size_t size = frame->crcFrameSize;
  uint8_t *buf = frame->crcFrame;
  uint16_t crc = 0xFFFF;
  uint32_t data;

  for (int i = 0; i < size; i++)
  {
    crc = (crc >> 8) ^ crc16_table[(crc ^ (uint16_t)*buf++) & 0x00ff];
  }

  // Byte swap
  data = crc;
  crc = (crc << 8) | ((data >> 8) & 0xff);
  crc = ~crc; // Invert the CRC

  // Store the CRC into the frame->fcs array
  frame->fcs[1] = crc & 0xff;        // Lower byte
  frame->fcs[0] = (crc >> 8) & 0xff; // Upper byte
}

/**
 * @brief Performs bit-stuffing on the AX.25 frame.
 *
 * This function modifies the frame by inserting 0 after every sequence of five consecutive 1 bits.
 *
 * @param frame A pointer to the AX.25 frame.
 */
void bitStuff(iFrame *frame)
{

  uint8_t *data = frame->binAx25Frame;
  size_t size = frame->binAx25FrameSize;
  int sum = 0;
  int sumZeros = 0;
  int zeroIndex = 0;
  int offset = 0;

  uint8_t *zeroPos = (uint8_t *)malloc((size / 6) * sizeof(uint8_t));
  if (zeroPos == NULL)
  {
    return; //todo: error handling
  }

  memset(zeroPos, 0, (size / 6) * sizeof(uint8_t));

  for (int i = 8; i < size - 8; i++)
  {

    if (data[i] == 1)
    {
      sum = sum + 1;
    }
    else
    {
      sum = 0;
    }

    if (sum == 5)
    {

      sumZeros = sumZeros + 1;

      zeroPos[zeroIndex] = i + 1;

      zeroIndex = zeroIndex + 1;

      sum = 0;
    }
  }

  uint8_t *hdlcFrame = (uint8_t *)malloc((size + sumZeros) * sizeof(uint8_t));
  if (hdlcFrame == NULL)
  {
    return; //todo: error handling
  }

  zeroIndex = 0;

  for (int i = 0; i < (size + sumZeros); i++)
  {

    if (i == zeroPos[zeroIndex] && i > 1)
    {

      hdlcFrame[i] = 0;

      zeroIndex = zeroIndex + 1;
      offset = offset + 1;
    }
    else
    {
      hdlcFrame[i] = data[i - offset];
    }
  }

  free(zeroPos);
  frame->binHdlcFrameSize = (size + sumZeros);
  frame->binHdlcFrame = hdlcFrame;
}

/**
 * @brief Concatenates the AX.25 frame components to create the CRC frame.
 *
 * This function concatenates the destination address, source address, control fields,
 * protocol identifier, and information into a single frame and prepares it for CRC calculation.
 * The final concatenated frame is stored in the `crcFrame` field of the given `iFrame` structure.
 *
 * The CRC frame includes:
 * - Destination address (6 bytes)
 * - SSID of the destination (1 byte)
 * - Source address (6 bytes)
 * - SSID of the source (1 byte)
 * - Control field (1 byte)
 * - Protocol identifier (1 byte)
 * - Information (variable size, depending on `infoSize`)
 *
 * @param frame A pointer to the `iFrame` structure that contains the data to be concatenated.
 *
 * @note The function dynamically allocates memory for the CRC frame. It is the caller's responsibility
 *       to free this memory when it is no longer needed.
 */
void concatCrcFrame(iFrame *frame)
{
  frame->crcFrameSize = 16 + frame->infoSize;
  uint8_t *crcFrame = (uint8_t *)malloc(frame->crcFrameSize * sizeof(uint8_t));
  if (crcFrame == NULL)
  {
    return; //todo: error handling
  }
  printf("CRC Frame Size: %ld\n", frame->crcFrameSize);

  memmove(crcFrame, frame->dest, 6);
  crcFrame[6] = frame->ssidDest;
  memmove(crcFrame + 7, frame->src, 6);
  crcFrame[13] = frame->ssidSrc;
  crcFrame[14] = frame->cntrl;
  crcFrame[15] = frame->pid;
  memmove(crcFrame + 16, frame->info, frame->infoSize);

  frame->crcFrame = crcFrame;
}

/**
 * @brief Concatenates the AX.25 frame by adding flags and FCS to the CRC frame.
 *
 * This function takes the CRC frame created by `concatCrcFrame` and appends the start and end flags
 * (0x7E), along with the Frame Check Sequence (FCS) bytes to create a complete AX.25 frame.
 *
 * The complete AX.25 frame structure includes:
 * - Start flag (1 byte)
 * - CRC frame (variable size, depending on `crcFrameSize`)
 * - FCS (2 bytes)
 * - End flag (1 byte)
 *
 * @param frame A pointer to the `iFrame` structure that contains the CRC frame and FCS.
 *
 * @note The function dynamically allocates memory for the AX.25 frame. It is the caller's responsibility
 *       to free this memory when it is no longer needed.
 */
void concatAx25Frame(iFrame *frame)
{
  size_t size = frame->crcFrameSize + 4;
  uint8_t *cpltFrame = (uint8_t *)malloc(size * sizeof(uint8_t));
  if (cpltFrame == NULL)
  {
    return; //todo: error handling
  }

  cpltFrame[0] = frame->flag;
  memcpy(cpltFrame + 1, frame->crcFrame, frame->crcFrameSize);
  cpltFrame[frame->crcFrameSize + 1] = frame->fcs[0];
  cpltFrame[frame->crcFrameSize + 2] = frame->fcs[1];
  cpltFrame[frame->crcFrameSize + 3] = frame->flag;

  frame->ax25FrameSize = size;
  frame->ax25Frame = cpltFrame;
}

/**
 * @brief Generates an NRZI (Non-Return-to-Zero Inverted) encoded frame from a binary HDLC frame.
 *
 * This function takes a binary HDLC frame and applies the NRZI encoding scheme, where a bit value of `0`
 * causes the signal level to invert, and a bit value of `1` causes the signal to maintain its current level.
 *
 * The NRZI encoding is performed bit by bit, with each bit causing a transition or maintaining the current
 * signal level based on its value.
 *
 * @param frame A pointer to the `iFrame` structure that contains the binary HDLC frame (`binHdlcFrame`)
 *              to be encoded.
 *
 * @return A pointer to a newly allocated array containing the NRZI encoded frame.
 *
 * @note The function dynamically allocates memory for the NRZI frame. It is the caller's responsibility
 *       to free this memory when it is no longer needed.
 */
uint8_t *genNRZI(iFrame *frame)
{
  bool cur_level = true;
  size_t size = frame->binHdlcFrameSize;
  uint8_t *nrziFrame =
      (uint8_t *)malloc(frame->binHdlcFrameSize * sizeof(uint8_t));
  if (nrziFrame == NULL)
  {
    return NULL; // todo: error handling
  }
  for (size_t i = 0; i < size; i++)
  {
    if (frame->binHdlcFrame[i] == 0)
    {
      cur_level = !cur_level;
    }
    nrziFrame[i] = cur_level ? 1 : 0;
  }
  frame->nrziBinHdlcFrameSize = size;
  return (nrziFrame);
}

/**
 * @brief Prints a binary representation of a byte array with a label.
 *
 * This function prints each byte of the provided binary array as a series of bits, formatted in groups
 * of 8 bits (1 byte) with spaces between each byte. The output is structured with the specified label
 * at the top and bottom dividers for easy readability.
 *
 * @param label A string label to print at the beginning of the output.
 * @param bin A pointer to the array of bytes (binary data) to be printed.
 * @param size The size of the binary array (number of bytes).
 *
 * @note The binary representation is printed as a sequence of '1's and '0's for each byte in the array.
 *       Each byte is displayed in a separate group with a space between every 8 bits for clarity.
 */
void printBin(const char *label, uint8_t *bin, size_t size)
{
  int newLineIndex = 0;
  printf("%s:\n", label);
  printf("--------------------------------------------\n");
  for (size_t i = 0; i < size; i++)
  {
    printf("%d", bin[i]);
    if ((i + 1) % 8 == 0)
    { // Print a space after every 8 bits (1 byte)
      printf(" ");
      if (newLineIndex == 7)
      {
        printf("\n");
        newLineIndex = 0;
      }
      else
      {
        newLineIndex++;
      }
    }
  }
  printf("\n");
  printf("--------------------------------------------\n");
}

/**
 * @brief Frees the memory allocated for the AX.25 frame.
 *
 * This function frees the memory allocated for the AX.25 frame.
 *
 * @param frame A pointer to the AX.25 frame.
 */
void cleanFrame(iFrame *frame)
{
  if (frame)
  {
    free(frame->src);
    free(frame->dest);
    free(frame->fcs);
    free(frame->crcFrame);
    free(frame->ax25Frame);
    free(frame->binAx25Frame);
    free(frame->binHdlcFrame);
    free(frame);
  }
}
