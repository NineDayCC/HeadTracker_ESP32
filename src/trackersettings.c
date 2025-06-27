#include "nvs_flash.h"
#include "esp_log.h"

#include "cJSON.h"
#include "trackersettings.h"

#define SETTINGS_NVS_NAMESPACE "trackersettings"

TrackerSettings trkset = {
    .v.rollReverse = 0, // Roll direction reversed. Y:1 N:0
    .v.tiltReverse = 0, // Tilt direction reversed. Y:1 N:0
    .v.panReverse = 0,  // Pan direction reversed. Y:1 N:0

    .v.useMagn = 0,     // Use Magnetometer. Y:1 N:0
    .v.useBlutooth = 0, // Open Blutooth. Y:1 N:0

    .v.rollEn = 1, // Enable roll ppm output
    .v.tiltEn = 1, // Enable tile ppm output
    .v.panEn = 1,  // Enable pan ppm output

    .v.rollMax = DEF_MAX_PWM, // pwm max
    .v.rollMin = DEF_MIN_PWM, // pwm min
    .v.tiltMax = DEF_MAX_PWM, // pwm max
    .v.tiltMin = DEF_MIN_PWM, // pwm min
    .v.panMax = DEF_MAX_PWM,  // pwm max
    .v.panMin = DEF_MIN_PWM,  // pwm min

    .v.rollGain = 14.3, // 1000/servo max rang, 11 - 90degress, 14.3 - 70d
    .v.tiltGain = 14.3, // 1000/servo max rang, 11 - 90degress, 14.3 - 70d
    .v.panGain = 8.3,   // 1000/servo max rang, 5.5 - 180degress, 8.3 -120d

    .v.rollCnt = PPM_CENTER, // Roll pwm center
    .v.tiltCnt = PPM_CENTER, // Tilt pwm center
    .v.panCnt = PPM_CENTER,  // Pan pwm center

    .v.rollChl = 6, // Roll channel
    .v.tiltChl = 7, // Tilt channel
    .v.panChl = 8,  // Pan channel

    .v.accOffset = {0.0f, 0.0f, 0.0f}, // in g
    .v.gyrOffset = {0.0f, 0.0f, 0.0f}, // in degress

    .v.ppmininvert = false,
    .v.ppmframe = 20000, // PPM Frame Length (us)
    .v.ppmsync = 300,    // PPM Sync Pulse Length (us)
    .v.ppmchcnt = 8,     // PPM channels to output

    .v.btmode = 0, // 0:off 1:output mode
};

// Define keys for settings in nvs and json
#define ROLL_REV_KEY "Roll_Reverse"
#define TILT_REV_KEY "Tilt_Reverse"
#define PAN_REV_KEY "Pan_Reverse"
#define ROLL_EN_KEY "Roll_Enable"
#define TILT_EN_KEY "Tilt_Enable"
#define PAN_EN_KEY "Pan_Enable"
#define PPM_INVRT_KEY "PPM_Invert"

#define ROLL_MAX_KEY "Roll_PWM_Max"
#define ROLL_MIN_KEY "Roll_PWM_Min"
#define TILT_MAX_KEY "Tilt_PWM_Max"
#define TILT_MIN_KEY "Tilt_PWM_Min"
#define PAN_MAX_KEY "Pan_PWM_Max"
#define PAN_MIN_KEY "Pan_PWM_Min"
#define ROLL_GAIN_KEY "Roll_Gain"
#define TILT_GAIN_KEY "Tilt_Gain"
#define PAN_GAIN_KEY "Pan_Gain"
#define ROLL_CNT_KEY "Roll_Center"
#define TILT_CNT_KEY "Tilt_Center"
#define PAN_CNT_KEY "Pan_Center"
#define PPM_CHCNT_KEY "PPM_Channels"
#define ROLL_CHL_KEY "Roll_Channel"
#define TILT_CHL_KEY "Tilt_Channel"
#define PAN_CHL_KEY "Pan_Channel"

bool setRll_Max(uint16_t val)
{
    if (val >= MIN_PWM && val <= MAX_PWM)
    {
        trkset.v.rollMax = val;
        return true;
    }
    return false;
}

