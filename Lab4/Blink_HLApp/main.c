#include <stdbool.h>
#include <time.h>

#include <applibs/log.h>
#include <applibs/gpio.h>

static int LedFd = -1;

int main(void)
{
    LedFd = GPIO_OpenAsOutput(10, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (LedFd < 0) return 1;

    const struct timespec interval = { 0, 100000000 };  // 0.1[sec.]

    while (true)
    {
        Log_Debug("Turn on\n");
        GPIO_SetValue(LedFd, GPIO_Value_Low);
        nanosleep(&interval, NULL);

        Log_Debug("Turn off\n");
        GPIO_SetValue(LedFd, GPIO_Value_High);
        nanosleep(&interval, NULL);
    }
}
