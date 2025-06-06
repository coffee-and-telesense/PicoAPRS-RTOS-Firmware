/**
 * @file app_events.h
 * @brief Event definitions and event group management for Pico Balloon application
 * 
 * This file defines all event flags used throughout the application for
 * inter-thread communication and coordination. Events are organized into
 * logical groups to optimize performance and maintainability.
 * 
 * @author Pico Balloon Team
 * @date 2025
 */

#pragma once


/*******************************************************************************
 * Event Group Declarations
 ******************************************************************************/

/* Global event groups used throughout the application */
extern TX_EVENT_FLAGS_GROUP system_events;      /* System-wide events */
extern TX_EVENT_FLAGS_GROUP sensor_events;      /* Sensor module events */
extern TX_EVENT_FLAGS_GROUP comm_events;        /* Communication events */
extern TX_EVENT_FLAGS_GROUP power_events;       /* Power management events */
extern TX_EVENT_FLAGS_GROUP storage_events;     /* Storage and logging events */

/*******************************************************************************
 * System Events (system_events)
 ******************************************************************************/

/* System State Events */
#define SYS_EVENT_INIT_COMPLETE         (1UL << 0)   /* System initialization done */
#define SYS_EVENT_SHUTDOWN_REQUEST      (1UL << 1)   /* System shutdown requested */
#define SYS_EVENT_ERROR_OCCURRED        (1UL << 2)   /* System error detected */
#define SYS_EVENT_RECOVERY_NEEDED       (1UL << 3)   /* Error recovery required */

/* Mission Events */
#define SYS_EVENT_MISSION_START         (1UL << 4)   /* Mission sequence start */
#define SYS_EVENT_MISSION_PAUSE         (1UL << 5)   /* Mission sequence pause */
#define SYS_EVENT_MISSION_RESUME        (1UL << 6)   /* Mission sequence resume */
#define SYS_EVENT_MISSION_ABORT         (1UL << 7)   /* Mission abort signal */

/* Timing Events */
#define SYS_EVENT_TELEMETRY_CYCLE       (1UL << 8)   /* Telemetry transmission cycle */
#define SYS_EVENT_SENSOR_CYCLE          (1UL << 9)   /* Sensor reading cycle */
#define SYS_EVENT_GPS_CYCLE             (1UL << 10)  /* GPS position update cycle */
#define SYS_EVENT_HOUSEKEEPING_CYCLE    (1UL << 11)  /* System maintenance cycle */

/* External Triggers */
#define SYS_EVENT_USER_BUTTON           (1UL << 12)  /* User button pressed */
#define SYS_EVENT_EXTERNAL_TRIGGER      (1UL << 13)  /* External trigger input */
#define SYS_EVENT_TIMER_EXPIRED         (1UL << 14)  /* General purpose timer */
#define SYS_EVENT_WATCHDOG_RESET        (1UL << 15)  /* Watchdog reset occurred */

/* System event mask for all events */
#define SYS_EVENT_ALL_MASK              (0x0000FFFFUL)

/*******************************************************************************
 * Sensor Events (sensor_events)
 ******************************************************************************/

/* BME68x Sensor Events */
#define SENSOR_EVENT_BME_INIT_DONE      (1UL << 0)   /* BME68x initialization complete */
#define SENSOR_EVENT_BME_READ_READY     (1UL << 1)   /* BME68x data ready to read */
#define SENSOR_EVENT_BME_READ_COMPLETE  (1UL << 2)   /* BME68x reading completed */
#define SENSOR_EVENT_BME_ERROR          (1UL << 3)   /* BME68x sensor error */

/* Future Sensor Extensions */
#define SENSOR_EVENT_SECONDARY_READY    (1UL << 4)   /* Secondary sensor ready */
#define SENSOR_EVENT_CALIBRATION_NEEDED (1UL << 5)   /* Sensor calibration required */
#define SENSOR_EVENT_THRESHOLD_EXCEEDED (1UL << 6)   /* Sensor threshold exceeded */

/* Sensor Processing Events */
#define SENSOR_EVENT_DATA_PROCESSED     (1UL << 8)   /* Sensor data processing done */
#define SENSOR_EVENT_DATA_VALIDATED     (1UL << 9)   /* Sensor data validation done */
#define SENSOR_EVENT_ANOMALY_DETECTED   (1UL << 10)  /* Data anomaly detected */

/* Sensor event mask */
#define SENSOR_EVENT_ALL_MASK           (0x000007FFUL)