bool setRll_Min(uint16_t val)
{
    if (val >= MIN_PWM && val <= MAX_PWM)
    {
        trkset.v.rollMin = val;
        return true;
    }
    return false;
}

void initialize_nvs()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

// Read data from NVS and convert it to JSON
cJSON *nvs_to_json()
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open(SETTINGS_NVS_NAMESPACE, NVS_READONLY, &my_handle);
    if (err != ESP_OK)
    {
        return NULL;
    }

    cJSON *root = cJSON_CreateObject();

    uint8_t u8_val;
    uint16_t u16_val;
    double dbl_val;
    size_t dbl_size = sizeof(double);

    // Roll Reverse
    if (nvs_get_u8(my_handle, ROLL_REV_KEY, &u8_val) == ESP_OK)
    {
        cJSON_AddBoolToObject(root, ROLL_REV_KEY, u8_val ? 1 : 0);
        trkset.v.rollReverse = u8_val;
    }

    // Tilt Reverse
    if (nvs_get_u8(my_handle, TILT_REV_KEY, &u8_val) == ESP_OK)
    {
        cJSON_AddBoolToObject(root, TILT_REV_KEY, u8_val ? 1 : 0);
        trkset.v.tiltReverse = u8_val;
    }

    // Pan Reverse
    if (nvs_get_u8(my_handle, PAN_REV_KEY, &u8_val) == ESP_OK)
    {
        cJSON_AddBoolToObject(root, PAN_REV_KEY, u8_val ? 1 : 0);
        trkset.v.panReverse = u8_val;
    }

    // Roll Enable
    if (nvs_get_u8(my_handle, ROLL_EN_KEY, &u8_val) == ESP_OK)
    {
        cJSON_AddBoolToObject(root, ROLL_EN_KEY, u8_val ? 1 : 0);
        trkset.v.rollEn = u8_val;
    }

    // Tilt Enable
    if (nvs_get_u8(my_handle, TILT_EN_KEY, &u8_val) == ESP_OK)
    {
        cJSON_AddBoolToObject(root, TILT_EN_KEY, u8_val ? 1 : 0);
        trkset.v.tiltEn = u8_val;
    }

    // Pan Enable
    if (nvs_get_u8(my_handle, PAN_EN_KEY, &u8_val) == ESP_OK)
    {
        cJSON_AddBoolToObject(root, PAN_EN_KEY, u8_val ? 1 : 0);
        trkset.v.panEn = u8_val;
    }

    // PPM Invert
    if (nvs_get_u8(my_handle, PPM_INVRT_KEY, &u8_val) == ESP_OK)
    {
        cJSON_AddBoolToObject(root, PPM_INVRT_KEY, u8_val ? 1 : 0);
        trkset.v.ppmininvert = u8_val;
    }

    // Roll PWM Max
    if (nvs_get_u16(my_handle, ROLL_MAX_KEY, &u16_val) == ESP_OK)
    {
        cJSON_AddNumberToObject(root, ROLL_MAX_KEY, u16_val);
        trkset.v.rollMax = u16_val;
    }

    // Roll PWM Min
    if (nvs_get_u16(my_handle, ROLL_MIN_KEY, &u16_val) == ESP_OK)
    {
        cJSON_AddNumberToObject(root, ROLL_MIN_KEY, u16_val);
        trkset.v.rollMin = u16_val;
    }

    // Tilt PWM Max
    if (nvs_get_u16(my_handle, TILT_MAX_KEY, &u16_val) == ESP_OK)
    {
        cJSON_AddNumberToObject(root, TILT_MAX_KEY, u16_val);
        trkset.v.tiltMax = u16_val;
    }

    // Tilt PWM Min
    if (nvs_get_u16(my_handle, TILT_MIN_KEY, &u16_val) == ESP_OK)
    {
        cJSON_AddNumberToObject(root, TILT_MIN_KEY, u16_val);
        trkset.v.tiltMin = u16_val;
    }

    // Pan PWM Max
    if (nvs_get_u16(my_handle, PAN_MAX_KEY, &u16_val) == ESP_OK)
    {
        cJSON_AddNumberToObject(root, PAN_MAX_KEY, u16_val);
        trkset.v.panMax = u16_val;
    }

    // Pan PWM Min
    if (nvs_get_u16(my_handle, PAN_MIN_KEY, &u16_val) == ESP_OK)
    {
        cJSON_AddNumberToObject(root, PAN_MIN_KEY, u16_val);
        trkset.v.panMin = u16_val;
    }

    // Roll Gain
    dbl_size = sizeof(double);
    if (nvs_get_blob(my_handle, ROLL_GAIN_KEY, &dbl_val, &dbl_size) == ESP_OK)
    {
        cJSON_AddNumberToObject(root, ROLL_GAIN_KEY, dbl_val);
        trkset.v.rollGain = (float)dbl_val;
    }

    // Tilt Gain
    dbl_size = sizeof(double);
    if (nvs_get_blob(my_handle, TILT_GAIN_KEY, &dbl_val, &dbl_size) == ESP_OK)
    {
        cJSON_AddNumberToObject(root, TILT_GAIN_KEY, dbl_val);
        trkset.v.tiltGain = (float)dbl_val;
    }

    // Pan Gain
    dbl_size = sizeof(double);
    if (nvs_get_blob(my_handle, PAN_GAIN_KEY, &dbl_val, &dbl_size) == ESP_OK)
    {
        cJSON_AddNumberToObject(root, PAN_GAIN_KEY, dbl_val);
        trkset.v.panGain = (float)dbl_val;
    }

    // Roll Center
    if (nvs_get_u16(my_handle, ROLL_CNT_KEY, &u16_val) == ESP_OK)
    {
        cJSON_AddNumberToObject(root, ROLL_CNT_KEY, u16_val);
        trkset.v.rollCnt = u16_val;
    }

    // Tilt Center
    if (nvs_get_u16(my_handle, TILT_CNT_KEY, &u16_val) == ESP_OK)
    {
        cJSON_AddNumberToObject(root, TILT_CNT_KEY, u16_val);
        trkset.v.tiltCnt = u16_val;
    }

    // Pan Center
    if (nvs_get_u16(my_handle, PAN_CNT_KEY, &u16_val) == ESP_OK)
    {
        cJSON_AddNumberToObject(root, PAN_CNT_KEY, u16_val);
        trkset.v.panCnt = u16_val;
    }

    // PPM Channels Count
    if (nvs_get_u8(my_handle, PPM_CHCNT_KEY, &u8_val) == ESP_OK)
    {
        cJSON_AddNumberToObject(root, PPM_CHCNT_KEY, u8_val);
        trkset.v.ppmchcnt = u8_val;
    }

    // Roll Channel
    if (nvs_get_u8(my_handle, ROLL_CHL_KEY, &u8_val) == ESP_OK)
    {
        cJSON_AddNumberToObject(root, ROLL_CHL_KEY, u8_val);
        trkset.v.rollChl = u8_val;
    }

    // Tilt Channel
    if (nvs_get_u8(my_handle, TILT_CHL_KEY, &u8_val) == ESP_OK)
    {
        cJSON_AddNumberToObject(root, TILT_CHL_KEY, u8_val);
        trkset.v.tiltChl = u8_val;
    }

    // Pan Channel
    if (nvs_get_u8(my_handle, PAN_CHL_KEY, &u8_val) == ESP_OK)
    {
        cJSON_AddNumberToObject(root, PAN_CHL_KEY, u8_val);
        trkset.v.panChl = u8_val;
    }

    nvs_close(my_handle);
    ESP_LOGI("NVS", "Settings loaded from NVS");
    return root;
}

