#include "sbus.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

//------------------------------------------------------------------------------
// Defines
#define SBUS_FRAME_LEN 25

//------------------------------------------------------------------------------
// Macro Modules
LOG_MODULE_REGISTER(SbusLog, LOG_LEVEL_DBG);
K_MUTEX_DEFINE(sbus_mutex);

//------------------------------------------------------------------------------
// Private Values
static const uint8_t HEADER_ = 0x0F;
static const uint8_t FOOTER_ = 0x00;
static const uint8_t FOOTER2_ = 0x04;
static const int8_t PAYLOAD_LEN_ = 23;
static const int8_t HEADER_LEN_ = 1;
static const int8_t FOOTER_LEN_ = 1;
static const uint8_t LEN_ = 25;
static const uint8_t CH17_ = 0x01;
static const uint8_t CH18_ = 0x02;
static const uint8_t LOST_FRAME_ = 0x04;
static const uint8_t FAILSAFE_ = 0x08;
static const uint8_t CH17_MASK_ = 0x01;
static const uint8_t CH18_MASK_ = 0x02;
static const uint8_t LOST_FRAME_MASK_ = 0x04;
static const uint8_t FAILSAFE_MASK_ = 0x08;
static bool failsafe_ = false, lost_frame_ = false, ch17_ = false, ch18_ = false;

static uint8_t localTXBuffer[SBUS_FRAME_LEN]; // Local Buffer
static uint8_t dmaTXBuffer[SBUS_FRAME_LEN];

static uint8_t testData[] = "UART1 OUT";
static const struct uart_config sbus_uart_cfg = {
		.baudrate = 115200,
		.parity = UART_CFG_PARITY_NONE,
		.stop_bits = UART_CFG_STOP_BITS_1,
		.data_bits = UART_CFG_DATA_BITS_8,
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE
	};
// static const struct uart_config sbus_uart_cfg = {
// 		.baudrate = 100000,
// 		.parity = UART_CFG_PARITY_ODD,
// 		.stop_bits = UART_CFG_STOP_BITS_2,
// 		.data_bits = UART_CFG_DATA_BITS_8,
// 		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE
// 	};

//--------------------Function Defines--------------------

/*
 * Print a null-terminated string character by character to the UART interface
 */
void print_uart(const struct device *dev, char *buf, uint16_t len)
{
    for (int i = 0; i < len; i++)
    {
        uart_poll_out(dev, buf[i]);
    }
}

int sbusInit(const struct device *dev)
{
    int ret;
    ret = uart_configure(dev, &sbus_uart_cfg);
    if (ret)
    {
        LOG_ERR("uart_configure failed: %d", ret);
    }
    else
    {
        LOG_DBG("uart_configure Success");
    }
}

void sbusTX(const struct device *dev)
{
    k_mutex_lock(&sbus_mutex, K_FOREVER);
    memcpy(dmaTXBuffer, localTXBuffer, SBUS_FRAME_LEN);
    k_mutex_unlock(&sbus_mutex);

    print_uart(dev, testData, sizeof(testData));
    LOG_DBG("Sbus Sent");
    // int err = uart_tx(dev, dmaTXBuffer, SBUS_FRAME_LEN, 0);
    // if (err)
    // {
    //     LOG_ERR("Sbus TX failed (err %u)", err);
    // }
    // else
    // {
    //     // LOG_DBG("Sbus Sent Success");
    //     for (uint8_t i = 0; i < SBUS_FRAME_LEN; i++)
    //     {
    //         printf("%x  ",dmaTXBuffer[i]);
    //     }
    //     printf("\r\n");
    // }
}

/* FROM -----
 * Brian R Taylor
 * brian.taylor@bolderflight.com
 *
 * Copyright (c) 2021 Bolder Flight Systems Inc
 */

void sbusBuildChannels(uint16_t *ch_)
{
    k_mutex_lock(&sbus_mutex, K_FOREVER);
    uint8_t *buf_ = localTXBuffer;
    buf_[0] = HEADER_;
    buf_[1] = (uint8_t)((ch_[0] & 0x07FF));
    buf_[2] = (uint8_t)((ch_[0] & 0x07FF) >> 8 | (ch_[1] & 0x07FF) << 3);
    buf_[3] = (uint8_t)((ch_[1] & 0x07FF) >> 5 | (ch_[2] & 0x07FF) << 6);
    buf_[4] = (uint8_t)((ch_[2] & 0x07FF) >> 2);
    buf_[5] = (uint8_t)((ch_[2] & 0x07FF) >> 10 | (ch_[3] & 0x07FF) << 1);
    buf_[6] = (uint8_t)((ch_[3] & 0x07FF) >> 7 | (ch_[4] & 0x07FF) << 4);
    buf_[7] = (uint8_t)((ch_[4] & 0x07FF) >> 4 | (ch_[5] & 0x07FF) << 7);
    buf_[8] = (uint8_t)((ch_[5] & 0x07FF) >> 1);
    buf_[9] = (uint8_t)((ch_[5] & 0x07FF) >> 9 | (ch_[6] & 0x07FF) << 2);
    buf_[10] = (uint8_t)((ch_[6] & 0x07FF) >> 6 | (ch_[7] & 0x07FF) << 5);
    buf_[11] = (uint8_t)((ch_[7] & 0x07FF) >> 3);
    buf_[12] = (uint8_t)((ch_[8] & 0x07FF));
    buf_[13] = (uint8_t)((ch_[8] & 0x07FF) >> 8 | (ch_[9] & 0x07FF) << 3);
    buf_[14] = (uint8_t)((ch_[9] & 0x07FF) >> 5 | (ch_[10] & 0x07FF) << 6);
    buf_[15] = (uint8_t)((ch_[10] & 0x07FF) >> 2);
    buf_[16] = (uint8_t)((ch_[10] & 0x07FF) >> 10 | (ch_[11] & 0x07FF) << 1);
    buf_[17] = (uint8_t)((ch_[11] & 0x07FF) >> 7 | (ch_[12] & 0x07FF) << 4);
    buf_[18] = (uint8_t)((ch_[12] & 0x07FF) >> 4 | (ch_[13] & 0x07FF) << 7);
    buf_[19] = (uint8_t)((ch_[13] & 0x07FF) >> 1);
    buf_[20] = (uint8_t)((ch_[13] & 0x07FF) >> 9 | (ch_[14] & 0x07FF) << 2);
    buf_[21] = (uint8_t)((ch_[14] & 0x07FF) >> 6 | (ch_[15] & 0x07FF) << 5);
    buf_[22] = (uint8_t)((ch_[15] & 0x07FF) >> 3);
    buf_[23] = 0x00 | (ch17_ * CH17_) | (ch18_ * CH18_) | (failsafe_ * FAILSAFE_) |
               (lost_frame_ * LOST_FRAME_);
    buf_[24] = FOOTER_;
    k_mutex_unlock(&sbus_mutex);
}