/*******************************************************************************
 * Communication Events (comm_events)
 ******************************************************************************/

/* GPS Events */
#define COMM_EVENT_GPS_INIT_DONE        (1UL << 0)   /* GPS initialization complete */
#define COMM_EVENT_GPS_FIX_ACQUIRED     (1UL << 1)   /* GPS fix acquired */
#define COMM_EVENT_GPS_FIX_LOST         (1UL << 2)   /* GPS fix lost */
#define COMM_EVENT_GPS_DATA_READY       (1UL << 3)   /* GPS position data ready */
#define COMM_EVENT_GPS_ERROR            (1UL << 4)   /* GPS communication error */

/* Radio/APRS Events */
#define COMM_EVENT_RADIO_INIT_DONE      (1UL << 8)   /* Radio initialization complete */
#define COMM_EVENT_RADIO_TX_START       (1UL << 9)   /* Radio transmission started */
#define COMM_EVENT_RADIO_TX_COMPLETE    (1UL << 10)  /* Radio transmission complete */
#define COMM_EVENT_RADIO_TX_FAILED      (1UL << 11)  /* Radio transmission failed */
#define COMM_EVENT_RADIO_BUSY           (1UL << 12)  /* Radio busy/in use */

/* Communication Protocol Events */
#define COMM_EVENT_PACKET_PREPARED      (1UL << 16)  /* Packet ready for transmission */
#define COMM_EVENT_ACK_RECEIVED         (1UL << 17)  /* Acknowledgment received */
#define COMM_EVENT_TIMEOUT_OCCURRED     (1UL << 18)  /* Communication timeout */
#define COMM_EVENT_RETRY_NEEDED         (1UL << 19)  /* Transmission retry needed */

/* Communication event mask */
#define COMM_EVENT_ALL_MASK             (0x000FFFFFUL)

/*******************************************************************************
 * Power Events (power_events)
 ******************************************************************************/

/* Power State Events */
#define POWER_EVENT_NORMAL_MODE         (1UL << 0)   /* Normal operation mode */
#define POWER_EVENT_BURST_MODE          (1UL << 1)   /* Burst processing mode */
#define POWER_EVENT_STOP2_MODE          (1UL << 2)   /* Stop 2 low power mode */
#define POWER_EVENT_CRITICAL_MODE       (1UL << 3)   /* Critical power mode */
#define POWER_EVENT_STANDBY_MODE        (1UL << 4)   /* Standby/sleep mode */

/* Battery Events */
#define POWER_EVENT_BATTERY_LOW         (1UL << 8)   /* Battery level low warning */
#define POWER_EVENT_BATTERY_CRITICAL    (1UL << 9)   /* Battery level critical */
#define POWER_EVENT_BATTERY_EMPTY       (1UL << 10)  /* Battery empty - shutdown */
#define POWER_EVENT_CHARGING_DETECTED   (1UL << 11)  /* Charging source detected */
#define POWER_EVENT_CHARGING_COMPLETE   (1UL << 12)  /* Battery fully charged */

/* Power Management Events */
#define POWER_EVENT_SLEEP_REQUEST       (1UL << 16)  /* Request to enter sleep */
#define POWER_EVENT_WAKEUP_REQUEST      (1UL << 17)  /* Request to wake up */
#define POWER_EVENT_POWER_BUDGET_UPDATE (1UL << 18)  /* Power budget recalculation */
#define POWER_EVENT_THERMAL_WARNING     (1UL << 19)  /* Thermal protection warning */

/* Power event mask */
#define POWER_EVENT_ALL_MASK            (0x000FFFFFUL)

/*******************************************************************************
 * Storage Events (storage_events)
 ******************************************************************************/

/* Storage Operations */
#define STORAGE_EVENT_INIT_DONE         (1UL << 0)   /* Storage initialization done */
#define STORAGE_EVENT_WRITE_READY       (1UL << 1)   /* Ready to write data */
#define STORAGE_EVENT_WRITE_COMPLETE    (1UL << 2)   /* Write operation complete */
#define STORAGE_EVENT_READ_COMPLETE     (1UL << 3)   /* Read operation complete */
#define STORAGE_EVENT_ERROR_OCCURRED    (1UL << 4)   /* Storage error occurred */

