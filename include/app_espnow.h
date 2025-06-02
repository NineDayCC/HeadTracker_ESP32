#ifndef APP_ESPNOW_H
#define APP_ESPNOW_H

#include "defines.h"

#ifndef FRAMEWORK_ARDUINO
#include "esp_now.h"

typedef struct {
    uint8_t src_addr[ESP_NOW_ETH_ALEN];
    uint8_t *data;
    int data_len;
} espnow_event_recv_cb_t;
#endif

typedef struct {
    uint8_t framing_char;   // "$" Framing magic start char
    uint8_t function;
    uint8_t payload[6];
    uint8_t crc_8;          // from framing_char to payload, 8 byte in total
} __attribute__((packed)) espnow_frame_t;

typedef enum {
    ESPNOW_FUNCTION_BIND = 0,
    ESPNOW_FUNCTION_GET_DATA,
} espnow_frame_function_t;

#define ESPNOW_THREAD_PRIORITY_SET      ESPNOW_THREAD_PRIORITY  //thread priority
#define ESPNOW_THREAD_STACK_SIZE_SET    ESPNOW_THREAD_STACK_SIZE  //thread stack size

void set_binding_flag(bool true_or_false);
void set_binding_mode(bool true_or_false);
void espnow_data_prepare(uint16_t chanl_till, uint16_t chanl_roll, uint16_t chanl_pan);
void ht_espnow_init(void);
void ht_espnow_deinit(void);
bool isBinding(void);
bool isconnected(void);
void rx_espnow_init(void);
void rx_espnow_deinit(void);
void rx_espnow_loop(void);

#endif