// Save JSON data to NVS
void json_to_nvs(cJSON *json)
{
    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open(SETTINGS_NVS_NAMESPACE, NVS_READWRITE, &my_handle));

    cJSON *item;

    // Roll Reverse
    item = cJSON_GetObjectItemCaseSensitive(json, ROLL_REV_KEY);
    if (cJSON_IsBool(item))
    {
        uint8_t val = cJSON_IsTrue(item) ? 1 : 0;
        ESP_ERROR_CHECK(nvs_set_u8(my_handle, ROLL_REV_KEY, val));
        trkset.v.rollReverse = val;
    }

    // Tilt Reverse
    item = cJSON_GetObjectItemCaseSensitive(json, TILT_REV_KEY);
    if (cJSON_IsBool(item))
    {
        uint8_t val = cJSON_IsTrue(item) ? 1 : 0;
        ESP_ERROR_CHECK(nvs_set_u8(my_handle, TILT_REV_KEY, val));
        trkset.v.tiltReverse = val;
    }

    // Pan Reverse
    item = cJSON_GetObjectItemCaseSensitive(json, PAN_REV_KEY);
    if (cJSON_IsBool(item))
    {
        uint8_t val = cJSON_IsTrue(item) ? 1 : 0;
        ESP_ERROR_CHECK(nvs_set_u8(my_handle, PAN_REV_KEY, val));
        trkset.v.panReverse = val;
    }

    // Roll Enable
    item = cJSON_GetObjectItemCaseSensitive(json, ROLL_EN_KEY);
    if (cJSON_IsBool(item))
    {
        uint8_t val = cJSON_IsTrue(item) ? 1 : 0;
        ESP_ERROR_CHECK(nvs_set_u8(my_handle, ROLL_EN_KEY, val));
        trkset.v.rollEn = val;
    }

    // Tilt Enable
    item = cJSON_GetObjectItemCaseSensitive(json, TILT_EN_KEY);
    if (cJSON_IsBool(item))
    {
        uint8_t val = cJSON_IsTrue(item) ? 1 : 0;
        ESP_ERROR_CHECK(nvs_set_u8(my_handle, TILT_EN_KEY, val));
        trkset.v.tiltEn = val;
    }

    // Pan Enable
    item = cJSON_GetObjectItemCaseSensitive(json, PAN_EN_KEY);
    if (cJSON_IsBool(item))
    {
        uint8_t val = cJSON_IsTrue(item) ? 1 : 0;
        ESP_ERROR_CHECK(nvs_set_u8(my_handle, PAN_EN_KEY, val));
        trkset.v.panEn = val;
    }

    // PPM Invert
    item = cJSON_GetObjectItemCaseSensitive(json, PPM_INVRT_KEY);
    if (cJSON_IsBool(item))
    {
        uint8_t val = cJSON_IsTrue(item) ? 1 : 0;
        ESP_ERROR_CHECK(nvs_set_u8(my_handle, PPM_INVRT_KEY, val));
        trkset.v.ppmininvert = val;
    }

    // Roll PWM Max
    item = cJSON_GetObjectItemCaseSensitive(json, ROLL_MAX_KEY);
    if (cJSON_IsNumber(item))
    {
        ESP_ERROR_CHECK(nvs_set_u16(my_handle, ROLL_MAX_KEY, (uint16_t)item->valuedouble));
        trkset.v.rollMax = (uint16_t)item->valuedouble;
    }

    // Roll PWM Min
    item = cJSON_GetObjectItemCaseSensitive(json, ROLL_MIN_KEY);
    if (cJSON_IsNumber(item))
    {
        if ((uint16_t)item->valuedouble >= trkset.v.rollMax)
        {
            item->valuedouble = trkset.v.rollMax - 2; // Ensure min is less than max
        }
        ESP_ERROR_CHECK(nvs_set_u16(my_handle, ROLL_MIN_KEY, (uint16_t)item->valuedouble));
        trkset.v.rollMin = (uint16_t)item->valuedouble;
    }

    // Tilt PWM Max
    item = cJSON_GetObjectItemCaseSensitive(json, TILT_MAX_KEY);
    if (cJSON_IsNumber(item))
    {
        ESP_ERROR_CHECK(nvs_set_u16(my_handle, TILT_MAX_KEY, (uint16_t)item->valuedouble));
        trkset.v.tiltMax = (uint16_t)item->valuedouble;
    }

    // Tilt PWM Min
    item = cJSON_GetObjectItemCaseSensitive(json, TILT_MIN_KEY);
    if (cJSON_IsNumber(item))
    {
        if ((uint16_t)item->valuedouble >= trkset.v.tiltMax)
        {
            item->valuedouble = trkset.v.tiltMax - 2; // Ensure min is less than max
        }
        ESP_ERROR_CHECK(nvs_set_u16(my_handle, TILT_MIN_KEY, (uint16_t)item->valuedouble));
        trkset.v.tiltMin = (uint16_t)item->valuedouble;
    }

    // Pan PWM Max
    item = cJSON_GetObjectItemCaseSensitive(json, PAN_MAX_KEY);
    if (cJSON_IsNumber(item))
    {
        ESP_ERROR_CHECK(nvs_set_u16(my_handle, PAN_MAX_KEY, (uint16_t)item->valuedouble));
        trkset.v.panMax = (uint16_t)item->valuedouble;
    }

    // Pan PWM Min
    item = cJSON_GetObjectItemCaseSensitive(json, PAN_MIN_KEY);
    if (cJSON_IsNumber(item))
    {
        if ((uint16_t)item->valuedouble >= trkset.v.panMax)
        {
            item->valuedouble = trkset.v.panMax - 2; // Ensure min is less than max
        }
        ESP_ERROR_CHECK(nvs_set_u16(my_handle, PAN_MIN_KEY, (uint16_t)item->valuedouble));
        trkset.v.panMin = (uint16_t)item->valuedouble;
    }

    // Roll Gain
    item = cJSON_GetObjectItemCaseSensitive(json, ROLL_GAIN_KEY);
    if (cJSON_IsNumber(item))
    {
        if (item->valuedouble < 1.0)
            item->valuedouble = 1.0;
        else if (item->valuedouble > 99.0)
            item->valuedouble = 99.0;
        ESP_ERROR_CHECK(nvs_set_blob(my_handle, ROLL_GAIN_KEY, &item->valuedouble, sizeof(double)));
        trkset.v.rollGain = (float)item->valuedouble;
    }

    // Tilt Gain
    item = cJSON_GetObjectItemCaseSensitive(json, TILT_GAIN_KEY);
    if (cJSON_IsNumber(item))
    {
        if (item->valuedouble < 1.0)
            item->valuedouble = 1.0;
        else if (item->valuedouble > 99.0)
            item->valuedouble = 99.0;
        ESP_ERROR_CHECK(nvs_set_blob(my_handle, TILT_GAIN_KEY, &item->valuedouble, sizeof(double)));
        trkset.v.tiltGain = (float)item->valuedouble;
    }

    // Pan Gain
    item = cJSON_GetObjectItemCaseSensitive(json, PAN_GAIN_KEY);
    if (cJSON_IsNumber(item))
    {
        if (item->valuedouble < 1.0)
            item->valuedouble = 1.0;
        else if (item->valuedouble > 99.0)
            item->valuedouble = 99.0;
        ESP_ERROR_CHECK(nvs_set_blob(my_handle, PAN_GAIN_KEY, &item->valuedouble, sizeof(double)));
        trkset.v.panGain = (float)item->valuedouble;
    }

    // Roll Center
    item = cJSON_GetObjectItemCaseSensitive(json, ROLL_CNT_KEY);
    if (cJSON_IsNumber(item))
    {
        if (item->valuedouble < trkset.v.rollMin)
            item->valuedouble = trkset.v.rollMin + 1; // Ensure center is greater than min
        else if (item->valuedouble > trkset.v.rollMax)
            item->valuedouble = trkset.v.rollMax - 1; // Ensure center is less than max
        ESP_ERROR_CHECK(nvs_set_u16(my_handle, ROLL_CNT_KEY, (uint16_t)item->valuedouble));
        trkset.v.rollCnt = (uint16_t)item->valuedouble;
    }

    // Tilt Center
    item = cJSON_GetObjectItemCaseSensitive(json, TILT_CNT_KEY);
    if (cJSON_IsNumber(item))
    {
        if (item->valuedouble < trkset.v.tiltMin)
            item->valuedouble = trkset.v.tiltMin + 1; // Ensure center is greater than min
        else if (item->valuedouble > trkset.v.tiltMax)
            item->valuedouble = trkset.v.tiltMax - 1; // Ensure center is less than max
        ESP_ERROR_CHECK(nvs_set_u16(my_handle, TILT_CNT_KEY, (uint16_t)item->valuedouble));
        trkset.v.tiltCnt = (uint16_t)item->valuedouble;
    }

    // Pan Center
    item = cJSON_GetObjectItemCaseSensitive(json, PAN_CNT_KEY);
    if (cJSON_IsNumber(item))
    {
        if (item->valuedouble < trkset.v.panMin)
            item->valuedouble = trkset.v.panMin + 1; // Ensure center is greater than min
        else if (item->valuedouble > trkset.v.panMax)
            item->valuedouble = trkset.v.panMax - 1; // Ensure center is less than max
        ESP_ERROR_CHECK(nvs_set_u16(my_handle, PAN_CNT_KEY, (uint16_t)item->valuedouble));
        trkset.v.panCnt = (uint16_t)item->valuedouble;
    }

    // PPM Channels Count
    item = cJSON_GetObjectItemCaseSensitive(json, PPM_CHCNT_KEY);
    if (cJSON_IsNumber(item))
    {
        if (item->valuedouble < 1)
            item->valuedouble = 1;
        else if (item->valuedouble > 15)
            item->valuedouble = 15;
        ESP_ERROR_CHECK(nvs_set_u8(my_handle, PPM_CHCNT_KEY, (uint8_t)item->valuedouble));
        trkset.v.ppmchcnt = (uint8_t)item->valuedouble;
    }

    // Roll Channel
    item = cJSON_GetObjectItemCaseSensitive(json, ROLL_CHL_KEY);
    if (cJSON_IsNumber(item))
    {
        if (item->valuedouble < 1)
            item->valuedouble = 1;
        else if (item->valuedouble > trkset.v.ppmchcnt)
            item->valuedouble = trkset.v.ppmchcnt;
        ESP_ERROR_CHECK(nvs_set_u8(my_handle, ROLL_CHL_KEY, (uint8_t)item->valuedouble));
        trkset.v.rollChl = (uint8_t)item->valuedouble;
    }

    // Tilt Channel
    item = cJSON_GetObjectItemCaseSensitive(json, TILT_CHL_KEY);
    if (cJSON_IsNumber(item))
    {
        if (item->valuedouble < 1)
            item->valuedouble = 1;
        else if (item->valuedouble > trkset.v.ppmchcnt)
            item->valuedouble = trkset.v.ppmchcnt;
        ESP_ERROR_CHECK(nvs_set_u8(my_handle, TILT_CHL_KEY, (uint8_t)item->valuedouble));
        trkset.v.tiltChl = (uint8_t)item->valuedouble;
    }

    // Pan Channel
    item = cJSON_GetObjectItemCaseSensitive(json, PAN_CHL_KEY);
    if (cJSON_IsNumber(item))
    {
        if (item->valuedouble < 1)
            item->valuedouble = 1;
        else if (item->valuedouble > trkset.v.ppmchcnt)
            item->valuedouble = trkset.v.ppmchcnt;
        ESP_ERROR_CHECK(nvs_set_u8(my_handle, PAN_CHL_KEY, (uint8_t)item->valuedouble));
        trkset.v.panChl = (uint8_t)item->valuedouble;
    }

    ESP_ERROR_CHECK(nvs_commit(my_handle));
    nvs_close(my_handle);
    ESP_LOGI("trkset", "Settings saved to NVS");
}

