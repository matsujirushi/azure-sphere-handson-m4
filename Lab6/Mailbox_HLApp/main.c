#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include <applibs/log.h>
#include <applibs/gpio.h>
#include <applibs/application.h>
#include <sys/time.h>
#include <sys/socket.h>

static const char RTAppCID[] = "0404a594-62bb-4fe6-b727-f387a6923ec7";

static int MailboxFd = -1;

int main(void)
{
    MailboxFd = Application_Connect(RTAppCID);
    if (MailboxFd < 0)
    {
        Log_Debug("ERROR: %s\n", strerror(errno));
        return 1;
    }
    const struct timeval recvTimeout = { .tv_sec = 5, .tv_usec = 0 };       // 5.0[sec.]
    int result = setsockopt(MailboxFd, SOL_SOCKET, SO_RCVTIMEO, &recvTimeout, sizeof(recvTimeout));

    // Send 
    Log_Debug("SEND: \"ping\"\n");
    send(MailboxFd, "ping", 4, 0);

    while (true)
    {
        // Receive
        uint8_t recvData[10];
        ssize_t recvSize = recv(MailboxFd, recvData, sizeof(recvData), 0);
        if (recvSize < 0)
        {
            Log_Debug("ERROR: %d %s\n", errno, strerror(errno));
            continue;
        }
        
        if (recvSize != 4)
        {
            Log_Debug("ERROR: size mismatch.\n");
            continue;
        }

        int count;
        memcpy(&count, recvData, sizeof(count));

        Log_Debug("RECEIVE: %d\n", count);
    }
}
