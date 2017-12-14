/*
 * Copyright (c) 2017 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __UART_STDOUT_H__
#define __UART_STDOUT_H__

#include <stdint.h>

/**
 * \brief UART channels that
 *        can be used from TFM
 */
enum uart_channel {
    UART0_CHANNEL = 0,
    UART1_CHANNEL,
    UART_INVALID
};

/**
 * \brief Initializes the UART.
 *
 * \param[in] uchan UART channel
 *                  to use, 0 or 1.
 */
void uart_init(enum uart_channel uchan);

/**
 * \brief Puts a character in the UART.
 *
 * \param[in]  ch  Character to write.
 */
void uart_putc(unsigned char ch);

/**
 * \brief Gets a character from the UART.
 *
 * \return Character read from UART.
 */
unsigned char uart_getc(void);

#endif /* __UART_STDOUT_H__ */
