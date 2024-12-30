/**
 * @file clientpackets.h
 * @author chmodsayshello (chmodsayshello@hotmail.com)
 * @brief clientpackets.h defines the layout of the packets the bot may send to the server.
 * @date 2024-12-30
 */
#pragma once
#include <stdint.h>

#include "luanticlient.h"

#define PROTOCOL_ID 0x4f457403
#define SEQNUM_INITIAL 65500
#define TYPE_CONTROL 0
#define CONTROLTYPE_ACK 0

#define BIGE __attribute__((scalar_storage_order("big-endian")))
#define PACK __attribute__((__packed__)) 
#define PKTSTRUCT typedef struct BIGE

PKTSTRUCT PACK cp_reliable_header {
    uint32_t protocol_id;
    uint16_t sender_peer_id;
    uint8_t channel;
    uint8_t type_reliable;
    uint16_t seqnum;
    uint8_t type;
    uint16_t command;
} cp_reliable_header;
void cp_reliable_header_init(LuantiClient* client, cp_reliable_header* pkt);

PKTSTRUCT PACK cp_request_peer_id {
    uint32_t protocol_id;
    uint16_t sender_peer_id;
    uint8_t channel;
    uint8_t type;
} cp_request_peer_id;
#define REQUEST_PEER_ID {PROTOCOL_ID, 0, 0, 3}

PKTSTRUCT PACK TOSERVER_INIT {
    cp_reliable_header header; // 0x02
    uint8_t ser_ver;
    uint16_t junk;
    uint16_t min_protocol_ver;
    uint16_t max_procotol_ver;
    uint16_t name_len;
    // string
} cp_toserver_init;

PKTSTRUCT PACK TOSERVER_SRP_BYTES_A {
    cp_reliable_header header;
    uint16_t len;
    uint8_t* bytes;
    uint8_t base; // should be always 1 in our case
} cp_toserver_srp_bytes_a;

PKTSTRUCT PACK TOSERVER_SRP_BYTES_M {
    cp_reliable_header header;
    uint16_t len;
    uint8_t* bytes;
} cp_toserver_srp_bytes_m;

PKTSTRUCT PACK TOSERVER_CLIENT_READY {
    cp_reliable_header header;
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
    uint8_t reserved;
    uint16_t len;
    //string
} cp_toserver_client_ready;

PKTSTRUCT PACK cp_reliable_ack {
    uint32_t protocol_id;
    uint16_t sender_peer_id;
    uint8_t channel;
    uint8_t type;
    uint8_t controltype;
    uint16_t seqnum;
} cp_reliable_ack;

PKTSTRUCT PACK TOSERVER_CHAT_MESSAGE {
    cp_reliable_header header;
    uint16_t len;
    //wstring
} cp_toserver_chat_message;
#define COMMAND_CHAT_MSG 0x32

#define CMD_TOSERVER_RESPAWN_LEGACY  0x38