/* Storage Management */
#define STORAGE_EVENT_SPACE_LOW         (1UL << 8)   /* Storage space low */
#define STORAGE_EVENT_SPACE_FULL        (1UL << 9)   /* Storage space full */
#define STORAGE_EVENT_CLEANUP_NEEDED    (1UL << 10)  /* Cleanup/garbage collection */
#define STORAGE_EVENT_BACKUP_COMPLETE   (1UL << 11)  /* Data backup complete */

/* Storage event mask */
#define STORAGE_EVENT_ALL_MASK          (0x00000FFFUL)

/*******************************************************************************
 * Event Helper Macros
 ******************************************************************************/

/* Common event wait options */
#define EVENT_WAIT_FOREVER              TX_WAIT_FOREVER
#define EVENT_WAIT_NO_BLOCK             TX_NO_WAIT
#define EVENT_WAIT_SHORT                (100)   /* 100ms timeout */
#define EVENT_WAIT_MEDIUM               (1000)  /* 1 second timeout */
#define EVENT_WAIT_LONG                 (5000)  /* 5 second timeout */

/* Event flag retrieval options */
#define EVENT_GET_ANY                   TX_OR
#define EVENT_GET_ALL                   TX_AND
#define EVENT_GET_CLEAR                 TX_OR_CLEAR
#define EVENT_GET_ALL_CLEAR             TX_AND_CLEAR

/*******************************************************************************
 * Event Management Functions
 ******************************************************************************/

/**
 * @brief Initialize all event groups
 * @return TX_SUCCESS on success, error code otherwise
 */
UINT app_events_init(void);

/**
 * @brief Cleanup all event groups
 * @return TX_SUCCESS on success, error code otherwise
 */
UINT app_events_cleanup(void);

/**
 * @brief Set events in a specific event group
 * @param group Pointer to event group
 * @param events Events to set
 * @return TX_SUCCESS on success, error code otherwise
 */
UINT app_events_set(TX_EVENT_FLAGS_GROUP *group, ULONG events);

/**
 * @brief Wait for events in a specific event group
 * @param group Pointer to event group
 * @param requested_events Events to wait for
 * @param get_option How to retrieve events (AND/OR/CLEAR)
 * @param actual_events Pointer to store actual events received
 * @param wait_option How long to wait
 * @return TX_SUCCESS on success, error code otherwise
 */
UINT app_events_wait(TX_EVENT_FLAGS_GROUP *group, 
                     ULONG requested_events,
                     UINT get_option,
                     ULONG *actual_events,
                     ULONG wait_option);

/**
 * @brief Clear specific events in an event group
 * @param group Pointer to event group
 * @param events Events to clear
 * @return TX_SUCCESS on success, error code otherwise
 */
UINT app_events_clear(TX_EVENT_FLAGS_GROUP *group, ULONG events);

/**
 * @brief Get current event flags without clearing
 * @param group Pointer to event group
 * @param current_events Pointer to store current events
 * @return TX_SUCCESS on success, error code otherwise
 */
UINT app_events_get_current(TX_EVENT_FLAGS_GROUP *group, ULONG *current_events);

/*******************************************************************************
 * Debug and Monitoring Functions
 ******************************************************************************/

#ifdef DEBUG
/**
 * @brief Print current status of all event groups
 */
void app_events_debug_print_status(void);

/**
 * @brief Get a string representation of system events
 * @param events Event flags to decode
 * @param buffer Buffer to store string
 * @param buffer_size Size of buffer
 */
void app_events_decode_system_events(ULONG events, char *buffer, size_t buffer_size);

/**
 * @brief Get a string representation of sensor events
 * @param events Event flags to decode
 * @param buffer Buffer to store string
 * @param buffer_size Size of buffer
 */
void app_events_decode_sensor_events(ULONG events, char *buffer, size_t buffer_size);

/**
 * @brief Get a string representation of communication events
 * @param events Event flags to decode
 * @param buffer Buffer to store string
 * @param buffer_size Size of buffer
 */
void app_events_decode_comm_events(ULONG events, char *buffer, size_t buffer_size);

/**
 * @brief Get a string representation of power events
 * @param events Event flags to decode
 * @param buffer Buffer to store string
 * @param buffer_size Size of buffer
 */
void app_events_decode_power_events(ULONG events, char *buffer, size_t buffer_size);

/**
 * @brief Get a string representation of storage events
 * @param events Event flags to decode
 * @param buffer Buffer to store string
 * @param buffer_size Size of buffer
 */
void app_events_decode_storage_events(ULONG events, char *buffer, size_t buffer_size);
