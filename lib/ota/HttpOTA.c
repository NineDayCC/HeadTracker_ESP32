#include "ota.h"

#define WIFI_SSID "HeadTracker_OTA"
#define WIFI_PASS "123456789"
#define OTA_URL "ota.local"

static const char *TAG = "OTA";
static bool OTA_Mode_flag = false;

httpd_handle_t HttpOTA_httpd = NULL;

/* 单个文件的最大大小*/
#define MAX_FILE_SIZE (1024 * 1024) // 1024 KB
#define MAX_FILE_SIZE_STR "1024KB"
/* 暂存缓冲区大小*/
#define SCRATCH_BUFSIZE 1024
/* SHA-256 长度 */
#define HASH_LEN 32

bool get_OTA_Mode(void)
{
    return OTA_Mode_flag;
}

void set_OTA_Mode(bool true_or_false)
{
    OTA_Mode_flag = true_or_false;
}

// 创建WiFi热点
void wifi_init_ap(void)
{
    esp_err_t ret;
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG, "Failed to create event loop 0x%x", ret);
        return;
    }

    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    assert(ap_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .password = WIFI_PASS,
            .max_connection = 1,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "WiFi AP Started: %s", WIFI_SSID);
}

static void print_sha256(const uint8_t *image_hash, const char *label)
{
    // char hash_print[HASH_LEN * 2 + 1];
    // hash_print[HASH_LEN * 2] = 0;
    // for (int i = 0; i < HASH_LEN; ++i) {
    //     sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
    // }
    // ESP_LOGI(TAG, "%s: %s", label, hash_print);
}
// 设置一个gpio为高来判断是否启动成功 gpio2内部下拉
#define CONFIG_EXAMPLE_GPIO_DIAGNOSTIC 2
static bool diagnostic(void)
{
    return true;
}
// 校验当前固件
void firmware_Sha256()
{
    uint8_t sha_256[HASH_LEN] = {0};
    esp_partition_t partition;

    // 获取分区表的sha256摘要
    partition.address = ESP_PARTITION_TABLE_OFFSET;
    partition.size = ESP_PARTITION_TABLE_MAX_LEN;
    partition.type = ESP_PARTITION_TYPE_DATA;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "分区表SHA-256: ");

    // 为启动加载程序获取sha256摘要
    partition.address = ESP_BOOTLOADER_OFFSET;
    partition.size = ESP_PARTITION_TABLE_OFFSET;
    partition.type = ESP_PARTITION_TYPE_APP;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "引导程序SHA-256: ");

    // 获取sha256摘要以运行分区
    esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
    print_sha256(sha_256, "当前固件SHA-256: ");

    const esp_partition_t *running_partition = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running_partition, &ota_state) == ESP_OK)
    {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY)
        { // 该固件首次启动
            // 运行诊断功能...
            bool diagnostic_is_ok = diagnostic();
            if (diagnostic_is_ok)
            {
                ESP_LOGI(TAG, "Firmware diagnostic completed successfully! Continuing ...");
                esp_ota_mark_app_valid_cancel_rollback();
            }
            else
            {
                ESP_LOGE(TAG, "Firmware diagnostic failed! Starting rollback to previous version ...");
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }
        }
    }

    // const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    // if (configured != running) {
    //     ESP_LOGW(TAG, "在偏移处 0x%08x 配置的OTA引导分区,但从偏移量 0x%08x 开始",
    //              configured->address, running->address);
    //     ESP_LOGW(TAG, "(如果OTA启动数据或首选启动映像因某种原因损坏，则可能会发生这种情况。)");
    // }
    // ESP_LOGI(TAG, "运行分区类型 %s 子类型 %#x (offset 0x%08x)", running->type?"DATA":"APP", running->subtype, running->address);

    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK)
    {
        ESP_LOGI(TAG, "Current firmware version: %s", running_app_info.version);
        ESP_LOGI(TAG, "Compile time  %s,%s", running_app_info.date, running_app_info.time);
    }
}

