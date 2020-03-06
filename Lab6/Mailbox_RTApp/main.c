#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "printf.h"
#include "mt3620.h"

#include "os_hal_uart.h"
#include "os_hal_gpio.h"

#include "mt3620-intercore.h"

static const uint8_t UART_PUTCHAR = OS_HAL_UART_PORT0;
static const uint8_t GPIO_BUTTON = OS_HAL_GPIO_12;

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

static BufferHeader* MailboxOutbound;
static BufferHeader* MailboxInbound;
static uint32_t MailboxSize = 0;
static const size_t MailboxPayloadHeader = 20;

static bool MailboxInitialize()
{
    if (GetIntercoreBuffers(&MailboxOutbound, &MailboxInbound, &MailboxSize) == -1)
    {
        printf("ERROR: GetIntercoreBuffers()\n");
        return false;
    }

    return true;
}

static ssize_t MailboxReceive(void* data, size_t size)
{
    uint8_t recvData[MailboxPayloadHeader + size];
    uint32_t dataSize = sizeof(recvData);

    if (DequeueData(MailboxOutbound, MailboxInbound, MailboxSize, recvData, &dataSize) != 0) return -1;

    if (dataSize < MailboxPayloadHeader) return -1;
    memcpy(data, recvData + MailboxPayloadHeader, dataSize - MailboxPayloadHeader);

    return dataSize - MailboxPayloadHeader;
}

static bool MailboxSend(const void* data, size_t size)
{
    uint8_t sendData[MailboxPayloadHeader + size];
    memset(sendData, 0, MailboxPayloadHeader);
    sendData[0] = 0x31;     // HLAppCID: 4bytes little-endian
    sendData[1] = 0x2b;
    sendData[2] = 0x94;
    sendData[3] = 0x30;
    sendData[4] = 0xbc;     // HLAppCID: 2bytes little-endian
    sendData[5] = 0xd6;
    sendData[6] = 0x63;     // HLAppCID: 2bytes little-endian
    sendData[7] = 0x44;
    sendData[8] = 0x8d;     // HLAppCID: 2bytes big-endian
    sendData[9] = 0x9d;
    sendData[10] = 0x3f;    // HLAppCID: 6bytes big-endian
    sendData[11] = 0xd2;
    sendData[12] = 0xde;
    sendData[13] = 0xb7;
    sendData[14] = 0x7d;
    sendData[15] = 0x05;
    memcpy(sendData + MailboxPayloadHeader, data, size);

    if (EnqueueData(MailboxInbound, MailboxOutbound, MailboxSize, sendData, sizeof(sendData)) != 0) return false;

    return true;
}

static void MailboxTask(void* pParameters)
{
	printf("Mailbox Task Started\n");

    // Initialize mailbox
    if (!MailboxInitialize())
    {
        printf("ERROR: MailboxInitialize()\n");
        return;
    }

    // Initialize button
    int ret = mtk_os_hal_gpio_request(GPIO_BUTTON);
	if (ret != 0)
	{
		printf("ERROR: mtk_os_hal_gpio_request()\n");
		return;
	}
	mtk_os_hal_gpio_set_direction(GPIO_BUTTON, OS_HAL_GPIO_DIR_INPUT);

    bool button;
	while (true)
	{
        // Receive message
        uint8_t recvData[10];
        ssize_t recvDataSize = MailboxReceive(recvData, sizeof(recvData));
        if (recvDataSize >= 0)
        {
            printf("RECEIVE: size=%d\n", recvDataSize);
        }

        // Get button state
        os_hal_gpio_data value;
        mtk_os_hal_gpio_get_input(GPIO_BUTTON, &value);
        bool preButton = button;
        button = value == OS_HAL_GPIO_DATA_LOW ? true : false;

        if (!preButton && button)
        {
            static int count = 0;
            ++count;

            // Send message
            printf("SEND: %d\n", count);
            if (!MailboxSend(&count, sizeof(count)))
            {
                printf("ERROR: MailboxSend()\n");
            }
        }

		vTaskDelay(pdMS_TO_TICKS(1));
	}
}

_Noreturn void RTCoreMain(void)
{
	NVIC_SetupVectorTable();

	mtk_os_hal_uart_ctlr_init(UART_PUTCHAR);

	xTaskCreate(MailboxTask, "Mailbox Task", 1024 / 4, NULL, 4, NULL);
	vTaskStartScheduler();

	for (;;)
    {
		__asm__("wfi");
	}
}
