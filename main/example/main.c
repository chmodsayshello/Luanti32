#include <stdio.h>
#include "luanticlient.h"
#include <wchar.h>
#include "wifi.h"
#include <lwip/sockets.h>
#include <math.h>

#define SERVER_PORT 7777
#define SERVER_IP "IP GOES HERE"
#define PLAYER_NAME "chmodsayshello"
#define PASSWORD "123"


void onChatmessageReceive (struct LuantiClient* client, wchar_t* message, size_t msg_len) {
    wprintf(L"%ls\n", message);
    if (wcsstr(message, L"!chat ") == message) {
        LuantiClient_send_chatmesage(client, message + 6);
    } else if (wcsstr(message, L"!disco") != NULL) {
        LuantiClient_disconnect(client);
    }
    if (wcsstr(message, L"!where") == message) {
        wchar_t answer[35];
        swprintf(answer, 35, L"I am at (%i|%i|%i)",
            (int) truncf(client->position.x),
            (int) truncf(client->position.y),
            (int) truncf(client->position.z)
        );
        LuantiClient_send_chatmesage(client, answer);
    }
}

void onHpReceive (struct LuantiClient* client, uint16_t hp) {
    printf("Received HP: %i\n", (int) hp);
    if (hp == 0) {
        // Note this won't work on modern servers (yet)
        LuantiClient_respawn(client);
    }
}

void onSEEDReceive (struct LuantiClient* client, uint64_t seed) {
    printf("Received Seed: %llu\n", seed);
}

void app_main(void) {
    connect_wifi();

    LuantiClient* client = calloc(1, sizeof(LuantiClient));
    client->username = PLAYER_NAME;
    client->username_len = strlen(PLAYER_NAME);
    client->callbacks.onChatmessageReceive = &onChatmessageReceive;
    client->callbacks.onHpReceive = &onHpReceive;
    client->callbacks.onSeedReceive = &onSEEDReceive;

    LuantiClient_connect(client, PASSWORD, SERVER_IP, SERVER_PORT);

    int rcvbuf_size = NETBUFF_MCLA * 2; // Desired receive buffer size in bytes
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