/*将文件上传到服务器的处理程序*/
uint8_t Upload_Timeout_num;
static esp_err_t upload_post_handler(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*"); // 跨域传输协议
    esp_err_t err;
    esp_ota_handle_t update_handle = 0;
    const esp_partition_t *update_partition = NULL;
    char SendStr[100];
    Upload_Timeout_num = 0;
    /* 文件不能大于限制*/
    if (req->content_len > MAX_FILE_SIZE)
    {
        ESP_LOGE(TAG, "File too big : %d bytes", req->content_len);
        /* 回应400错误请求 */
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                            "File size must be less than" MAX_FILE_SIZE_STR "!");
        /* 返回失败以关闭基础连接，否则传入的文件内容将使套接字繁忙 */
        return ESP_FAIL;
    }
    /*请求的内容长度给出了要上传的文件的大小*/
    int remaining = req->content_len;
    int received, L_remaining = remaining;
    bool image_header_was_checked = false; // 固件头检查标识
    char *OTA_buf = malloc(sizeof(char) * SCRATCH_BUFSIZE);
    while (remaining > 0)
    {
        /* 将文件部分接收到缓冲区中 */
        if ((received = httpd_req_recv(req, OTA_buf, MIN(remaining, SCRATCH_BUFSIZE))) <= 0)
        {
            if (received == HTTPD_SOCK_ERR_TIMEOUT)
            {
                Upload_Timeout_num++;
                ESP_LOGE(TAG, "Receive overtime %d", Upload_Timeout_num);
                /* 如果发生超时，请重试 */
                if (Upload_Timeout_num >= 3)
                {
                    Upload_Timeout_num = 0;
                    ESP_LOGE(TAG, "Too many overtime!");
                    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "File receiving timeout!");
                    return ESP_FAIL;
                }
                continue;
            }
            /* 如果出现无法恢复的错误，请关闭并删除未完成的文件*/
            free(OTA_buf);
            ESP_LOGE(TAG, "File receive filed");
            /* 响应500内部服务器错误 */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Unable to receive file!");
            if (update_handle)
                esp_ota_end(update_handle); // 若已begin OTA则停止OTA
            return ESP_FAIL;
        }
        /*固件头校验*/
        // 接收到固件头
        if (image_header_was_checked == false)
        {
            esp_app_desc_t new_app_info; // 存储新固件头
            if (received > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t))
            {

                esp_app_desc_t running_app_info;
                const esp_partition_t *running = esp_ota_get_running_partition();
                if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK)
                {
                    ESP_LOGI(TAG, "Current firmware version: %s", running_app_info.version);
                    ESP_LOGI(TAG, "Comeplie time %s,%s", running_app_info.date, running_app_info.time);
                }
                // 通过下载检查新固件版本
                memcpy(&new_app_info, &OTA_buf[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
#ifdef HEADTRACKER
                if (strstr(new_app_info.version, "HT_") == NULL) // 版本错误
#elif defined RECEIVER
                if (strstr(new_app_info.version, "RX_") == NULL) // 版本错误
#else
                if (strstr(new_app_info.version, "TEST_") == NULL) // 版本错误
#endif
                {
                    ESP_LOGE(TAG, "Firmware header error!");
                    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Firmware header error!");
                    return ESP_FAIL;
                }
                ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);
                ESP_LOGI(TAG, "New firmware complie time: %s, %s", new_app_info.date, new_app_info.time);

                // 返回下一个应使用新固件写入的OTA应用程序分区
                // esp_ota_get_next_update_partition 自动选择下一个可用ota分区
                update_partition = esp_ota_get_next_update_partition(NULL);
                if (update_partition == NULL)
                {
                    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA Partition error!");
                    return ESP_FAIL;
                }
                sprintf(SendStr, "To: OTA%d Ver: %s Time: %s, %s",
                        update_partition->subtype - ESP_PARTITION_SUBTYPE_APP_OTA_MIN,
                        new_app_info.version, new_app_info.date, new_app_info.time);

                // 开始OTA OTA_SIZE_UNKNOWN将擦除整个分区
                err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
                if (err != ESP_OK)
                {
                    char str[25];
                    sprintf(str, "esp_ota_begin failed (%s)", esp_err_to_name(err));
                    ESP_LOGE(TAG, "%s", str);
                    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, str);
                    return ESP_FAIL;
                }
                ESP_LOGI(TAG, "esp_ota_begin succeeded");

                image_header_was_checked = true; // 固件头验证完成 可自行添加版本比对
            }
            else
            {
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "received package is not fit len!");
                return ESP_FAIL;
            }
        }
        /*将固件分块写入OTA分区*/
        err = esp_ota_write(update_handle, (const void *)OTA_buf, received);
        if (err != ESP_OK)
        {
            char str[25];
            sprintf(str, "esp_ota_write failed (%s)", esp_err_to_name(err));
            ESP_LOGE(TAG, "%s", str);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, str);
            return ESP_FAIL;
        }

        /*跟踪剩余要上传的文件的剩余大小*/
        remaining -= received;
    }
    free(OTA_buf);
    ESP_LOGI(TAG, "File receive complete: %dByte", L_remaining);

    err = esp_ota_end(update_handle);
    if (err != ESP_OK)
    {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED)
        {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
        }
        char str[25];
        sprintf(str, "esp_ota_end failed (%s)", esp_err_to_name(err));
        ESP_LOGE(TAG, "%s", str);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, str);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Firmware validation succeeded");

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK)
    {
        char str[50];
        sprintf(str, "esp_ota_set_boot_partition failed (%s)", esp_err_to_name(err));
        ESP_LOGE(TAG, "%s", str);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, str);
        return ESP_FAIL;
    }
    // httpd_resp_sendstr(req, "OTA successfully");
    httpd_resp_sendstr(req, SendStr);
    vTaskDelay(500 / portTICK_PERIOD_MS); // 延时等待消息发送
    ESP_LOGI(TAG, "Ready to reboot.");
    esp_restart();
    return ESP_OK;
}

