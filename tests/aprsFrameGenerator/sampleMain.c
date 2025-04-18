

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aprs.h"
#include "aprsConfig.h"
#include "ax25.h"

// input values
char *analogValue1 = "001,";
char *analogValue2 = "002,";
char *analogValue3 = "003,";
char *analogValue4 = "004,";
char *analogValue5 = "005,";
char *digitalValue = "00001100";
char *comment = "hello!";


int main()
{

    telemetryInfoFrame *tFrame = initTFrame();
    updateTelemData(
        tFrame,
        (uint8_t*)analogValue1,
        (uint8_t*)analogValue2,
        (uint8_t*)analogValue3,
        (uint8_t*)analogValue4,
        (uint8_t*)analogValue5,
        (uint8_t*)digitalValue,
        comment
    );
    concatTelemData(tFrame);
    
    printHex("Telemetry Data", tFrame->tData, tFrame->tDataSize);
    
    // Initialize telemetryParamFrame with defined constants
    telemetryParamFrame *tParamFrame = initParamFrame();
    
    updateTelemParamData(
        tParamFrame,
        ANALOG_CHANNEL1_NAME,  // "AbsTemp" 
        ANALOG_CHANNEL2_NAME,  // "rtcTime"
        ANALOG_CHANNEL3_NAME,  // "Weight"
        ANALOG_CHANNEL4_NAME,  // "Charge"
        ANALOG_CHANNEL5_NAME,  // "aDist"
        DIGITAL_CHANNEL1_NAME, // "Energy"
        DIGITAL_CHANNEL2_NAME, // "HeatE"
        DIGITAL_CHANNEL3_NAME, // "AMPS"
        DIGITAL_CHANNEL4_NAME, // "Work"
        DIGITAL_CHANNEL5_NAME, // "Oppo"
        DIGITAL_CHANNEL6_NAME, // "LUX"
        DIGITAL_CHANNEL7_NAME, // "BEC"
        DIGITAL_CHANNEL8_NAME  // "RAD"
    );
    // Concatenate parameter data
    concatParamData(tParamFrame);
   
   
    telemetryUnitFrame *tUnitFrame = initUnitFrame();

    updateTelemUnitData(
        tUnitFrame,
        ANALOG_CHANNEL1_UNIT,  // "Meter"
        ANALOG_CHANNEL2_UNIT,  // "Second"
        ANALOG_CHANNEL3_UNIT,  // "Gram"
        ANALOG_CHANNEL4_UNIT,  // "Ampere"
        ANALOG_CHANNEL5_UNIT,  // "Kelvin"
        DIGITAL_CHANNEL1_UNIT, // "Volt"
        DIGITAL_CHANNEL2_UNIT, // "Ohm"
        DIGITAL_CHANNEL3_UNIT, // "Joule"
        DIGITAL_CHANNEL4_UNIT, // "Watt"
        DIGITAL_CHANNEL5_UNIT, // "Tesla"
        DIGITAL_CHANNEL6_UNIT, // "Lux"
        DIGITAL_CHANNEL7_UNIT, // "Bec"
        DIGITAL_CHANNEL8_UNIT  // "Rad"
        );

    concatUnitData(tUnitFrame);


    messageFrame *mFrame = initMFrame(); 
    updateMessageData(
        mFrame,
        "APRS",
        "Hello World!"
    );
    concatMessageData(mFrame);

    positionFrame *pFrame = initPFrame();
    updatePositionData(
        pFrame,
        "123456",
        "1234.56N",
        "12345.67W",
        "Hello World!"
    );
    concatPositionData(pFrame);


    iFrame *frame = initFrame(tFrame->tData, tFrame->tDataSize);
    hdlcFrame infoResult = processFrameVerbose(frame);

   // iFrame *paramFrame = initFrame(tParamFrame->paramFrame, tParamFrame->paramFrameSize);
    //hdlcFrame paramResult = processFrameVerbose(paramFrame);
    
    int totalBits = infoResult.size;
    int totalBytes = (totalBits + 7) / 8;  

    for (int i = 0; i < totalBytes; i++) {
        uint8_t byte = 0;

        for (int bit = 0; bit < 8; bit++) {
            int bitIndex = i * 8 + bit;
            if (bitIndex >= totalBits) break;

            byte <<= 1;
            byte |= infoResult.nrziBinHdlcFrame[bitIndex] & 0x01;
        }

        printf("%02X ", (unsigned int)byte);
    }

    printf("\n");


    // Print the  bitstream
    printf("hdlc Bitstream: ");
    for (size_t i = 0; i < infoResult.size; i++)
    {
        printf("%d", infoResult.nrziBinHdlcFrame[i]);
        // if ((i + 1) % 8 == 0) { // Print a space after every 8 bits (1 byte)
        printf(", ");
        // }
    }
    printf("\n");

    printf("hdlc Bitstream Size: %ld\n", infoResult.size);
    
    printf("--------------------------------------------\n");
    printf("Parameter Frame\n");
    printf("---------------------------------------------\n");

     // Print the  bitstream
     /*
    printf("hdlc Bitstream: ");
    for (size_t i = 0; i < paramResult.size; i++)
    {
        printf("%d", paramResult.nrziBinHdlcFrame[i]);
        // if ((i + 1) % 8 == 0) { // Print a space after every 8 bits (1 byte)
        printf(", ");
        // }
    }
    printf("\n");

    printf("hdlc Bitstream Size: %ld\n", paramResult.size);
    */
     //Free the allocated memory for the NRZI bitstream
    free(infoResult.nrziBinHdlcFrame);
    //free(paramResult.nrziBinHdlcFrame);
    free(tFrame->tData);
    free(tFrame);
    free(tParamFrame->paramFrame);
   free(tParamFrame);
    free(tUnitFrame->unitFrame);
   free(tUnitFrame);
   free(mFrame->messageFrame);
   free(mFrame);
   free(pFrame->positionFrame);
    free(pFrame);

    return 0;
}
