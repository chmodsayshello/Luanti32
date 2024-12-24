#include "luanticlient.h"

#include "network.h"
#include "clientpackets.h"
#include "serverpackets.h"

#include <lwip/sockets.h> // for htons()
#include <string.h>
#include <stdio.h>
#include "srp/srp.h"
#include <wchar.h>

#define MAX_PKT_SIZE 0x1ff

//   ________________________
//__/Login and authentication\__________________________

// Helper declariations

static void obtain_peer_id(LuantiClient* client);
static void toserver_init(LuantiClient* client);
static void toserver_srp_bytes_a (LuantiClient* client, char* password, struct SRPUser** srpuser);
static void receive_toclient_srp_bytes_s_b(LuantiClient* client, sp_toclient_srp_bytes_s_b* bytes_sb);
static void toserver_srp_bytes_m(LuantiClient* client, struct SRPUser* srpuser, sp_toclient_srp_bytes_s_b* sb);
static void toserver_init2(LuantiClient* client);
static void toserver_client_Ready(LuantiClient* client);
static void login(LuantiClient* client, char* password);

void LuantiClient_connect(LuantiClient* client, char* password, char* address, uint16_t port) {
    if (client->connected) {
        return;
    }

    if (!n_connect(address, port, &client->connection_fd))  {
        return;
    }
    client->connected = true;
    login(client, password);
}

void LuantiClient_disconnect(LuantiClient* client) {
    if (!client->connected) {
        return;
    }
    n_disconnect(client->connection_fd);
    client->connected = false;
}

static void login(LuantiClient* client, char* password) {
    obtain_peer_id(client);
    if (client->peer_id == 0) {
        LuantiClient_disconnect(client);
        return;
    }

    toserver_init(client);

    struct SRPUser* srpuser;
    toserver_srp_bytes_a(client, password, &srpuser);

    sp_toclient_srp_bytes_s_b bytes_sb;
    receive_toclient_srp_bytes_s_b(client, &bytes_sb);

    toserver_srp_bytes_m(client, srpuser, &bytes_sb);

    free(bytes_sb.s);
    free(bytes_sb.b);
    srp_user_delete(srpuser);

    toserver_init2(client);
    toserver_client_Ready(client);
}



static void obtain_peer_id(LuantiClient* client) {
    cp_request_peer_id pkt = REQUEST_PEER_ID;
    n_send(&pkt, sizeof(pkt), client->connection_fd);
    
    sp_assign_peer_id response;
    memset(&response, 0, sizeof(response));
    n_read(&response, sizeof(response), client->connection_fd);

    client->peer_id = response.peer_id_new;
}

static void toserver_init(LuantiClient* client) {
    client->seqnum = SEQNUM_INITIAL;
    cp_toserver_init pkt;
    cp_reliable_header_init(client, &pkt.header);
    pkt.header.command = 0x02;
    pkt.min_protocol_ver = 0x25;
    pkt.max_procotol_ver = 0x2a;
    pkt.ser_ver = 29;
    pkt.junk = 0;
    pkt.name_len = client->username_len;
    uint8_t* buff = malloc(sizeof(pkt) + client->username_len);
    assert(buff != NULL);
    memcpy(buff, &pkt, sizeof(pkt));
    memcpy(buff + sizeof(pkt), client->username, client->username_len);
    n_send(buff, sizeof(pkt) + client->username_len, client->connection_fd);

    free(buff);


    sp_toclient_hello response;
    n_read(&response, sizeof(response), client->connection_fd);
    //assertions to be made!
}

static void toserver_srp_bytes_a (LuantiClient* client, char* password, struct SRPUser** srpuser) {
    cp_toserver_srp_bytes_a bytesa;

    cp_reliable_header_init(client, &bytesa.header);
    bytesa.header.command = 0x51;
    bytesa.base = 1;

    *srpuser = srp_user_new(
        SRP_SHA256, SRP_NG_2048,
        client->username, client->username,
        (const unsigned char*) password, strlen(password),
        NULL, NULL
    );

    size_t bytes_len;
    uint8_t* bytes;
    SRP_Result res = srp_user_start_authentication(
        *srpuser, NULL, NULL, 0,
        (unsigned char**) &bytes, &bytes_len
    );
    bytesa.len = bytes_len;
    const size_t buf_pos = sizeof(bytesa) - sizeof(char*) - sizeof(uint8_t);
    uint8_t sendbuff[buf_pos + bytes_len + sizeof(uint8_t)];
    memcpy(sendbuff, &bytesa, buf_pos);
    memcpy((sendbuff + buf_pos), bytes, bytes_len);
    *(sendbuff + buf_pos + bytes_len) = bytesa.base;

    n_send(sendbuff, sizeof(bytesa) - sizeof(char*) + bytes_len, client->connection_fd);
}

