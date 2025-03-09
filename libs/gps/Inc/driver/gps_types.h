#pragma once

typedef enum {
    UBLOX_OK = 0x00,             // Operation successful
    UBLOX_ERROR = 0x01,          // General error
    UBLOX_INVALID_PARAM = 0x02,  // Invalid parameter provided
    UBLOX_TIMEOUT = 0x03,        // Operation timed out
    UBLOX_CHECKSUM_ERR = 0x04,   // Checksum verification failed
    // Add more error codes as needed
} gps_status_e;