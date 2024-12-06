#pragma once
#include "network.h"

#include <stdbool.h>

struct LuantiClient;

typedef struct LuantiCallbacks {
    void (*onSeedRecieve) (struct LuantiClient* client, uint64_t seed);
    void (*onChatmessageRecieve) (struct LuantiClient* client, wchar_t* message, size_t msg_len);
    void (*onForcedDisconnect) (struct LuantiClient* client, char* reason, size_t reason_len, bool reconnect_suggested);
} LuantiCallbacks;

typedef struct LuantiClient {
    LuantiCallbacks callbacks;
    bool connected;
    conn_fd connection_fd;

    char* username;
    uint16_t username_len;

    uint16_t peer_id;
    uint16_t seqnum;
    uint16_t last_acked;
} LuantiClient;

void LuantiClient_connect(LuantiClient* client, char* password, char* address, uint16_t port);
void LuantiClient_disconnect(LuantiClient* client);
void LuantiClient_tick(LuantiClient* client, void* buff, size_t max_len);

void LuantiClient_send_chatmesage(LuantiClient* client, wchar_t* chatmessage);