#pragma once
#include "network.h"

#include <stdbool.h>

struct LuantiClient;
enum AccessDeniedCode;

typedef struct LuantiCallbacks {
    void (*onSeedReceive) (struct LuantiClient* client, uint64_t seed);
    void (*onChatmessageReceive) (struct LuantiClient* client, wchar_t* message, size_t msg_len);
    void (*onForcedDisconnect) (struct LuantiClient* client, enum AccessDeniedCode code, char* reason, size_t reason_len, bool reconnect_suggested);
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

// See networkprotocol.h in minetest/luanti source code
enum AccessDeniedCode {
	SERVER_ACCESSDENIED_WRONG_PASSWORD,
	SERVER_ACCESSDENIED_UNEXPECTED_DATA,
	SERVER_ACCESSDENIED_SINGLEPLAYER,
	SERVER_ACCESSDENIED_WRONG_VERSION,
	SERVER_ACCESSDENIED_WRONG_CHARS_IN_NAME,
	SERVER_ACCESSDENIED_WRONG_NAME,
	SERVER_ACCESSDENIED_TOO_MANY_USERS,
	SERVER_ACCESSDENIED_EMPTY_PASSWORD,
	SERVER_ACCESSDENIED_ALREADY_CONNECTED,
	SERVER_ACCESSDENIED_SERVER_FAIL,
	SERVER_ACCESSDENIED_CUSTOM_STRING,
	SERVER_ACCESSDENIED_SHUTDOWN,
	SERVER_ACCESSDENIED_CRASH,
	SERVER_ACCESSDENIED_MAX,
};

void LuantiClient_connect(LuantiClient* client, char* password, char* address, uint16_t port);
void LuantiClient_disconnect(LuantiClient* client);
void LuantiClient_tick(LuantiClient* client, void* buff, size_t max_len);

void LuantiClient_send_chatmesage(LuantiClient* client, wchar_t* chatmessage);
