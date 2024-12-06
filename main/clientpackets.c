#include "clientpackets.h"

void cp_reliable_header_init(LuantiClient* client, cp_reliable_header* pkt) {
    pkt->protocol_id = PROTOCOL_ID;
    pkt->sender_peer_id = client->peer_id;
    pkt->channel = 0;
    pkt->type_reliable = 3;
    pkt->seqnum = client->seqnum++;
    pkt->type = 1;
}