static void receive_toclient_srp_bytes_s_b(LuantiClient* client, sp_toclient_srp_bytes_s_b* bytes_sb) {
    //Receive
    const size_t s_offset = sizeof(sp_pkt_header) + 2 * sizeof(uint16_t);
    uint8_t* recvbuf = calloc(MAX_PKT_SIZE, 1);
    assert(recvbuf != NULL);

    cp_reliable_header* pkt_header = (cp_reliable_header*) recvbuf;
    pkt_header->command = 0;

    // TODO: timeout
    while(pkt_header->command != CMD_TOCLIENT_SRP_BYTES_S_B) {
        memset(recvbuf, 0, MAX_PKT_SIZE);
        if (n_read(recvbuf, MAX_PKT_SIZE, client->connection_fd) < sizeof(sp_toclient_srp_bytes_s_b)) {
            continue;
        }
        pkt_header = (cp_reliable_header*) recvbuf;
    }

    memcpy(bytes_sb, recvbuf, s_offset);
    bytes_sb->size_s = 16; //TEMP!
    assert(sizeof(sp_toclient_srp_bytes_s_b) + bytes_sb->size_s < MAX_PKT_SIZE);
    bytes_sb->s = malloc(bytes_sb->size_s);
    assert(bytes_sb->s != NULL);
    memcpy(bytes_sb->s, recvbuf + s_offset, bytes_sb->size_s);

    bytes_sb->size_b = htons(*((uint16_t*) (recvbuf + s_offset + bytes_sb->size_s)));
    assert(sizeof(sp_toclient_srp_bytes_s_b) + bytes_sb->size_s + bytes_sb->size_b < MAX_PKT_SIZE);
    bytes_sb->b = malloc(bytes_sb->size_b);
    assert(bytes_sb->b != NULL);
    memcpy(bytes_sb->b, recvbuf + s_offset + bytes_sb->size_s + sizeof(uint16_t), bytes_sb->size_b);
    bytes_sb->size_s = bytes_sb->size_s;

    free(recvbuf);
}

static void toserver_srp_bytes_m(LuantiClient* client, struct SRPUser* srpuser, sp_toclient_srp_bytes_s_b* sb) {
    char* bytes_m;
    size_t size_m;
    srp_user_process_challenge(
        srpuser,
        sb->s, sb->size_s,
        sb->b, sb->size_b,
        &bytes_m, &size_m
    );

    cp_toserver_srp_bytes_m bytes_m_pkt;
    cp_reliable_header_init(client, &bytes_m_pkt.header);
    bytes_m_pkt.header.command = 0x52;
    bytes_m_pkt.len = size_m;

    uint8_t send_buf[sizeof(cp_reliable_header) + sizeof(uint16_t) + bytes_m_pkt.len];
    memcpy(send_buf, &bytes_m_pkt, sizeof(bytes_m_pkt) - sizeof(uint8_t*));
    memcpy(send_buf + sizeof(cp_reliable_header) + sizeof(uint16_t), bytes_m, size_m);
    n_send(send_buf, sizeof(cp_reliable_header) + sizeof(uint16_t) + bytes_m_pkt.len, client->connection_fd);
}

static void toserver_init2(LuantiClient* client) {
    cp_reliable_header init2;
    cp_reliable_header_init(client, &init2);
    init2.command = 0x11;
    n_send(&init2, sizeof(init2), client->connection_fd);
}

static void toserver_client_Ready(LuantiClient* client) {
    cp_toserver_client_ready ready;
    cp_reliable_header_init(client, &ready.header);
    ready.header.command = 0x43;
    ready.major = 5;
    ready.minor = 9;
    ready.patch = 0;
    ready.len = 0;
    n_send(&ready, sizeof(ready), client->connection_fd);
}



//   __________________________________
//__/Packet Identification and handling\______________________________

static void acknowledge_packet(LuantiClient* client, uint16_t seqnum, uint8_t channel);
static void handle_toclient_chat_message(LuantiClient* client, void* buffer, size_t size, size_t total_size);
static void handle_toclient_access_denied(LuantiClient* client, void* buffer, size_t size);

