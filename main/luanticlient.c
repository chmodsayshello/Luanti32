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
static void receive_toclient_srp_bytes_s_B(LuantiClient* client, sp_toclient_srp_bytes_s_b* bytes_sb);
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
    receive_toclient_srp_bytes_s_B(client, &bytes_sb);

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

static void receive_toclient_srp_bytes_s_B(LuantiClient* client, sp_toclient_srp_bytes_s_b* bytes_sb) {
    //Receive
    const size_t s_offset = sizeof(sp_pkt_header) + 2 * sizeof(uint16_t);
    uint8_t* recvbuf = malloc(MAX_PKT_SIZE);
    while(1) {
        memset(recvbuf, 0, MAX_PKT_SIZE);
        if(n_read(recvbuf, MAX_PKT_SIZE, client->connection_fd) > 200) {
            break;
        }
    }

    memcpy(bytes_sb, recvbuf, s_offset);
    bytes_sb->size_s = 16; //TEMP!
    bytes_sb->s = malloc(bytes_sb->size_s);
    assert(bytes_sb->s != NULL);
    memcpy(bytes_sb->s, recvbuf + s_offset, bytes_sb->size_s);

    bytes_sb->size_b = htons(*((uint16_t*) (recvbuf + s_offset + bytes_sb->size_s)));
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

static void acknowledge_packet(LuantiClient* client, uint16_t seqnum);
static void handle_toclient_chat_message(LuantiClient* client, void* buffer, size_t size, size_t total_size);

void LuantiClient_tick(LuantiClient* client, void* buff, size_t max_len) {
    if (!client->connected) {
        return;
    }

    sp_generic_pkt* header;
    size_t read_size;

    read_size = n_read(buff, max_len, client->connection_fd);
    if (read_size < sizeof(header)) {
        // No valid data read
        return;
    }

    header = buff;

    if (header->header.seqnum == client->last_acked) {
        return;
    }

    if (header->header.type != TYPE_RELIABLE || header->header.protocol_id != PROTOCOL_ID) {
        // We do not care about the few scenarios in whcih the server might send an unreliable packet
        return;
    } else {
        // This only is in an else so there are no mistakes to be made if this ever were to handle
        // unreliable packets
        acknowledge_packet(client, header->header.seqnum);
    }

    switch (header->command) {
    
    case CMD_TOCLIENT_CHAT_MESSAGE:
        handle_toclient_chat_message(client, buff, read_size, max_len);
    break;
    }
}

static void acknowledge_packet(LuantiClient* client, uint16_t seqnum) {
    client->last_acked = seqnum;
    cp_reliable_ack pkt;
    pkt.protocol_id = PROTOCOL_ID;
    pkt.sender_peer_id = client->peer_id;
    pkt.channel = 0;
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


    wchar_t* msg;

    if (total_size - size < 2) {
        msg = calloc(chatpkt->msg_len + 1, sizeof(wchar_t));
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

//   _______
//__/Actions\___________________________________________________________
void LuantiClient_send_chatmesage(LuantiClient* client, wchar_t* chatmessage) {
    cp_toserver_chat_message msg_pkt;
    cp_reliable_header_init(client, &msg_pkt.header);
    msg_pkt.header.command = COMMAND_CHAT_MSG;
    msg_pkt.len = wcslen(chatmessage);

    uint8_t* outbuff = malloc(sizeof(msg_pkt) + msg_pkt.len * sizeof(wchar_t));
    memcpy(outbuff, &msg_pkt, sizeof(msg_pkt));

    // We can't just memcpy as we need to invert the byte order of every character
    wchar_t* writestr = outbuff + sizeof(msg_pkt);
    for (uint16_t i = 0; i < msg_pkt.len; i++) {
        writestr[i] = htons(chatmessage[i]);
    }

    n_send(outbuff, sizeof(msg_pkt) + msg_pkt.len * sizeof(wchar_t), client->connection_fd);
    free(outbuff);
}