void trkset_init()
{
    initialize_nvs();

    // Load settings from NVS
    cJSON *json = nvs_to_json();
    if (json)
    {
        cJSON_Delete(json);
        ESP_LOGI("trkset", "Loaded settings from NVS");
    }
    else
    {
        trkset_restore_defaults();
        ESP_LOGW("trkset", "No settings found in NVS, using default values");
    }
}

void trkset_restore_defaults()
{
    // 恢复trkset结构体为默认值
    trkset.v.rollReverse = 0;
    trkset.v.tiltReverse = 0;
    trkset.v.panReverse = 0;
    trkset.v.useMagn = 0;
    trkset.v.useBlutooth = 0;
    trkset.v.rollEn = 1;
    trkset.v.tiltEn = 1;
    trkset.v.panEn = 1;
    trkset.v.rollMax = DEF_MAX_PWM;
    trkset.v.rollMin = DEF_MIN_PWM;
    trkset.v.tiltMax = DEF_MAX_PWM;
    trkset.v.tiltMin = DEF_MIN_PWM;
    trkset.v.panMax = DEF_MAX_PWM;
    trkset.v.panMin = DEF_MIN_PWM;
    trkset.v.rollGain = 14.3f;
    trkset.v.tiltGain = 14.3f;
    trkset.v.panGain = 8.3f;
    trkset.v.rollCnt = PPM_CENTER;
    trkset.v.tiltCnt = PPM_CENTER;
    trkset.v.panCnt = PPM_CENTER;
    trkset.v.rollChl = 6;
    trkset.v.tiltChl = 7;
    trkset.v.panChl = 8;
    trkset.v.accOffset[0] = 0.0f;
    trkset.v.accOffset[1] = 0.0f;
    trkset.v.accOffset[2] = 0.0f;
    trkset.v.gyrOffset[0] = 0.0f;
    trkset.v.gyrOffset[1] = 0.0f;
    trkset.v.gyrOffset[2] = 0.0f;
    trkset.v.ppmininvert = false;
    trkset.v.ppmframe = 20000;
    trkset.v.ppmsync = 300;
    trkset.v.ppmchcnt = 8;
    trkset.v.btmode = 0;

    // 擦除NVS命名空间
    nvs_handle_t handle;
    if (nvs_open(SETTINGS_NVS_NAMESPACE, NVS_READWRITE, &handle) == ESP_OK) {
        ESP_LOGI("trkset", "Erasing NVS namespace: %s", SETTINGS_NVS_NAMESPACE);
        nvs_erase_all(handle);
        nvs_commit(handle);
        nvs_close(handle);
    }

    // 重新保存默认值到NVS
    cJSON *default_json = cJSON_CreateObject();
    cJSON_AddBoolToObject(default_json, ROLL_REV_KEY, trkset.v.rollReverse);
    cJSON_AddBoolToObject(default_json, TILT_REV_KEY, trkset.v.tiltReverse);
    cJSON_AddBoolToObject(default_json, PAN_REV_KEY, trkset.v.panReverse);
    cJSON_AddBoolToObject(default_json, ROLL_EN_KEY, trkset.v.rollEn);
    cJSON_AddBoolToObject(default_json, TILT_EN_KEY, trkset.v.tiltEn);
    cJSON_AddBoolToObject(default_json, PAN_EN_KEY, trkset.v.panEn);
    cJSON_AddBoolToObject(default_json, PPM_INVRT_KEY, trkset.v.ppmininvert);

    cJSON_AddNumberToObject(default_json, ROLL_MAX_KEY, trkset.v.rollMax);
    cJSON_AddNumberToObject(default_json, ROLL_MIN_KEY, trkset.v.rollMin);
    cJSON_AddNumberToObject(default_json, TILT_MAX_KEY, trkset.v.tiltMax);
    cJSON_AddNumberToObject(default_json, TILT_MIN_KEY, trkset.v.tiltMin);
    cJSON_AddNumberToObject(default_json, PAN_MAX_KEY, trkset.v.panMax);
    cJSON_AddNumberToObject(default_json, PAN_MIN_KEY, trkset.v.panMin);

    cJSON_AddNumberToObject(default_json, ROLL_GAIN_KEY, trkset.v.rollGain);
    cJSON_AddNumberToObject(default_json, TILT_GAIN_KEY, trkset.v.tiltGain);
    cJSON_AddNumberToObject(default_json, PAN_GAIN_KEY, trkset.v.panGain);

    cJSON_AddNumberToObject(default_json, ROLL_CNT_KEY, trkset.v.rollCnt);
    cJSON_AddNumberToObject(default_json, TILT_CNT_KEY, trkset.v.tiltCnt);
    cJSON_AddNumberToObject(default_json, PAN_CNT_KEY, trkset.v.panCnt);

    cJSON_AddNumberToObject(default_json, PPM_CHCNT_KEY, trkset.v.ppmchcnt);
    cJSON_AddNumberToObject(default_json, ROLL_CHL_KEY, trkset.v.rollChl);
    cJSON_AddNumberToObject(default_json, TILT_CHL_KEY, trkset.v.tiltChl);
    cJSON_AddNumberToObject(default_json, PAN_CHL_KEY, trkset.v.panChl);

    ESP_LOGI("trkset", "Saving default settings to NVS");
    json_to_nvs(default_json);
    cJSON_Delete(default_json);

    ESP_LOGW("trkset", "Settings restored to default and saved to NVS");
}