/*
 * Copyright (c) 2019, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef TFM_TEST_CONFIG_H
#define TFM_TEST_CONFIG_H

//-------- <<< Use Configuration Wizard in Context Menu >>> --------------------

// <h>Test Framework Configuration

//   <o>TF-M Isolation Level
//     <1=> 1  <2=> 2  <3=> 3
#define TFM_LVL 1

//   <c>Non-Secure Client Identification
//#define TFM_NS_CLIENT_IDENTIFICATION
//   </c>

//   <o>STDIO USART Driver Number (Driver_USART#) <0-255>
#define STDIO_NS_USART_DRV_NUM  0

//   <o>STDIO USART Baudrate
#define DEFAULT_UART_BAUDRATE   115200U

//   <o>SST Maximum Asset Size
#define SST_MAX_ASSET_SIZE      2048

//   <o>ITS Maximum Asset Size
#define ITS_MAX_ASSET_SIZE      512

// </h>

//------------- <<< end of configuration section >>> ---------------------------

#endif /* TFM_TEST_CONFIG_H */
