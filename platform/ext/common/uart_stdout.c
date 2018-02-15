/*
 * Copyright (c) 2017 ARM Limited
 *
 * Licensed under the Apace License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apace.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "uart_stdout.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "arm_uart_drv.h"
#include "Driver_USART.h"

#define ASSERT_HIGH(X)  assert(X == ARM_DRIVER_OK)

/* Imports USART driver */
extern ARM_DRIVER_USART Driver_USART0;
extern ARM_DRIVER_USART Driver_USART1;

/* Struct FILE is implemented in stdio.h. Used to redirect printf to UART */
FILE __stdout;

/* Redirects printf to UART */
__attribute__ ((weak)) int fputc(int ch, FILE *f) {
    /* Send byte to USART */
    uart_putc(ch);

    /* Return character written */
    return ch;
}

extern struct arm_uart_dev_t ARM_UART0_DEV_S, ARM_UART0_DEV_NS;
extern struct arm_uart_dev_t ARM_UART1_DEV_S, ARM_UART1_DEV_NS;

/* Generic driver to be configured and used */
ARM_DRIVER_USART *Driver_USART = NULL;

void uart_init(enum uart_channel uchan)
{
    int32_t ret = ARM_DRIVER_OK;

    /* Add a configuration step for the UART channel to use, 0 or 1 */
    switch(uchan) {
    case UART0_CHANNEL:
        /* UART0 is configured as a non-secure peripheral, so we wouldn't be
         * able to access it using its secure alias. Ideally, we would want
         * to use UART1 only from S side as it's a secure peripheral, but for
         * simplicity, leave the option to use UART0 and use a workaround
         */
        memcpy(&ARM_UART0_DEV_S, &ARM_UART0_DEV_NS, sizeof(struct arm_uart_dev_t));
        Driver_USART = &Driver_USART0;
        break;
    case UART1_CHANNEL:
        Driver_USART = &Driver_USART1;
        break;
    default:
        ret = ARM_DRIVER_ERROR;
    }
    ASSERT_HIGH(ret);

    ret = Driver_USART->Initialize(NULL);
    ASSERT_HIGH(ret);

    ret = Driver_USART->Control(ARM_USART_MODE_ASYNCHRONOUS, 115200);
    ASSERT_HIGH(ret);
}

void uart_putc(unsigned char c)
{
    int32_t ret = ARM_DRIVER_OK;

    ret = Driver_USART->Send(&c, 1);
    ASSERT_HIGH(ret);
}

unsigned char uart_getc(void)
{
    unsigned char c = 0;
    int32_t ret = ARM_DRIVER_OK;

    ret = Driver_USART->Receive(&c, 1);
    ASSERT_HIGH(ret);

    return c;
}
