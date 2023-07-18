#include "bt.h"

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <stdio.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include "trackersettings.h"
#include "PPM.h"

//------------------------------------------------------------------------------
// Defines
#define BT_HEADTRACKER_UUID_VALUE 0x4EF0
#define BT_PWM_UUID_VALUE 0x4EF1
#define BT_UUID_HEADTRACKER BT_UUID_DECLARE_16(BT_HEADTRACKER_UUID_VALUE)
#define BT_UUID_PWM BT_UUID_DECLARE_16(BT_PWM_UUID_VALUE)

#define BT_MIN_CONN_INTER 10   // Minimum Connection Interval (N * 1.25 ms)
#define BT_MAX_CONN_INTER 10   // Maximum Connection Interval (N * 1.25 ms)
#define BT_CONN_LOST_TIME 100 // Supervision Timeout (N * 10 ms)

//------------------------------------------------------------------------------
// Macro Modules
LOG_MODULE_REGISTER(BtLog, LOG_LEVEL_DBG);

//------------------------------------------------------------------------------
// Private Values

/* BLE connection */
static struct bt_conn *default_conn;

// Service UUID
static struct bt_uuid_16 bt_service_uuid = BT_UUID_INIT_16(
    BT_HEADTRACKER_UUID_VALUE);

// UUID's
struct bt_uuid_16 bt_pwm_uuid = BT_UUID_INIT_16(BT_PWM_UUID_VALUE);

static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);
static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params subscribe_params;

//--------------------Function Defines--------------------
void printBtData(uint8_t *data);
static void start_scan(void);

static uint8_t notify_func(struct bt_conn *conn,
                           struct bt_gatt_subscribe_params *params,
                           const void *data, uint16_t length)
{
    if (!data)
    {
        LOG_DBG("[UNSUBSCRIBED]");
        params->value_handle = 0U;
        return BT_GATT_ITER_STOP;
    }

    // LOG_DBG("[NOTIFICATION] data %p length %u", data, length);

    uint8_t *channel_data = data;
    for (uint8_t i = 0; i < PpmOut_getChnCount(); i++)
    {
        uint16_t ppmout = (channel_data[i * 2] << 8) | channel_data[i * 2 + 1];
        if (ppmout == 0)
            ppmout = PPM_CENTER;
        // printf("%d  ", ppmout); //test
        PpmOut_setChannel(i, ppmout);
    }
    buildChannels();
    printBtData(channel_data); // test

    return BT_GATT_ITER_CONTINUE;
}

static uint8_t discover_func(struct bt_conn *conn,
                             const struct bt_gatt_attr *attr,
                             struct bt_gatt_discover_params *params)
{
    int err;

    if (!attr)
    {
        LOG_INF("Discover complete");
        (void)memset(params, 0, sizeof(*params));
        return BT_GATT_ITER_STOP;
    }

    if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_HEADTRACKER))
    {
        memcpy(&uuid, BT_UUID_PWM, sizeof(uuid));
        discover_params.uuid = &uuid.uuid;
        discover_params.start_handle = attr->handle + 1;
        discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

        err = bt_gatt_discover(conn, &discover_params);
        if (err)
        {
            LOG_ERR("Discover failed (err %d)", err);
        }
    }
    else if (!bt_uuid_cmp(discover_params.uuid,
                          BT_UUID_PWM))
    {
        memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
        discover_params.uuid = &uuid.uuid;
        discover_params.start_handle = attr->handle + 2;
        discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
        subscribe_params.value_handle = bt_gatt_attr_value_handle(attr);

        err = bt_gatt_discover(conn, &discover_params);
        if (err)
        {
            LOG_ERR("Discover failed (err %d)", err);
        }
    }
    else
    {
        subscribe_params.notify = notify_func;
        subscribe_params.value = BT_GATT_CCC_NOTIFY;
        subscribe_params.ccc_handle = attr->handle;

        err = bt_gatt_subscribe(conn, &subscribe_params);
        if (err && err != -EALREADY)
        {
            LOG_INF("Subscribe failed (err %d)", err);
        }
        else
        {
            LOG_INF("[SUBSCRIBED]");
        }

        return BT_GATT_ITER_STOP;
    }

    return BT_GATT_ITER_STOP;
}

