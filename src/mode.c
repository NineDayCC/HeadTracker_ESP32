#ifndef FRAMEWORK_ARDUINO

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "app_espnow.h"
#include "buzzer.h"
#include "ota.h"
#include "imu.h"
#include "mode.h"
#include "led.h"

#define MODE_NVS_NAMESPACE "mode"
#define MODE_NVS_CNT_KEY "cycle_cnt"

#define POWER_CYCLE_WINDOW_US 2 * 1000 * 1000 // 2 seconds
#define OTA_WAIT_US 60 * 1000 * 1000          // 60 seconds
#define BIND_MODE_COUNT 3

#define TASK_DISTROY_TIME_US (OTA_WAIT_US + 500 * 1000)

static const char *TAG = "MODE";
static uint8_t power_cycle_cnt = 0; // count power cycle

// Save the power cycle counter in NVS
esp_err_t save_cycle_cnt(uint8_t cnt)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(MODE_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open NVS");
        return err;
    }

    err = nvs_set_blob(nvs_handle, MODE_NVS_CNT_KEY, &cnt, sizeof(cnt));
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

uint8_t load_cycle_cnt(void)
{
    uint8_t cnt;
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(MODE_NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open NVS");
        return 0;
    }

    size_t cnt_size = sizeof(cnt);
    ret = nvs_get_blob(nvs_handle, MODE_NVS_CNT_KEY, &cnt, &cnt_size);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read peer from NVS");
        return 0;
    }

    nvs_close(nvs_handle);
    return cnt;
}

void mode_Thread(void *pvParameters)
{
    // Load the power cycle count from NVS
    power_cycle_cnt = load_cycle_cnt() < BIND_MODE_COUNT ? load_cycle_cnt() : 0;

    save_cycle_cnt(++power_cycle_cnt); // increment and save the count
    if (power_cycle_cnt == BIND_MODE_COUNT)
    {
        ESP_LOGI(TAG, "Enter binding mode");
        led_set_status(binding);
        set_binding_mode(true); // set binding mode
        vTaskDelete(NULL); // delete this task
    }
    ESP_LOGI(TAG, "Power cycle count: %d", power_cycle_cnt);

    for (;;)
    {
        if (esp_timer_get_time() > POWER_CYCLE_WINDOW_US && power_cycle_cnt != 0)
        {
            power_cycle_cnt = 0; // reset the count
            save_cycle_cnt(power_cycle_cnt);
            ESP_LOGD(TAG, "Power cycle count reset");
        }

        if (!isconnected() && !isBinding() && !get_OTA_Mode() && esp_timer_get_time() > OTA_WAIT_US)
        {
            ESP_LOGI(TAG, "Enter OTA mode");
            #ifdef HEADTRACKER
            imu_Deinit(); // Delet IMU task and calculation task
            ht_espnow_deinit();
            buzzer_play_tone_sequence(doremi, 8); // play a tone sequence
            #elif defined RX_SE
            rx_espnow_deinit();
            #endif
            HttpOTA_server_init();                // OTA server init
            set_OTA_Mode(true);
            led_set_status(ota);
        }
        else if (isconnected() || get_OTA_Mode() || isBinding())   // exit if espnow connected
        {
            ESP_LOGD(TAG, "Task exit");
            vTaskDelete(NULL);
        }

        if (esp_timer_get_time() > TASK_DISTROY_TIME_US)
        {
            ESP_LOGD(TAG, "Task destroy");
            // To do, destroy the task
            vTaskDelete(NULL);
        }

        vTaskDelay(pdMS_TO_TICKS(MODE_THREAD_PERIOD));
    }
}

void mode_init(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGE(TAG, "nvs_flash_init failed.");
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Create a task for power cycle mode
    xTaskCreate(mode_Thread, "mode_Thread", MODE_THREAD_STACK_SIZE_SET, NULL, MODE_THREAD_PRIORITY_SET, NULL); // run on core0
}

#endif