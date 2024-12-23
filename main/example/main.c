#include <stdio.h>
#include "luanticlient.h"
#include <wchar.h>
#include "wifi.h"
#include <lwip/sockets.h>

#define SERVER_PORT 7777
#define SERVER_IP "IP GOES HERE"
#define PLAYER_NAME "chmodsayshello"
#define PASSWORD "123"


void onChatmessageReceive (struct LuantiClient* client, wchar_t* message, size_t msg_len) {
    wprintf(L"%ls\n", message);
}

void app_main(void) {
    connect_wifi();

    LuantiClient* client = calloc(1, sizeof(LuantiClient));
    client->username = PLAYER_NAME;
    client->username_len = strlen(PLAYER_NAME);
    client->callbacks.onChatmessageReceive = &onChatmessageReceive;

    LuantiClient_connect(client, PASSWORD, SERVER_IP, SERVER_PORT);

    int rcvbuf_size = NETBUFF_MCLA; // Desired receive buffer size in bytes
    int optlen = sizeof(rcvbuf_size);
    setsockopt(client->connection_fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf_size, optlen);

    uint8_t* buff = malloc(1024);
    LuantiClient_send_chatmesage(client, L"Hello from minetest on the esp32!");

    while (client->connected)
    {
        LuantiClient_tick(client, buff, 1024);
    }
    free(buff);
    free(client);
}
