#pragma once 


/* This section contains command map declarations */
struct __attribute__((packed)) si446x_reply_GENERIC_map {
        uint8_t  REPLY[16];
};

struct __attribute__((packed)) si446x_reply_PART_INFO_map {
        uint8_t  CHIPREV;
        uint16_t  PART;
        uint8_t  PBUILD;
        uint16_t  ID;
        uint8_t  CUSTOMER;
        uint8_t  ROMID;
};

struct __attribute__((packed)) si446x_reply_FUNC_INFO_map {
        uint8_t  REVEXT;
        uint8_t  REVBRANCH;
        uint8_t  REVINT;
        uint8_t  FUNC;
};

struct __attribute__((packed)) si446x_reply_GET_PROPERTY_map {
        uint8_t  DATA[16];
};

struct __attribute__((packed)) si446x_reply_GPIO_PIN_CFG_map {
        uint8_t  GPIO[4];
        uint8_t  NIRQ;
        uint8_t  SDO;
        uint8_t  GEN_CONFIG;
};

struct __attribute__((packed)) si446x_reply_FIFO_INFO_map {
        uint8_t  RX_FIFO_COUNT;
        uint8_t  TX_FIFO_SPACE;
};

struct __attribute__((packed)) si446x_reply_GET_INT_STATUS_map {
        uint8_t  INT_PEND;
        uint8_t  INT_STATUS;
        uint8_t  PH_PEND;
        uint8_t  PH_STATUS;
        uint8_t  MODEM_PEND;
        uint8_t  MODEM_STATUS;
        uint8_t  CHIP_PEND;
        uint8_t  CHIP_STATUS;
};

struct __attribute__((packed)) si446x_reply_REQUEST_DEVICE_STATE_map {
        uint8_t  CURR_STATE;
        uint8_t  CURRENT_CHANNEL;
};

struct __attribute__((packed)) si446x_reply_READ_CMD_BUFF_map {
        uint8_t  BYTE[16];
};

struct __attribute__((packed)) si446x_reply_FRR_A_READ_map {
        uint8_t  FRR_A_VALUE;
        uint8_t  FRR_B_VALUE;
        uint8_t  FRR_C_VALUE;
        uint8_t  FRR_D_VALUE;
};

struct __attribute__((packed)) si446x_reply_FRR_B_READ_map {
        uint8_t  FRR_B_VALUE;
        uint8_t  FRR_C_VALUE;
        uint8_t  FRR_D_VALUE;
        uint8_t  FRR_A_VALUE;
};

struct __attribute__((packed)) si446x_reply_FRR_C_READ_map {
        uint8_t  FRR_C_VALUE;
        uint8_t  FRR_D_VALUE;
        uint8_t  FRR_A_VALUE;
        uint8_t  FRR_B_VALUE;
};

struct __attribute__((packed)) si446x_reply_FRR_D_READ_map {
        uint8_t  FRR_D_VALUE;
        uint8_t  FRR_A_VALUE;
        uint8_t  FRR_B_VALUE;
        uint8_t  FRR_C_VALUE;
};

struct __attribute__((packed)) si446x_reply_IRCAL_MANUAL_map {
        uint8_t  IRCAL_AMP_REPLY;
        uint8_t  IRCAL_PH_REPLY;
};

struct __attribute__((packed)) si446x_reply_PACKET_INFO_map {
        uint16_t  LENGTH;
};

struct __attribute__((packed)) si446x_reply_GET_MODEM_STATUS_map {
        uint8_t  MODEM_PEND;
        uint8_t  MODEM_STATUS;
        uint8_t  CURR_RSSI;
        uint8_t  LATCH_RSSI;
        uint8_t  ANT1_RSSI;
        uint8_t  ANT2_RSSI;
        uint16_t  AFC_FREQ_OFFSET;
};

struct __attribute__((packed)) si446x_reply_READ_RX_FIFO_map {
        uint8_t  DATA[2];
};

struct __attribute__((packed)) si446x_reply_GET_ADC_READING_map {
        uint16_t  GPIO_ADC;
        uint16_t  BATTERY_ADC;
        uint16_t  TEMP_ADC;
};

struct __attribute__((packed)) si446x_reply_GET_PH_STATUS_map {
        uint8_t  PH_PEND;
        uint8_t  PH_STATUS;
};

struct __attribute__((packed)) si446x_reply_GET_CHIP_STATUS_map {
        uint8_t  CHIP_PEND;
        uint8_t  CHIP_STATUS;
        uint8_t  CMD_ERR_STATUS;
        uint8_t  CMD_ERR_CMD_ID;
};


/* The union that stores the reply written back to the host registers */
union __attribute__((packed)) si446x_cmd_reply_union {
        uint8_t                                                               RAW[16];
        struct si446x_reply_GENERIC_map                                  GENERIC;
        struct si446x_reply_PART_INFO_map                                PART_INFO;
        struct si446x_reply_FUNC_INFO_map                                FUNC_INFO;
        struct si446x_reply_GET_PROPERTY_map                             GET_PROPERTY;
        struct si446x_reply_GPIO_PIN_CFG_map                             GPIO_PIN_CFG;
        struct si446x_reply_FIFO_INFO_map                                FIFO_INFO;
        struct si446x_reply_GET_INT_STATUS_map                           GET_INT_STATUS;
        struct si446x_reply_REQUEST_DEVICE_STATE_map                     REQUEST_DEVICE_STATE;
        struct si446x_reply_READ_CMD_BUFF_map                            READ_CMD_BUFF;
        struct si446x_reply_FRR_A_READ_map                               FRR_A_READ;
        struct si446x_reply_FRR_B_READ_map                               FRR_B_READ;
        struct si446x_reply_FRR_C_READ_map                               FRR_C_READ;
        struct si446x_reply_FRR_D_READ_map                               FRR_D_READ;
        struct si446x_reply_IRCAL_MANUAL_map                             IRCAL_MANUAL;
        struct si446x_reply_PACKET_INFO_map                              PACKET_INFO;
        struct si446x_reply_GET_MODEM_STATUS_map                         GET_MODEM_STATUS;
        struct si446x_reply_READ_RX_FIFO_map                             READ_RX_FIFO;
        struct si446x_reply_GET_ADC_READING_map                          GET_ADC_READING;
        struct si446x_reply_GET_PH_STATUS_map                            GET_PH_STATUS;
        struct si446x_reply_GET_CHIP_STATUS_map                          GET_CHIP_STATUS;
};