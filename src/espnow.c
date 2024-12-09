#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "esp_random.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_crc.h"
#include "espnow.h"

#include "defines.h"

#define ESPNOW_QUEUE_SIZE 3
#define ESPNOW_CHANNEL 1 // range 0 to 14
#define ESPNOW_ENABLE_LONG_RANGE false

#define FRAMING_CHAR '$'
static const char *TAG = "espnow";
static const char *BIND_MSG_TX = "TXTXTX"; // size of payload is 6
// static const char *BIND_MSG_rX = "RXRXRX"; // size of payload is 6

static QueueHandle_t espnow_queue;
static bool is_binding_mode = false;

void set_binding_mode(bool true_or_false)
{
    is_binding_mode = true_or_false;
}

static const uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t local_mac[ESP_NOW_ETH_ALEN] = {};

/* WiFi should start before using ESPNOW */
static void wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));

#if ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK(esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR));
#endif
}

static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if (status != ESP_NOW_SEND_SUCCESS)
    {
        ESP_LOGE(TAG, "Send error");
    }
}

static void espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    // Check if the message is sent to current device by comparing the mac address.
    if (memcmp(recv_info->des_addr, local_mac, ESP_NOW_ETH_ALEN))
    {
        // If in binding mode, receive broadcast data.
        if (is_binding_mode && !memcmp(recv_info->des_addr, broadcast_mac, ESP_NOW_ETH_ALEN))
        {
            /* In binding mode, do not return */
        }
        else
        {
            ESP_LOGI(TAG, "Message MAC address does not match");
            return;
        }
    }

    espnow_event_recv_cb_t recv_cb;

    memcpy(recv_cb.src_addr, recv_info->src_addr, ESP_NOW_ETH_ALEN);
    recv_cb.data = malloc(len);
    if (recv_cb.data == NULL)
    {
        ESP_LOGE(TAG, "Malloc receive data fail");
        return;
    }
    memcpy(recv_cb.data, data, len);
    recv_cb.data_len = len;

    // send received data to queue
    if (xQueueSend(espnow_queue, &recv_cb, 0) != pdTRUE)
    {
        ESP_LOGW(TAG, "Send receive queue fail");
        free(recv_cb.data);
    }
}

// Calculate the crc of the espnow frame.
static inline uint8_t espnow_crc(espnow_frame_t *pvFrame)
{
    // initial number: 0
    // calculate all data in frame except crc byte itself.
    return esp_crc8_le(0, (uint8_t *)pvFrame, sizeof(espnow_frame_t) - 1);
}

// Unpair all peers.
static void espnow_unpairAll(void)
{
    esp_now_peer_info_t peer;
    if (!esp_now_fetch_peer(true, &peer))
    {
        esp_now_del_peer(peer.peer_addr);
    }
    while (!esp_now_fetch_peer(false, &peer))
    {
        esp_now_del_peer(peer.peer_addr);
    }
}
/* Parse received ESPNOW data. */
// void espnow_data_parse(uint8_t *data, uint16_t data_len, uint8_t *state, uint16_t *seq, uint32_t *magic)
// {
//     example_espnow_data_t *buf = (example_espnow_data_t *)data;
//     uint16_t crc, crc_cal = 0;

//     if (data_len < sizeof(example_espnow_data_t)) {
//         ESP_LOGE(TAG, "Receive ESPNOW data too short, len:%d", data_len);
//         return -1;
//     }

//     *state = buf->state;
//     *seq = buf->seq_num;
//     *magic = buf->magic;
//     crc = buf->crc;
//     buf->crc = 0;
//     crc_cal = esp_crc16_le(UINT16_MAX, (uint8_t const *)buf, data_len);

//     if (crc_cal == crc) {
//         return buf->type;
//     }

//     return -1;
// }

/* Prepare ESPNOW data to be sent. */
// void example_espnow_data_prepare(example_espnow_send_param_t *send_param)
// {
//     example_espnow_data_t *buf = (example_espnow_data_t *)send_param->buffer;

//     assert(send_param->len >= sizeof(example_espnow_data_t));

//     buf->type = IS_BROADCAST_ADDR(send_param->dest_mac) ? EXAMPLE_ESPNOW_DATA_BROADCAST : EXAMPLE_ESPNOW_DATA_UNICAST;
//     buf->state = send_param->state;
//     buf->seq_num = espnow_seq[buf->type]++;
//     buf->crc = 0;
//     buf->magic = send_param->magic;
//     /* Fill all remaining bytes after the data with random values */
//     esp_fill_random(buf->payload, send_param->len - sizeof(example_espnow_data_t));
//     buf->crc = esp_crc16_le(UINT16_MAX, (uint8_t const *)buf, send_param->len);
// }

