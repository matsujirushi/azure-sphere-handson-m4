/*
 * (C) 2005-2019 MediaTek Inc. All rights reserved.
 *
 * Copyright Statement:
 *
 * This MT3620 driver software/firmware and related documentation
 * ("MediaTek Software") are protected under relevant copyright laws.
 * The information contained herein is confidential and proprietary to
 * MediaTek Inc. ("MediaTek"). You may only use, reproduce, modify, or
 * distribute (as applicable) MediaTek Software if you have agreed to and been
 * bound by this Statement and the applicable license agreement with MediaTek
 * ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User"). If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE
 * PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS
 * ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO
 * LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED
 * HEREUNDER WILL BE ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY
 * RECEIVER TO MEDIATEK DURING THE PRECEDING TWELVE (12) MONTHS FOR SUCH
 * MEDIATEK SOFTWARE AT ISSUE.
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "printf.h"
#include "mt3620.h"

#include "os_hal_uart.h"
#include "os_hal_gpio.h"

static const uint8_t UART_PUTCHAR = OS_HAL_UART_PORT0;
static const uint8_t GPIO_LED = OS_HAL_GPIO_16;

void _putchar(char character)
{
	mtk_os_hal_uart_put_char(UART_PUTCHAR, character);
	if (character == '\n')
		mtk_os_hal_uart_put_char(UART_PUTCHAR, '\r');
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName)
{
	printf("%s: %s\n", __func__, pcTaskName);
}

void vApplicationMallocFailedHook(void)
{
	printf("%s\n", __func__);
}

static void BlinkTask(void* pParameters)
{
	printf("Blink Task Started\n");

	int ret = mtk_os_hal_gpio_request(GPIO_LED);
	if (ret != 0)
	{
		printf("request gpio[%d] fail\n", GPIO_LED);
		return;
	}
	mtk_os_hal_gpio_set_direction(GPIO_LED, OS_HAL_GPIO_DIR_OUTPUT);

	while (true)
	{
		mtk_os_hal_gpio_set_output(GPIO_LED, OS_HAL_GPIO_DATA_LOW);
		vTaskDelay(pdMS_TO_TICKS(100));
		mtk_os_hal_gpio_set_output(GPIO_LED, OS_HAL_GPIO_DATA_HIGH);
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

_Noreturn void RTCoreMain(void)
{
	NVIC_SetupVectorTable();

	mtk_os_hal_uart_ctlr_init(UART_PUTCHAR);

	xTaskCreate(BlinkTask, "Blink Task", 1024 / 4, NULL, 4, NULL);
	vTaskStartScheduler();

	for (;;)
    {
		__asm__("wfi");
	}
}