void LuantiClient_tick(LuantiClient* client, void* buff, size_t max_len) {
    if (!client->connected || max_len < sizeof(sp_generic_pkt)) {
        return;
    }

    const size_t read_size = n_read(buff, max_len, client->connection_fd);
    if (read_size < BASE_HEADER_SIZE) {
        return;
    }

    sp_generic_pkt* header = buff;
    const bool split = header->header.type == TYPE_SPLIT;
    uint16_t command;

    if (header->header.protocol_id != PROTOCOL_ID) {
       return;
    }

    if (split) {
        sp_generic_split_pkt* split_header = buff;
        acknowledge_packet(client, split_header->header.seqnum, split_header->header.channel);
        command = split_header->command;
    } else if (header->header.type != TYPE_RELIABLE) {
        header = buff + 2; // offset caused by the sequence number
        command = header->command;
    } else {
        acknowledge_packet(client, header->header.seqnum, header->header.channel);
        command = header->command;
    }

    switch (command) {
    
    case CMD_TOCLIENT_CHAT_MESSAGE:
        handle_toclient_chat_message(client, buff, read_size, max_len);
    break;

    case CMD_TOCLIENT_ACCESS_DENIED:
        handle_toclient_access_denied(client, buff, read_size);
    break;

    case CMD_TOCLIENT_HP:
        if (client->callbacks.onHpReceive == NULL) {
            break;
        }
        so_toclient_hp* hp_pkt = buff;
        client->callbacks.onHpReceive(client, hp_pkt->hp);
    break;
    }
}

static void acknowledge_packet(LuantiClient* client, uint16_t seqnum, uint8_t channel) {
    cp_reliable_ack pkt;
    pkt.protocol_id = PROTOCOL_ID;
    pkt.sender_peer_id = client->peer_id;
    pkt.channel = channel;
    pkt.type = TYPE_CONTROL;
    pkt.controltype = CONTROLTYPE_ACK;
    pkt.seqnum = seqnum;

    n_send(&pkt, sizeof(pkt), client->connection_fd);
}

static void handle_toclient_chat_message(LuantiClient* client, void* buffer, size_t size, size_t total_size) {
    if (client->callbacks.onChatmessageReceive == NULL || size < sizeof(sp_toclient_chat_message)) {
        // No handler, so no need to parse it properly...
        return;
    }

    sp_toclient_chat_message* chatpkt = (sp_toclient_chat_message*) buffer;

    if (sizeof(sp_toclient_chat_message) + chatpkt->msg_len > size) {
        perror("SIZE OF RECEIVED CHAT MESSAGE SMALLER THAN READ SIZE!");
        return;
    }

    wchar_t* msg;

    if (total_size - size < 2) {
        msg = calloc(chatpkt->msg_len + 1, sizeof(wchar_t));
        assert(msg != NULL);
        memcpy(msg, buffer + sizeof(sp_toclient_chat_message) + 1, chatpkt->msg_len * sizeof(wchar_t));
    } else {
        msg = buffer + sizeof(sp_toclient_chat_message) + 1;
        msg[chatpkt->msg_len] = '\0';
    }

    client->callbacks.onChatmessageReceive(client, msg, chatpkt->msg_len);    

    if (total_size - size < 2) {
        free(msg);
    }
}

static void handle_toclient_access_denied(LuantiClient* client, void* buffer, size_t size) {
    LuantiClient_disconnect(client);

    if (client->callbacks.onForcedDisconnect == NULL) {
        return;
    }

    assert(size > sizeof(sp_toclient_access_denied));
    sp_toclient_access_denied* pkt = (sp_toclient_access_denied*) buffer;
    assert(sizeof(sp_toclient_access_denied) + sizeof(bool) + pkt->reason_str_len <= size);

    // Save the boolean so we can safely overwrite it with a NULL-character
    // => the reason string can be extracted and passed without copying.

    bool reconnect = *(bool*) (buffer + size - 1);
    *(char*) (buffer + size - 1) = '\0';

    client->callbacks.onForcedDisconnect(client, (enum AccessDeniedCode) pkt->reason,
        buffer + sizeof(sp_toclient_access_denied), pkt->reason_str_len,
        reconnect
    );
}

//   _______
//__/Actions\___________________________________________________________
void LuantiClient_send_chatmesage(LuantiClient* client, wchar_t* chatmessage) {
    cp_toserver_chat_message msg_pkt;
    cp_reliable_header_init(client, &msg_pkt.header);
    msg_pkt.header.command = COMMAND_CHAT_MSG;
    msg_pkt.len = wcslen(chatmessage);

    uint8_t* outbuff = malloc(sizeof(msg_pkt) + msg_pkt.len * sizeof(wchar_t));
    assert(outbuff != NULL);
    memcpy(outbuff, &msg_pkt, sizeof(msg_pkt));

    // We can't just memcpy as we need to invert the byte order of every character
    wchar_t* writestr = outbuff + sizeof(msg_pkt);
    for (uint16_t i = 0; i < msg_pkt.len; i++) {
        writestr[i] = htons(chatmessage[i]);
    }

    n_send(outbuff, sizeof(msg_pkt) + msg_pkt.len * sizeof(wchar_t), client->connection_fd);
    free(outbuff);
}

void LuantiClient_respawn(LuantiClient* client) {
    cp_reliable_header header;
    cp_reliable_header_init(client, &header);
    header.command = CMD_TOSERVER_RESPAWN_LEGACY;

    n_send(&header, sizeof(header), client->connection_fd);
}