static void espnow_bind_task()
{
    espnow_event_recv_cb_t recv_cb;
    espnow_frame_t frame;
    bool success_flag = false;

    // Fill binding message.
    frame.framing_char = FRAMING_CHAR;
    frame.function = ESPNOW_FUNCTION_BIND;
    memcpy(frame.payload, BIND_MSG_TX, sizeof(frame.payload));
    frame.crc_8 = espnow_crc(&frame);

    for (;;)
    {
        // Send binding message.
        ESP_ERROR_CHECK(esp_now_send(broadcast_mac, (uint8_t *)&frame, sizeof(espnow_frame_t)));
        // Waiting for the other device's binding message.
        memset(&recv_cb, 0, sizeof(espnow_frame_t));
        // Do not bind again after success.
        if (xQueueReceive(espnow_queue, &recv_cb, 0) == pdTRUE && !success_flag)
        {
            // Check length.
            if (recv_cb.data_len != sizeof(espnow_frame_t))
            {
                ESP_LOGI(TAG, "Binding message length incorrect.");
            }
            // Check crc.
            else if (espnow_crc((espnow_frame_t *)recv_cb.data) != recv_cb.data[sizeof(espnow_frame_t) - 1])
            {
                ESP_LOGI(TAG, "Binding message crc incorrect.");
            }
            else
            {
                // if the binding message matches, add to peer list.
                if (!memcmp(recv_cb.data, &frame, sizeof(espnow_frame_t)))
                {
                    // Unpair all unicast peers first, make sure only one unicast exist at the same time.
                    espnow_unpairAll();
                    // Add peer to the list.
                    esp_now_peer_info_t peer;
                    memset(&peer, 0, sizeof(esp_now_peer_info_t));
                    memcpy(peer.peer_addr, recv_cb.src_addr, ESP_NOW_ETH_ALEN);
                    peer.channel = ESPNOW_CHANNEL;
                    peer.ifidx = WIFI_IF_STA;
                    peer.encrypt = false;
                    ESP_ERROR_CHECK(esp_now_add_peer(&peer));
                    success_flag = true;
                    ESP_LOGI(TAG, "Pair "MACSTR" success.", MAC2STR(peer.peer_addr));
                }
            }
            free(recv_cb.data);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void espnow_task()
{
    ESP_LOGI(TAG, "Running espnow_task.");
    vTaskDelete(NULL);
//     espnow_event_recv_cb_t recv_cb;

//     uint8_t recv_state = 0;
//     uint16_t recv_seq = 0;
//     uint32_t recv_magic = 0;
//     bool is_broadcast = false;
//     int ret;

//     /* Start sending broadcast ESPNOW data. */
//     if (esp_now_send(send_param->dest_mac, send_param->buffer, send_param->len) != ESP_OK)
//     {
//         ESP_LOGE(TAG, "Send error");
//         example_espnow_deinit(send_param);
//         vTaskDelete(NULL);
//     }

//     while (xQueueReceive(espnow_queue, &recv_cb, portMAX_DELAY) == pdTRUE)
//     {
//         switch (evt.id)
//         {
//         case EXAMPLE_ESPNOW_SEND_CB:
//         {
//             example_espnow_event_send_cb_t *send_cb = &evt.info.send_cb;
//             is_broadcast = IS_BROADCAST_ADDR(send_cb->mac_addr);

//             ESP_LOGD(TAG, "Send data to " MACSTR ", status1: %d", MAC2STR(send_cb->mac_addr), send_cb->status);

//             if (is_broadcast && (send_param->broadcast == false))
//             {
//                 break;
//             }

//             ESP_LOGI(TAG, "send data to " MACSTR "", MAC2STR(send_cb->mac_addr));

//             memcpy(send_param->dest_mac, send_cb->mac_addr, ESP_NOW_ETH_ALEN);
//             example_espnow_data_prepare(send_param);

//             /* Send the next data after the previous data is sent. */
//             if (esp_now_send(send_param->dest_mac, send_param->buffer, send_param->len) != ESP_OK)
//             {
//                 ESP_LOGE(TAG, "Send error");
//                 example_espnow_deinit(send_param);
//                 vTaskDelete(NULL);
//             }
//             break;
//         }
//         case EXAMPLE_ESPNOW_RECV_CB:
//         {
//             espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;

//             ret = espnow_data_parse(recv_cb->data, recv_cb->data_len, &recv_state, &recv_seq, &recv_magic);
//             free(recv_cb->data);
//             if (ret == EXAMPLE_ESPNOW_DATA_BROADCAST)
//             {
//                 ESP_LOGI(TAG, "Receive %dth broadcast data from: " MACSTR ", len: %d", recv_seq, MAC2STR(recv_cb->mac_addr), recv_cb->data_len);

//                 /* If MAC address does not exist in peer list, add it to peer list. */
//                 if (esp_now_is_peer_exist(recv_cb->mac_addr) == false)
//                 {
//                     esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
//                     if (peer == NULL)
//                     {
//                         ESP_LOGE(TAG, "Malloc peer information fail");
//                         example_espnow_deinit(send_param);
//                         vTaskDelete(NULL);
//                     }
//                     memset(peer, 0, sizeof(esp_now_peer_info_t));
//                     peer->channel = CONFIG_ESPNOW_CHANNEL;
//                     peer->ifidx = ESPNOW_WIFI_IF;
//                     peer->encrypt = true;
//                     memcpy(peer->lmk, CONFIG_ESPNOW_LMK, ESP_NOW_KEY_LEN);
//                     memcpy(peer->peer_addr, recv_cb->mac_addr, ESP_NOW_ETH_ALEN);
//                     ESP_ERROR_CHECK(esp_now_add_peer(peer));
//                     free(peer);
//                 }

//                 /* Indicates that the device has received broadcast ESPNOW data. */
//                 if (send_param->state == 0)
//                 {
//                     send_param->state = 1;
//                 }

//                 /* If receive broadcast ESPNOW data which indicates that the other device has received
//                  * broadcast ESPNOW data and the local magic number is bigger than that in the received
//                  * broadcast ESPNOW data, stop sending broadcast ESPNOW data and start sending unicast
//                  * ESPNOW data.
//                  */
//                 if (recv_state == 1)
//                 {
//                     /* The device which has the bigger magic number sends ESPNOW data, the other one
//                      * receives ESPNOW data.
//                      */
//                     if (send_param->unicast == false && send_param->magic >= recv_magic)
//                     {
//                         ESP_LOGI(TAG, "Start sending unicast data");
//                         ESP_LOGI(TAG, "send data to " MACSTR "", MAC2STR(recv_cb->mac_addr));

//                         /* Start sending unicast ESPNOW data. */
//                         memcpy(send_param->dest_mac, recv_cb->mac_addr, ESP_NOW_ETH_ALEN);
//                         example_espnow_data_prepare(send_param);
//                         if (esp_now_send(send_param->dest_mac, send_param->buffer, send_param->len) != ESP_OK)
//                         {
//                             ESP_LOGE(TAG, "Send error");
//                             example_espnow_deinit(send_param);
//                             vTaskDelete(NULL);
//                         }
//                         else
//                         {
//                             send_param->broadcast = false;
//                             send_param->unicast = true;
//                         }
//                     }
//                 }
//             }
//             else if (ret == EXAMPLE_ESPNOW_DATA_UNICAST)
//             {
//                 ESP_LOGI(TAG, "Receive %dth unicast data from: " MACSTR ", len: %d", recv_seq, MAC2STR(recv_cb->mac_addr), recv_cb->data_len);

//                 /* If receive unicast ESPNOW data, also stop sending broadcast ESPNOW data. */
//                 send_param->broadcast = false;
//             }
//             else
//             {
//                 ESP_LOGI(TAG, "Receive error data from: " MACSTR "", MAC2STR(recv_cb->mac_addr));
//             }
//             break;
//         }
//         default:
//             ESP_LOGE(TAG, "Callback type error: %d", evt.id);
//             break;
//         }
//     }
}

static esp_err_t espnow_init(void)
{
    espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(espnow_event_recv_cb_t));
    if (espnow_queue == NULL)
    {
        ESP_LOGE(TAG, "Create mutex fail");
        return ESP_FAIL;
    }

    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));

    // Add broadcast peer information to peer list if in binding mode.
    if (is_binding_mode)
    {
        esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
        /* Add broadcast peer information to peer list. */
        if (peer == NULL)
        {
            ESP_LOGE(TAG, "Malloc peer information fail");
            vSemaphoreDelete(espnow_queue);
            esp_now_deinit();
            return ESP_FAIL;
        }
        memset(peer, 0, sizeof(esp_now_peer_info_t));
        peer->channel = ESPNOW_CHANNEL;
        peer->ifidx = WIFI_IF_STA;
        peer->encrypt = false;
        memcpy(peer->peer_addr, broadcast_mac, ESP_NOW_ETH_ALEN);
        // Delete the old broadcast peer info before adding.
        esp_now_del_peer(broadcast_mac);
        ESP_ERROR_CHECK(esp_now_add_peer(peer));
        free(peer);

        xTaskCreate(espnow_bind_task, "espnow_bind_task", 2048, NULL, PRIORITY_HIGH, NULL);
    }
    else
    {
        xTaskCreate(espnow_task, "espnow_task", 2048, NULL, PRIORITY_HIGH, NULL);
    }
    return ESP_OK;
}

void ht_espnow_init(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_read_mac(local_mac, ESP_MAC_WIFI_STA);
    ESP_LOGI(TAG, "Local Mac: " MACSTR "", MAC2STR(local_mac));
    wifi_init();
    espnow_init();
}
