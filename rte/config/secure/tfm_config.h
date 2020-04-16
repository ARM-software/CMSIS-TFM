/*
 * Copyright (c) 2019, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef TFM_CONFIG_H
#define TFM_CONFIG_H

#define DOMAIN_NS 0

//-------- <<< Use Configuration Wizard in Context Menu >>> --------------------

// <c>Bootloader
//#define BL2
// </c>

// <o>TF-M Isolation Level
//   <1=> 1  <2=> 2  <3=> 3
#define TFM_LVL 1

// <c>Non-Secure Client Identification
//#define TFM_NS_CLIENT_IDENTIFICATION
// </c>

// <c>Core Debug Messages
//#define TFM_CORE_DEBUG
// </c>

// <o>STDIO USART Driver Number (Driver_USART#) <0-255>
#define STDIO_TFM_USART_DRV_NUM 1

// <o>STDIO USART Baudrate
#define DEFAULT_UART_BAUDRATE   115200U

// <o>Flash Driver Number (Driver_FLASH#) <0-255>
#define FLASH_DRV_NUM           0

// <o>SST Flash Driver Number (Driver_FLASH#) <0-255>
#define SST_FLASH_DRV_NUM       0

// <o>ITS Flash Driver Number (Driver_FLASH#) <0-255>
#define ITS_FLASH_DRV_NUM       0

//------------- <<< end of configuration section >>> ---------------------------

#endif /* TFM_CONFIG_H */
