#pragma once
#define BIGE __attribute__((scalar_storage_order("big-endian")))
#define PACK __attribute__((__packed__)) 
#define PKTSTRUCT typedef struct BIGE

#define TYPE_RELIABLE 3

#include <stdint.h>

PKTSTRUCT PACK sp_pkt_header {
    uint32_t protocol_id;
    uint16_t sender_peer_id;
    uint8_t channel;
    uint8_t type;
    uint16_t seqnum;
    uint8_t type2;
} sp_pkt_header;

PKTSTRUCT sp_assign_peer_id {
    sp_pkt_header header;
    uint8_t controltype;
    uint16_t peer_id_new;
} sp_assign_peer_id;

PKTSTRUCT PACK sp_generic_pkt {
    sp_pkt_header header;
    uint16_t command;
} sp_generic_pkt;

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