#ifndef POP3_H
#define POP3_H

#include <stdint.h>
#include "netstack.h"
#include "ssl.h"

// POP3 commands
#define POP3_CMD_USER "USER"
#define POP3_CMD_PASS "PASS"
#define POP3_CMD_QUIT "QUIT"
#define POP3_CMD_STAT "STAT"
#define POP3_CMD_LIST "LIST"
#define POP3_CMD_RETR "RETR"
#define POP3_CMD_DELE "DELE"
#define POP3_CMD_NOOP "NOOP"
#define POP3_CMD_RSET "RSET"
#define POP3_CMD_TOP  "TOP"
#define POP3_CMD_UIDL "UIDL"
#define POP3_CMD_APOP "APOP"

// POP3 response codes
#define POP3_OK  "+OK"
#define POP3_ERR "-ERR"

// POP3 connection states
typedef enum {
    POP3_STATE_DISCONNECTED,
    POP3_STATE_AUTHORIZATION,
    POP3_STATE_TRANSACTION,
    POP3_STATE_UPDATE
} pop3_state_t;

// Email message structure
typedef struct {
    uint32_t id;
    uint32_t size;
    char* uid;
    char* from;
    char* to;
    char* subject;
    char* date;
    void* content;
    size_t content_length;
    int deleted;
} pop3_message_t;

// POP3 session structure
typedef struct {
    socket_t* socket;
    ssl_connection_t* ssl;
    pop3_state_t state;
    char* username;
    char* password;
    uint32_t server_ip;
    uint16_t server_port;
    int use_ssl;
    char* server_timestamp;
    pop3_message_t** messages;
    size_t message_count;
    void (*progress_callback)(size_t current, size_t total);
} pop3_session_t;

// Initialize POP3 client
pop3_session_t* pop3_create_session(void);
void pop3_destroy_session(pop3_session_t* session);

// Connection management
int pop3_connect(pop3_session_t* session, uint32_t server_ip, uint16_t server_port, int use_ssl);
int pop3_disconnect(pop3_session_t* session);
int pop3_authenticate(pop3_session_t* session, const char* username, const char* password);

// Message operations
int pop3_get_message_count(pop3_session_t* session, size_t* count, size_t* total_size);
int pop3_list_messages(pop3_session_t* session);
pop3_message_t* pop3_retrieve_message(pop3_session_t* session, uint32_t msg_id);
int pop3_delete_message(pop3_session_t* session, uint32_t msg_id);
int pop3_undelete_messages(pop3_session_t* session);

// Message management
pop3_message_t* pop3_message_create(void);
void pop3_message_destroy(pop3_message_t* message);
int pop3_get_message_headers(pop3_session_t* session, uint32_t msg_id,
                           pop3_message_t* message);
int pop3_get_message_content(pop3_session_t* session, uint32_t msg_id,
                           pop3_message_t* message);

// APOP authentication
int pop3_authenticate_apop(pop3_session_t* session, const char* username,
                         const char* password);

// TOP command for partial message retrieval
int pop3_get_message_top(pop3_session_t* session, uint32_t msg_id,
                        uint32_t lines, char** headers, char** body);

// UIDL command for unique message IDs
int pop3_get_message_uid(pop3_session_t* session, uint32_t msg_id, char** uid);
int pop3_list_message_uids(pop3_session_t* session);

// Command functions
int pop3_send_command(pop3_session_t* session, const char* command,
                     const char* params);
int pop3_receive_response(pop3_session_t* session, char* buffer,
                         size_t size, int multi_line);

// Utility functions
const char* pop3_state_string(pop3_state_t state);
void pop3_set_progress_callback(pop3_session_t* session,
                              void (*callback)(size_t current, size_t total));

#endif 