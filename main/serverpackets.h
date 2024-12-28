#pragma once
#define PACK __attribute__((__packed__)) 
#define PKTSTRUCT typedef struct BIGE

#define TYPE_SPLIT 2
#define TYPE_RELIABLE 3

#define BASE_HEADER_SIZE 7

#include <stdint.h>
#include <stdbool.h>
#include "common.h"

#ifndef BIGE
#define BIGE __attribute__((scalar_storage_order("big-endian")))
#endif

PKTSTRUCT PACK sp_unreliable_pkt_header {
    uint32_t protocol_id;
    uint16_t sender_peer_id;
    uint8_t channel;
    uint8_t type;
} sp_unreliable_pkt_header;

PKTSTRUCT PACK sp_pkt_header {
    uint32_t protocol_id;
    uint16_t sender_peer_id;
    uint8_t channel;
    uint8_t type;
    uint16_t seqnum;
    uint8_t type2;
} sp_pkt_header;

PKTSTRUCT PACK sp_pkt_split_header {
    uint32_t protocol_id;
    uint16_t sender_peer_id;
    uint8_t channel;
    uint8_t type;
    uint16_t seqnum;
    uint16_t chunk_count;
    uint16_t chunk_num;
    uint8_t type2;
} sp_pkt_split_header;

PKTSTRUCT sp_assign_peer_id {
    sp_pkt_header header;
    uint8_t controltype;
    uint16_t peer_id_new;
} sp_assign_peer_id;

PKTSTRUCT PACK sp_generic_pkt {
    sp_pkt_header header;
    uint16_t command;
} sp_generic_pkt;

PKTSTRUCT PACK sp_generic_split_pkt {
    sp_pkt_split_header header;
    uint16_t command;
} sp_generic_split_pkt;

PKTSTRUCT PACK TOCLIENT_HELLO {
    sp_pkt_header header;
    uint8_t ser_ver;
    uint16_t network_compression; // currently unused
    uint16_t protocol_ver;
    uint32_t auth_methods;
} sp_toclient_hello;

PKTSTRUCT TOCLIENT_SRP_BYTES_S_B {
    sp_generic_pkt header;
    uint16_t size_s;
    uint8_t* s; // stored on the heap, so free!
    uint16_t size_b;
    uint8_t* b; // stored on the heap, so free!
} sp_toclient_srp_bytes_s_b;
#define CMD_TOCLIENT_SRP_BYTES_S_B 0x60

#define CMD_TOCLIENT_CHAT_MESSAGE 0x2f
PKTSTRUCT PACK TOCLIENT_CHAT_MESSAGE {
    sp_pkt_header header;
    uint16_t command;
    uint16_t version;
    uint16_t message_type;
    uint16_t msg_len;
    //wstring
} sp_toclient_chat_message;

PKTSTRUCT PACK TOCLIENT_ACCESS_DENIED {
    sp_generic_pkt header;
    uint8_t reason;
    uint16_t reason_str_len;
    // string
    // bool reconnect; // Can't be directly read as we do not know the size of the string
} sp_toclient_access_denied;
#define CMD_TOCLIENT_ACCESS_DENIED 0x0a

PKTSTRUCT PACK TOCLIENT_HP {
    sp_generic_pkt header;
    uint16_t hp;
} sp_toclient_hp;
#define CMD_TOCLIENT_HP 0x33

PKTSTRUCT PACK TOCLIENT_AUTH_ACCEPT {
    sp_unreliable_pkt_header header;
    uint16_t command;
    uint8_t junk[0x0d];
    uint64_t seed;
} sp_toclient_auth_accept;
#define CMD_TOCLIENT_AUTH_ACCEPT 0x03

PKTSTRUCT PACK TOCLIENT_MOVE_PLAYER {
    sp_generic_pkt header;
    v3f32 pos;
    float pitch;
    float yaw;
} sp_toclient_move_player;
#define CMD_TOCLIENT_MOVE_PLAYER 0x34