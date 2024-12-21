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
#include "buzzer.h"

#define ESPNOW_QUEUE_SIZE 1
#define ESPNOW_CHANNEL 1 // range 0 to 14
#define ESPNOW_ENABLE_LONG_RANGE false

#define FRAMING_CHAR '$'
static const char *TAG = "espnow";
static const char *BIND_MSG_TX = "TXTXTX"; // size of payload is 6
// static const char *BIND_MSG_rX = "RXRXRX"; // size of payload is 6

static TaskHandle_t Handle_espnow_send_task;
static QueueHandle_t espnow_re_queue;
static bool is_binding_mode = false;
static uint16_t chanl_data[6];

static const uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t local_mac[ESP_NOW_ETH_ALEN] = {};

bool isBinding(void)
{
    return is_binding_mode;
}

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
    // if (status != ESP_NOW_SEND_SUCCESS)
    // {
    //     ESP_LOGE(TAG, "Send error");
    // }
    // else
    // {
    //     ESP_LOGI(TAG, "Send success");
    // }
}

static void espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    // Check if the message is sent to current device by comparing the mac address.
    if (memcmp(recv_info->des_addr, local_mac, ESP_NOW_ETH_ALEN))
    {
        // If in binding mode, receive broadcast data.
        if (!is_binding_mode || memcmp(recv_info->des_addr, broadcast_mac, ESP_NOW_ETH_ALEN))
        {
            // Ignore unicast message if in bind mode.
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
    if (xQueueSend(espnow_re_queue, &recv_cb, 0) != pdTRUE)
    {
        // ESP_LOGW(TAG, "Send receive queue fail");
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

/**
 * @brief Send channel data to esp now task.
 * @param chanl_till: Till pwm data range 0-2500
 * @param chanl_roll: Roll pwm data range 0-2500
 * @param chanl_pan: Pan pwm data range 0-2500
 */
void espnow_data_prepare(uint16_t chanl_till, uint16_t chanl_roll, uint16_t chanl_pan)
{
    chanl_data[0] = chanl_till;
    chanl_data[1] = chanl_roll;
    chanl_data[2] = chanl_pan;
}

#define ESPNOW_NVS_NAMESPACE "esp_now"
#define ESPNOW_NVS_PEER_KEY "peer_mac"

// Save the ESP-NOW peer information in NVS
esp_err_t esp_now_save_peer(esp_now_peer_info_t *peer)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(ESPNOW_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open NVS");
        return err;
    }

    err = nvs_set_blob(nvs_handle, ESPNOW_NVS_PEER_KEY, peer, sizeof(esp_now_peer_info_t));
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to write peer info into NVS");
        return err;
    }

    // Commit
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "Successfully write peer info into NVS");
        return err;
    }

    // Close
    nvs_close(nvs_handle);
    return ESP_OK;
}

/**
 * @brief Recover ESP-NOW peer fome NVS
 * @return peer_addr: the peer address recovered from nvs
 */
uint8_t *esp_now_restore_peer(void)
{
    uint8_t *addr_ret;
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(ESPNOW_NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open NVS");
        return NULL;
    }

    esp_now_peer_info_t *peer_info;
    size_t peer_size = sizeof(esp_now_peer_info_t);
    peer_info = malloc(sizeof(esp_now_peer_info_t));
    // read peer from nvs.
    ret = nvs_get_blob(nvs_handle, ESPNOW_NVS_PEER_KEY, peer_info, &peer_size);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read peer from NVS");
    }
    else
    {
        // recover peer information
        if (!esp_now_is_peer_exist(peer_info->peer_addr))
        {
            // add peer if not in ram yet.
            ret = esp_now_add_peer(peer_info);
            if (ret != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to add peer");
            }
            else
            {
                ESP_LOGI(TAG, "Restored peer: " MACSTR "", MAC2STR(peer_info->peer_addr));
            }
        }
    }

    // copy the mac address to be return.
    addr_ret = malloc(ESP_NOW_ETH_ALEN);
    memcpy(addr_ret, peer_info->peer_addr, ESP_NOW_ETH_ALEN);

    free(peer_info);
    nvs_close(nvs_handle);

    return addr_ret;
}

