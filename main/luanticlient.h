/**
 * @file luanticlient.h
 * @author chmodsayshello (chmodsayshello@hotmail.com)
 * @brief luanticlient.h defines the interface users of the libary shall use to build their minetest bots.
 * @date 2024-12-30
 */
#pragma once
#include "network.h"
#include "common.h"

#include <stdbool.h>

#define NETBUFF_DEVTEST 0x14000
#define NETBUFF_MT_GAME 0x1e000
//Requires ESP32-S3; This may actually be more/less than needed (just used mt game * 3)
#define NETBUFF_MCLA    0x5a000

struct LuantiClient;
enum AccessDeniedCode;

/**
 * @struct LuantiCallbacks
 * @brief The LuantiCallbacks struct contains function pointers to all callback functions of a LuantiClient .
 * 
 * If you don't wish to write a callback function for one of these, set it's function pointer to NULL.
 * 
 * @var LuantiCallbacks::onSeedReceive
 * Callback called upon the server sending it's seed to the client.
 * <br> Please note that this doesn't fully work yet (some bits are cut off)!
 */
typedef struct LuantiCallbacks {
    void (*onHpReceive) (struct LuantiClient* client, uint16_t hp);
	void (*onSeedReceive) (struct LuantiClient* client, uint64_t seed);
	void (*onPosUpdate) (struct LuantiClient* client, float pitch, float yaw);
    void (*onChatmessageReceive) (struct LuantiClient* client, wchar_t* message, size_t msg_len);
    void (*onForcedDisconnect) (struct LuantiClient* client, enum AccessDeniedCode code, char* reason, size_t reason_len, bool reconnect_suggested);
} LuantiCallbacks;

/**
 * @struct LuantiClient
 * @brief The LuantiClient struct represents a Luanti client in memory.
 * 
 * After allowcation, it should be overwritten with 0 (i.e. using bzero() or memset()) so all function pointers are NULL.
 * <br> Set LuantiClient::username , LuantiClient::username_len and LuantiClient::callbacks accordingly before using LuantiClient_connect .
 */
typedef struct LuantiClient {
    LuantiCallbacks callbacks; /**< Contains function pointers to your callbacks. */
    bool connected; /**< Indicator for whether or not the client is currently connected to a server. */
    conn_fd connection_fd; /**< A file descriptor for the ongoing connection. To be set by LuantiClient_connect. */

    char* username; //!< The username to be used on the server.
					//!<<br> NULL-Termination is optional.
    uint16_t username_len; /**< The length of the username to be used in bytes, not including a potential NULL-byte. */

    uint16_t peer_id; /**< The peer id assigned by the Luanti server. To be set by LuantiClient_connect. */
    uint16_t seqnum; /**< The next outgoing sequence number for a reliable packet. Do not alter this. */
	v3f32 position; /**< The client's current position in the world of the Luanti Server. */
} LuantiClient;

/**
 * @brief The AccessDeniedCode enum represents a reason for which the server disconnected the client.
 * 
 * Copied from luanti's source, see <a href="https://github.com/minetest/minetest/blob/27c3aade5dc689db3e4eaa9c7273584bee6527ee
 * /src/network/networkprotocol.h#L902">src/network/networkprotocol.h in their codebase</a>.
 */
enum AccessDeniedCode {
	SERVER_ACCESSDENIED_WRONG_PASSWORD, /**< Wong password supplied */
	SERVER_ACCESSDENIED_UNEXPECTED_DATA, /**< Invalid data has been sent to the server */
	SERVER_ACCESSDENIED_SINGLEPLAYER, /**< The client has tried to connect to a singleplayer world */
	SERVER_ACCESSDENIED_WRONG_VERSION, /**< The client is on an incompatible version of the game */
	SERVER_ACCESSDENIED_WRONG_CHARS_IN_NAME, /**< The client uses non-whitelisted characters in it's username */
	SERVER_ACCESSDENIED_WRONG_NAME, /**< The username the client has chosen doesn't exist */
	SERVER_ACCESSDENIED_TOO_MANY_USERS, /**< The server is currently full and doesn't allow any more clients */
	SERVER_ACCESSDENIED_EMPTY_PASSWORD, /**< The client has sent an empty password */
	SERVER_ACCESSDENIED_ALREADY_CONNECTED, /**< There's another client with the same username already connected to the server */
	SERVER_ACCESSDENIED_SERVER_FAIL,
	SERVER_ACCESSDENIED_CUSTOM_STRING, /**< A custom reason */
	SERVER_ACCESSDENIED_SHUTDOWN, /**< The server has shut down */
	SERVER_ACCESSDENIED_CRASH, /**< The server has crashed */
	SERVER_ACCESSDENIED_MAX,
};
/**
 * @brief LuantiClient_connect conntects a client to a Luanti server
 * 
 * Make sure the following fields in your LuantiClient are set:
 * - LuantiClient::username
 * - LuantiClient::username_len
 * - ( LuantiClient::callbacks ) (can also be set/changed once client is connected)
 * 
 * @param client A pointer to the LuantiClient to connect
 * @param password The password in a (NULL-terminated) string
 * @param address The address as a (NULL-terminated) string
 * @param port The port as an unsigned 16-bit integer
 */
void LuantiClient_connect(LuantiClient* client, char* password, char* address, uint16_t port);

/**
 * @brief LuantiClient_disconnect disconnects a client from a Luanti Server
 * 
 * As of right now, this will just cause the client to time out, but that will be improved later on.
 * 
 * @param client A pointer to the LuantiClient to disconnect
 */
void LuantiClient_disconnect(LuantiClient* client);

/**
 * @brief LuantiClient_tick handles incomming packets
 * 
 * Try to run it as often as possible.
 * <br> Depending on what you are doing, it would make sense to make use of that second core.
 * 
 * A buffer size of 512 or more bytes is highly recommended
 * 
 * @param client A pointer to the LuantiClient to tick
 * @param buff A buffer which LuantiClient_tick may read incomming packets into
 * @param max_len The length of buff in bytes
 */
void LuantiClient_tick(LuantiClient* client, void* buff, size_t max_len);

/**
 * @brief LuantiClient_send_chatmessage sends a chat message
 * 
 * @param client A pointer to the LuantiClient to send the chat message
 * @param chatmessage A NULL-terminated wide string (use L"string here" for constants)
 */
void LuantiClient_send_chatmesage(LuantiClient* client, wchar_t* chatmessage);

/**
 * @brief LuantiClient_respawn respawns a client if dead in-game
 * 
 * @param client A pointer to the LuantiClient to respawn
 */
void LuantiClient_respawn(LuantiClient* client);
