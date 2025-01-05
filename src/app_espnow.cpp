#ifdef FRAMEWORK_ARDUINO
#include <ESP8266WiFi.h>
#include "app_espnow.h"
#include <espnow.h>
#include <Ticker.h>
#include "receiver.h"
#include "ppm.h"
#include "trackersettings.h"
extern "C"
{
#include "crc8.h"
}

#define ESPNOW_QUEUE_SIZE 1
#define ESPNOW_CHANNEL 1 // range 0 to 14
#define ESPNOW_ENABLE_LONG_RANGE false

#define FRAMING_CHAR '$'
static const char *BIND_MSG_TX = "TXTXTX"; // size of payload is 6
#define ESP_NOW_ETH_ALEN 6

static bool is_binding_mode = false;
static uint16_t chanl_data[6];

static uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t source_mac[ESP_NOW_ETH_ALEN] = {};

static espnow_frame_t recv_cb_data;  // Received data from espnow.
static bool is_recv_cb_data = false; // Flag to indicate if received data is the latest.

static Ticker LED_blink_timer;

bool isBinding(void)
{
    return is_binding_mode;
}

static void espnow_send_cb(u8 *mac_addr, u8 status)
{
}

static void espnow_recv_cb(u8 *mac_addr, u8 *data, u8 len)
{
    // Ignore if the length of the received data is not correct.
    if (len != sizeof(espnow_frame_t))
    {
        Serial.println("ESPNOW receive data length incorrect.");
        return;
    }

    if (is_recv_cb_data)
    {
        Serial.println("Last received data not processed yet.");
        return;
    }

    // Only copy mac address in binding mode.
    if (isBinding())
    {
        memcpy(source_mac, mac_addr, ESP_NOW_ETH_ALEN);
    }

    // Copy received data.
    memcpy(&recv_cb_data, data, sizeof(espnow_frame_t));
    is_recv_cb_data = true;
}

// Calculate the crc of the espnow frame.
static inline uint8_t espnow_crc(espnow_frame_t *pvFrame)
{
    // initial number: 0
    // calculate all data in frame except crc byte itself.
    return crc8_calculate((uint8_t *)pvFrame, sizeof(espnow_frame_t) - 1);
}

// Unpair all peers.
static void espnow_unpairAll(void)
{
    uint8_t *peer;
    peer = esp_now_fetch_peer(true); // fetch the first peer.
    while (peer)
    {
        esp_now_del_peer(peer);
        peer = esp_now_fetch_peer(false); // fetch the rest of peers.
    }
}

static void espnow_bind_task()
{
    espnow_frame_t frame;
    bool success_flag = false;

    // Blink led to indicate binding mode.
    LED_blink_timer.attach_ms(200, []()
                              { digitalWrite(GPIO_LED_STATUS, !digitalRead(GPIO_LED_STATUS)); });

    // Add broadcast peer information to peer list if in binding mode.
    esp_now_add_peer(broadcast_mac, ESP_NOW_ROLE_COMBO, ESPNOW_CHANNEL, NULL, 0);

    // esp_now_del_peer(broadcast_mac);

    // Fill binding message.
    frame.framing_char = FRAMING_CHAR;
    frame.function = ESPNOW_FUNCTION_BIND;
    memcpy(frame.payload, BIND_MSG_TX, sizeof(frame.payload));
    frame.crc_8 = espnow_crc(&frame);

    Serial.println("Start binding.");

    for (;;)
    {
        // Send binding message.
        esp_now_send(broadcast_mac, (uint8_t *)&frame, sizeof(espnow_frame_t));
        // Waiting for the other device's binding message.
        delay(100);
        // Do not bind again after success.
        if (is_recv_cb_data && !success_flag)
        {
            // Check crc.
            if (espnow_crc(&recv_cb_data) != recv_cb_data.crc_8)
            {
                Serial.println("Binding message crc incorrect.");
            }
            else
            {
                // if the binding message matches, add to peer list.
                if (!memcmp(&recv_cb_data, &frame, sizeof(espnow_frame_t)))
                {
                    // Unpair all unicast peers first, make sure only one unicast exist at the same time.
                    espnow_unpairAll();
                    // Add peer to the list.
                    esp_now_add_peer(source_mac, ESP_NOW_ROLE_COMBO, ESPNOW_CHANNEL, NULL, 0);
                    success_flag = true;
                    Serial.println("Bind success.");
                    // save peer info into nvs.
                }
            }
        }
        if (success_flag)
        {
            is_binding_mode = false;
            Serial.println("End binding, start sending channal data.");
            // Stop blinking led.
            LED_blink_timer.detach();
            digitalWrite(GPIO_LED_STATUS, 0);
            break;
        }
    }
}

void set_binding_mode(bool true_or_false)
{
    is_binding_mode = true_or_false;
}

void rx_espnow_init(void)
{
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // Init ESP-NOW
    if (esp_now_init() != 0)
    {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    else
    {
        Serial.println("esp_now init success");
    }

    // Set ESP-NOW Role
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

    // Once ESPNow is successfully Init, we will register for Send CB
    esp_now_register_send_cb(espnow_send_cb);
    // Register for a callback function that will be called when data is received
    esp_now_register_recv_cb(espnow_recv_cb);
}

void rx_espnow_loop(void)
{
    if (isBinding())
    {
        espnow_bind_task();
    }
    else
    {
        // Prase channel data and send to ppm.
        if (is_recv_cb_data)
        {
            // Check crc.
            if (espnow_crc(&recv_cb_data) != recv_cb_data.crc_8)
            {
                Serial.println("Binding message crc incorrect.");
            }
            else
            {
                if (recv_cb_data.function == ESPNOW_FUNCTION_GET_DATA)
                {
                    // Prase channel data.
                    uint16_t chanl_till = (recv_cb_data.payload[1] << 8) | recv_cb_data.payload[0];
                    uint16_t chanl_roll = (recv_cb_data.payload[3] << 8) | recv_cb_data.payload[2];
                    uint16_t chanl_pan = (recv_cb_data.payload[5] << 8) | recv_cb_data.payload[4];
                    // Send channel data to ppm.
                    PpmOut_setChannel(getTiltChl(), chanl_till);
                    PpmOut_setChannel(getRollChl(), chanl_roll);
                    PpmOut_setChannel(getPanChl(), chanl_pan);
                    Serial.printf("%d %d %d\n", PpmOut_getChannel(getTiltChl()), PpmOut_getChannel(getRollChl()), PpmOut_getChannel(getPanChl()));
                }
            }
            is_recv_cb_data = false;
        }
    }
}

#endif