static void espnow_send_task()
{
    uint8_t *peer_addr;
    esp_err_t err;
    espnow_frame_t frame;
    TickType_t xLastWakeTime;

    // Initialise the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount();

    // Get peer information.
    peer_addr = esp_now_restore_peer();
    if (peer_addr == NULL)
    {
        ESP_LOGE(TAG, "ESPNOW peer not found.");
        Handle_espnow_send_task = NULL;
        vTaskDelete(NULL);
    }

    for (;;)
    {
        memcpy(frame.payload, chanl_data, sizeof(frame.payload));
        frame.framing_char = FRAMING_CHAR;
        frame.function = ESPNOW_FUNCTION_GET_DATA;
        frame.crc_8 = espnow_crc(&frame);

        err = esp_now_send(peer_addr, (uint8_t *)&frame, sizeof(frame));
        // if (err)
        // {
        //     ESP_LOGE(TAG, "ESPNOW send failed %d.", err);
        // }
        xTaskDelayUntil(&xLastWakeTime, ESPNOW_SEND_PERIOD);
    }
}

static void espnow_bind_task()
{
    espnow_event_recv_cb_t recv_cb;
    espnow_frame_t frame;
    bool success_flag = false;

    if (!is_binding_mode)
    {
        vTaskDelete(NULL);
    }

    // Delete send task before running bind task.
    if (Handle_espnow_send_task != NULL)
    {
        vTaskDelete(Handle_espnow_send_task);
        Handle_espnow_send_task = NULL;
    }

    // Add broadcast peer information to peer list if in binding mode.
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    /* Add broadcast peer information to peer list. */
    if (peer == NULL)
    {
        ESP_LOGE(TAG, "Malloc peer information fail");
        vSemaphoreDelete(espnow_re_queue);
        esp_now_deinit();
        return;
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

    // Fill binding message.
    frame.framing_char = FRAMING_CHAR;
    frame.function = ESPNOW_FUNCTION_BIND;
    memcpy(frame.payload, BIND_MSG_TX, sizeof(frame.payload));
    frame.crc_8 = espnow_crc(&frame);

    ESP_LOGI(TAG, "Start binding.");

    for (;;)
    {
        // Send binding message.
        esp_now_send(broadcast_mac, (uint8_t *)&frame, sizeof(espnow_frame_t));
        // Waiting for the other device's binding message.
        memset(&recv_cb, 0, sizeof(espnow_frame_t));
        // Do not bind again after success.
        if (xQueueReceive(espnow_re_queue, &recv_cb, 0) == pdTRUE && !success_flag)
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
                    ESP_LOGI(TAG, "Pair " MACSTR " success.", MAC2STR(peer.peer_addr));
                    // save peer info into nvs.
                    esp_now_save_peer(&peer);
                }
            }
            free(recv_cb.data);
        }
        if (success_flag)
        {
            is_binding_mode = false;
            buzzer_set_state(BUZZER_SINGLE, 1000, 0);
            ESP_LOGI(TAG, "End binding, start sending channal data.");
            xTaskCreate(espnow_send_task, "espnow_send_task", 2048, NULL, PRIORITY_HIGH, &Handle_espnow_send_task);
            vTaskDelete(NULL);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void set_binding_mode(bool true_or_false)
{
    // Only create task once if already in binding mode.
    if (true_or_false && !is_binding_mode)
    {
        is_binding_mode = true_or_false;
        xTaskCreate(espnow_bind_task, "espnow_bind_task", 2048, NULL, PRIORITY_HIGH, NULL);
    }
    is_binding_mode = true_or_false;
}

static esp_err_t espnow_init(void)
{
    espnow_re_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(espnow_event_recv_cb_t));
    if (espnow_re_queue == NULL)
    {
        ESP_LOGE(TAG, "Create mutex fail");
        return ESP_FAIL;
    }

    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));

    xTaskCreate(espnow_send_task, "espnow_send_task", 2048, NULL, PRIORITY_HIGH, &Handle_espnow_send_task);
    return ESP_OK;
}

void ht_espnow_init(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGE(TAG, "nvs_flash_init failed.");
        // ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_read_mac(local_mac, ESP_MAC_WIFI_STA);
    ESP_LOGI(TAG, "Local Mac: " MACSTR "", MAC2STR(local_mac));
    wifi_init();
    espnow_init();
}
