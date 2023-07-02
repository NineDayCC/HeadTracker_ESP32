#include "bt.h"

#include <zephyr/kernel.h>
#include <stdio.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>

#include "io.h"
#include "trackersettings.h"

//------------------------------------------------------------------------------
// Defines
#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
#define BT_HEADTRACKER_UUID_VALUE 0x4EF0
#define BT_PWM_UUID_VALUE 0x4EF1

//------------------------------------------------------------------------------
// Macro Modules
LOG_MODULE_REGISTER(BtLog, LOG_LEVEL_DBG);

//------------------------------------------------------------------------------
// Private Values
static uint8_t curmode = 0;                         // current bt mode
static uint8_t bt_pwm_value[BT_CHANNELS * 2] = {0}; // pwm data to be sent via Bluetooth

static struct k_poll_signal btThreadRunSignal = K_POLL_SIGNAL_INITIALIZER(btThreadRunSignal);
struct k_poll_event btRunEvents[1] = {
    K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &btThreadRunSignal),
};

/* BLE connection */
struct bt_conn *conn;

// Service UUID
static struct bt_uuid_16 bt_service_uuid = BT_UUID_INIT_16(
    BT_HEADTRACKER_UUID_VALUE);

// UUID's
struct bt_uuid_16 bt_pwm_uuid = BT_UUID_INIT_16(BT_PWM_UUID_VALUE);

// Bluetooth advertisement data
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
    BT_DATA_BYTES(BT_DATA_UUID16_SOME, BT_UUID_16_ENCODE(BT_HEADTRACKER_UUID_VALUE))};

//--------------------Function Defines--------------------
void printBtData(void);

static void mpu_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    ARG_UNUSED(attr);
    LOG_INF("Notification %s", (value == BT_GATT_CCC_NOTIFY) ? "enabled" : "disabled");
}

BT_GATT_SERVICE_DEFINE(
    bt_srv,
    // ATTRIBUTE 0
    BT_GATT_PRIMARY_SERVICE(&bt_service_uuid),

    // Data output Characteristic  ATTRIBUTE 1,2
    BT_GATT_CHARACTERISTIC(&bt_pwm_uuid.uuid,
                           BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ, NULL, NULL, bt_pwm_value),
    BT_GATT_CCC(mpu_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

);

static void connected(struct bt_conn *connected, uint8_t err)
{
    if (err)
    {
        LOG_ERR("Connection failed (err %u)", err);
    }
    else
    {
        LOG_INF("Connected");
        if (!conn)
        {
            conn = bt_conn_ref(connected);
        }

        struct bt_conn_info info;
        bt_conn_get_info(connected, &info);

        LOG_INF("BT Connection Params Int:%d Lat:%d Timeout:%d", info.le.interval, info.le.latency,
                info.le.timeout);
    }
}

static void disconnected(struct bt_conn *disconn, uint8_t reason)
{
    if (conn)
    {
        bt_conn_unref(conn);
        conn = NULL;
    }

    LOG_INF("Disconnected (reason %u)", reason);

    /* Start advertising */
    int err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err)
    {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return;
    }

    LOG_INF("Configuration mode: waiting connections...");
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

void BTHeadStart()
{
    /* Start advertising */
    int err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err)
    {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return;
    }

    LOG_INF("Configuration mode: waiting connections...");
}

void BTHeadStop()
{
    // Stop Advertising
    int err = bt_le_adv_stop();
    if (err)
    {
        LOG_ERR("BLE Unable to Stop advertising");
    }
    else
    {
        LOG_INF("BLE Stopped Advertising");
    }
}

void BTHeadExecute()
{
    if (conn)
    {
        // Send Trainer Data
        // int64_t uselepsed = micros64();
        bt_gatt_notify(NULL, &bt_srv.attrs[2], bt_pwm_value, sizeof(bt_pwm_value));
        printBtData();
        // uselepsed = micros64() - uselepsed;
        // LOG_DBG("Notify cost: %lld", uselepsed);
    }
}

void bt_ready(int error)
{
    BTHeadStart();
    k_poll_signal_raise(&btThreadRunSignal, 1);
}

void bt_init()
{
    int err = bt_enable(bt_ready);
    if (err)
    {
        LOG_ERR("bt_enable failed (err %d)", err);
        return;
    }
    for (uint8_t i = 0; i < sizeof(bt_pwm_value); i += 2)
    {
        bt_pwm_value[i] = (uint8_t)(1500 & 0xff);
        bt_pwm_value[i + 1] = (uint8_t)(1500 >> 8);
    }
}

void bt_Thread()
{
    int64_t usduration = 0;
    while (1)
    {
        k_poll(btRunEvents, 1, K_FOREVER);

        usduration = micros64();

        BTHeadExecute();

        // Adjust sleep for a more accurate period
        usduration = micros64() - usduration;
        if (BT_PERIOD - usduration <
            BT_PERIOD * 0.7)
        { // Took a long time. Will crash if sleep is too short
            k_usleep(BT_PERIOD);
        }
        else
        {
            k_usleep(BT_PERIOD - usduration);
        }
    }
}

void buildBtChannels(uint16_t *channelData, uint8_t channels)
{
    for (uint8_t i = 0; i < channels; i++)
    {
        bt_pwm_value[i * 2] = channelData[i] >> 8;
        bt_pwm_value[i * 2 + 1] = channelData[i] & 0xff;
    }
}

void printBtData(void)
{
    uint16_t u16_val;
    printf("[%s]", now_str());
    for (uint8_t i = 0; i < BT_CHANNELS; i++)
    {
        u16_val = (bt_pwm_value[i * 2] << 8) | bt_pwm_value[i * 2 + 1];
        printf("%d  ", u16_val);
    }
    printf("\r\n");
}