static bool eir_found(struct bt_data *data, void *user_data)
{
    bt_addr_le_t *addr = user_data;
    int i;

    // LOG_DBG("[AD]: %u data_len %u", data->type, data->data_len);

    switch (data->type)
    {
    case BT_DATA_UUID16_SOME:
    case BT_DATA_UUID16_ALL:
        if (data->data_len % sizeof(uint16_t) != 0U)
        {
            LOG_ERR("AD malformed");
            return true;
        }

        for (i = 0; i < data->data_len; i += sizeof(uint16_t))
        {
            struct bt_le_conn_param *param;
            struct bt_uuid *uuid;
            uint16_t u16_val;
            int err;

            memcpy(&u16_val, &data->data[i], sizeof(u16_val));
            uuid = BT_UUID_DECLARE_16(sys_le16_to_cpu(u16_val));
            if (bt_uuid_cmp(uuid, BT_UUID_HEADTRACKER))
            {
                continue;
            }

            err = bt_le_scan_stop();
            if (err)
            {
                LOG_ERR("Stop LE scan failed (err %d)", err);
                continue;
            }

            param = BT_LE_CONN_PARAM(BT_MIN_CONN_INTER,
                                     BT_MAX_CONN_INTER,
                                     0, BT_CONN_LOST_TIME);
            err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
                                    param, &default_conn);
            if (err)
            {
                LOG_ERR("Create conn failed (err %d)", err);
                start_scan();
            }

            return false;
        }
    }

    return true;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
                         struct net_buf_simple *ad)
{
    char dev[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(addr, dev, sizeof(dev));
    // LOG_DBG("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %i",
    //    dev, type, ad->len, rssi);

    /* We're only interested in connectable events */
    if (type == BT_GAP_ADV_TYPE_ADV_IND ||
        type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND)
    {
        bt_data_parse(ad, eir_found, (void *)addr);
    }
}

static void start_scan(void)
{
    int err;

    /* Use active scanning and disable duplicate filtering to handle any
     * devices that might update their advertising data at runtime. */
    struct bt_le_scan_param scan_param = {
        .type = BT_LE_SCAN_TYPE_ACTIVE,
        .options = BT_LE_SCAN_OPT_NONE,
        .interval = BT_GAP_SCAN_FAST_INTERVAL,
        .window = BT_GAP_SCAN_FAST_WINDOW,
    };

    err = bt_le_scan_start(&scan_param, device_found);
    if (err)
    {
        LOG_ERR("Scanning failed to start (err %d)", err);
        return;
    }

    LOG_INF("Scanning successfully started");
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
    char addr[BT_ADDR_LE_STR_LEN];
    int err;

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (conn_err)
    {
        LOG_ERR("Failed to connect to %s (%u)", addr, conn_err);

        bt_conn_unref(default_conn);
        default_conn = NULL;

        start_scan();
        return;
    }

    LOG_INF("Connected: %s", addr);

    struct bt_conn_info info;
    bt_conn_get_info(default_conn, &info);

    LOG_INF("BT Connection Params Int:%d Lat:%d Timeout:%d", info.le.interval, info.le.latency,
            info.le.timeout);
    // LOG_INF("Requesting coded PHY - %s",
    // bt_conn_le_phy_update(pararmtconn, &phy_params) ? "FAILED" : "Success");

    if (conn == default_conn)
    {
        memcpy(&uuid, BT_UUID_HEADTRACKER, sizeof(uuid));
        discover_params.uuid = &uuid.uuid;
        discover_params.func = discover_func;
        discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
        discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
        discover_params.type = BT_GATT_DISCOVER_PRIMARY;

        err = bt_gatt_discover(default_conn, &discover_params);
        if (err)
        {
            LOG_ERR("Discover failed(err %d)", err);
            return;
        }
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("Disconnected: %s (reason 0x%02x)", addr, reason);

    if (default_conn != conn)
    {
        return;
    }

    bt_conn_unref(default_conn);
    default_conn = NULL;

    start_scan();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

void bt_init()
{
    int err = bt_enable(NULL);
    if (err)
    {
        LOG_ERR("bt_enable failed (err %d)", err);
        return;
    }

    start_scan();
    LOG_INF("Bluetooth initialized");
}

void bt_Thread()
{
    int64_t usduration = 0;
    while (1)
    {

        usduration = micros64();

        // struct bt_conn_info info;
        // bt_conn_get_info(default_conn, &info);

        // LOG_INF("BT Connection Params Int:%d Lat:%d Timeout:%d", info.le.interval, info.le.latency,
        //         info.le.timeout);

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

void printBtData(uint8_t *data)
{
    uint16_t u16_val;
    printf("[%s]", now_str());
    for (uint8_t i = 0; i < BT_CHANNELS; i++)
    {
        u16_val = (data[i * 2] << 8) | data[i * 2 + 1];
        printf("%d  ", u16_val);
    }
    printf("\r\n");
}
