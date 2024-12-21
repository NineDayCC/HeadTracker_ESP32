#ifndef ESPNOW_H
#define ESPNOW_H

#include "esp_now.h"

typedef struct {
    uint8_t src_addr[ESP_NOW_ETH_ALEN];
    uint8_t *data;
    int data_len;
} espnow_event_recv_cb_t;

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

void set_binding_mode(bool true_or_false);
void espnow_data_prepare(uint16_t chanl_till, uint16_t chanl_roll, uint16_t chanl_pan);
void ht_espnow_init(void);
bool isBinding(void);

// typedef union {
//     example_espnow_event_send_cb_t send_cb;
//     espnow_event_recv_cb_t recv_cb;
// } example_espnow_event_info_t;

// /* When ESPNOW sending or receiving callback function is called, post event to ESPNOW task. */
// typedef struct {
//     example_espnow_event_id_t id;
//     example_espnow_event_info_t info;
// } example_espnow_event_t;

// enum {
//     EXAMPLE_ESPNOW_DATA_BROADCAST,
//     EXAMPLE_ESPNOW_DATA_UNICAST,
//     EXAMPLE_ESPNOW_DATA_MAX,
// };

// /* User defined field of ESPNOW data in this example. */
// typedef struct {
//     uint8_t type;                         //Broadcast or unicast ESPNOW data.
//     uint8_t state;                        //Indicate that if has received broadcast ESPNOW data or not.
//     uint16_t seq_num;                     //Sequence number of ESPNOW data.
//     uint16_t crc;                         //CRC16 value of ESPNOW data.
//     uint32_t magic;                       //Magic number which is used to determine which device to send unicast ESPNOW data.
//     uint8_t payload[0];                   //Real payload of ESPNOW data.
// } __attribute__((packed)) example_espnow_data_t;


#endif
