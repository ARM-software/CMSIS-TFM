/*
 * Copyright (c) 2019, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef TFM_CONFIG_H
#define TFM_CONFIG_H

#define BL2

#include "bl2_config.h"

//-------- <<< Use Configuration Wizard in Context Menu >>> --------------------

// <o>STDIO USART Driver Number (Driver_USART#) <0-255>
#define STDIO_TFM_USART_DRV_NUM 1

// <o>STDIO USART Baudrate
#define DEFAULT_UART_BAUDRATE   115200U

// <o>Flash Driver Number (Driver_FLASH#) <0-255>
#define FLASH_DRV_NUM           0

//------------- <<< end of configuration section >>> ---------------------------

#endif /* TFM_CONFIG_H */