// OTA 页面
static esp_err_t HttpOTA_handler(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*"); // 跨域传输协议

    extern const unsigned char HttpOTA_html_gz_start[] asm("_binary_HttpOTA_html_gz_start");
    extern const unsigned char HttpOTA_html_gz_end[] asm("_binary_HttpOTA_html_gz_end");
    size_t HttpOTA_html_gz_len = HttpOTA_html_gz_end - HttpOTA_html_gz_start;

    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    return httpd_resp_send(req, (const char *)HttpOTA_html_gz_start, HttpOTA_html_gz_len);
}

// 当前固件信息
static esp_err_t Now_handler(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*"); // 跨域传输协议

    static char json_response[1024];

    esp_app_desc_t running_app_info;
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_get_partition_description(running, &running_app_info);

    char *p = json_response;
    *p++ = '{';
    p += sprintf(p, "\"OTAsubtype\":%d,", running->subtype - ESP_PARTITION_SUBTYPE_APP_OTA_MIN); // OTA分区
    p += sprintf(p, "\"address\":%lu,", running->address);                                       // 地址
    p += sprintf(p, "\"version\":\"%s\",", running_app_info.version);                            // 版本号
    p += sprintf(p, "\"date\":\"%s\",", running_app_info.date);                                  // 日期
    p += sprintf(p, "\"time\":\"%s\"", running_app_info.time);                                   // 时间
    *p++ = '}';
    *p++ = 0;

    httpd_resp_set_type(req, "application/json");                      // 设置http响应类型
    return httpd_resp_send(req, json_response, strlen(json_response)); // 发送一个完整的HTTP响应。内容在json_response中
}

void HttpOTA_server_init()
{
    wifi_init_ap(); // 创建WiFi热点
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_open_sockets = 1;
    config.backlog_conn = 1;
    config.lru_purge_enable = true;
    config.max_uri_handlers = 15;
    config.stack_size = 8192;
    // /*使用URI通配符匹配功能，以允许同一处理程序响应与通配符方案匹配的多个不同目标URI。*/
    // config.uri_match_fn = httpd_uri_match_wildcard;

    config.server_port = 80;
    config.ctrl_port = 32779;

    ESP_LOGI(TAG, "Starting OTA server on port: '%d'", config.server_port);
    if (httpd_start(&HttpOTA_httpd, &config) == ESP_OK)
    {
        httpd_uri_t HttpOTA_uri = {// OTA页面
                                   .uri = "/",
                                   .method = HTTP_GET,
                                   .handler = HttpOTA_handler,
                                   .user_ctx = NULL};
        httpd_register_uri_handler(HttpOTA_httpd, &HttpOTA_uri);

        httpd_uri_t Now_uri = {// 当前固件信息
                               .uri = "/Now",
                               .method = HTTP_GET,
                               .handler = Now_handler,
                               .user_ctx = NULL};
        httpd_register_uri_handler(HttpOTA_httpd, &Now_uri);

        /* URI处理程序，用于将文件上传到服务器*/
        httpd_uri_t file_upload = {
            .uri = "/upload",
            .method = HTTP_POST,
            .handler = upload_post_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(HttpOTA_httpd, &file_upload);
    }
}
