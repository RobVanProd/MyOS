#ifndef IMAP_H
#define IMAP_H

#include <stdint.h>
#include "netstack.h"
#include "ssl.h"

// IMAP states
typedef enum {
    IMAP_STATE_DISCONNECTED,
    IMAP_STATE_NOT_AUTHENTICATED,
    IMAP_STATE_AUTHENTICATED,
    IMAP_STATE_SELECTED,
    IMAP_STATE_LOGOUT
} imap_state_t;

// IMAP capabilities
typedef struct {
    int imap4rev1;
    int starttls;
    int login_disabled;
    int auth_plain;
    int auth_login;
    int auth_cram_md5;
    int idle;
    int namespace;
    int literal_plus;
    int quota;
    int acl;
    int uidplus;
    char* auth_mechanisms;
} imap_capabilities_t;

// IMAP mailbox flags
typedef struct {
    int noinferiors;
    int noselect;
    int marked;
    int unmarked;
    int has_children;
    int has_no_children;
} imap_mailbox_flags_t;

// IMAP mailbox structure
typedef struct {
    char* name;
    char* delimiter;
    imap_mailbox_flags_t flags;
    uint32_t messages;
    uint32_t recent;
    uint32_t unseen;
    uint32_t uidnext;
    uint32_t uidvalidity;
} imap_mailbox_t;

// IMAP message flags
typedef struct {
    int seen;
    int answered;
    int flagged;
    int deleted;
    int draft;
    int recent;
    char** custom;
    size_t custom_count;
} imap_message_flags_t;

// IMAP message structure
typedef struct {
    uint32_t uid;
    uint32_t sequence;
    char* subject;
    char* from;
    char* to;
    char* cc;
    char* date;
    size_t size;
    imap_message_flags_t flags;
    void* body;
    size_t body_length;
    char* body_structure;
} imap_message_t;

// IMAP session structure
typedef struct {
    socket_t* socket;
    ssl_connection_t* ssl;
    imap_state_t state;
    char* username;
    char* password;
    uint32_t server_ip;
    uint16_t server_port;
    int use_ssl;
    uint32_t tag_counter;
    imap_capabilities_t capabilities;
    imap_mailbox_t* current_mailbox;
    void (*progress_callback)(size_t current, size_t total);
} imap_session_t;

// Initialize IMAP client
imap_session_t* imap_create_session(void);
void imap_destroy_session(imap_session_t* session);

// Connection management
int imap_connect(imap_session_t* session, uint32_t server_ip, uint16_t server_port, int use_ssl);
int imap_disconnect(imap_session_t* session);
int imap_authenticate(imap_session_t* session, const char* username, const char* password);
int imap_capability(imap_session_t* session);
int imap_starttls(imap_session_t* session);

// Mailbox operations
int imap_list(imap_session_t* session, const char* reference, const char* mailbox,
              imap_mailbox_t** mailboxes, size_t* count);
int imap_select(imap_session_t* session, const char* mailbox);
int imap_create(imap_session_t* session, const char* mailbox);
int imap_delete(imap_session_t* session, const char* mailbox);
int imap_rename(imap_session_t* session, const char* old_name, const char* new_name);
int imap_subscribe(imap_session_t* session, const char* mailbox);
int imap_unsubscribe(imap_session_t* session, const char* mailbox);
int imap_status(imap_session_t* session, const char* mailbox, imap_mailbox_t* status);

// Message operations
int imap_fetch_messages(imap_session_t* session, const char* sequence_set,
                       const char* items, imap_message_t** messages, size_t* count);
int imap_fetch_message(imap_session_t* session, uint32_t uid,
                      imap_message_t* message);
int imap_store_flags(imap_session_t* session, const char* sequence_set,
                    const char* flags, int add);
int imap_copy_messages(imap_session_t* session, const char* sequence_set,
                      const char* mailbox);
int imap_move_messages(imap_session_t* session, const char* sequence_set,
                      const char* mailbox);
int imap_append_message(imap_session_t* session, const char* mailbox,
                       const void* message, size_t length,
                       const imap_message_flags_t* flags);

// Search operations
int imap_search(imap_session_t* session, const char* criteria,
                uint32_t** uids, size_t* count);
int imap_sort(imap_session_t* session, const char* sort_criteria,
              const char* search_criteria, uint32_t** uids, size_t* count);

// IDLE support
int imap_idle_start(imap_session_t* session);
int imap_idle_done(imap_session_t* session);

// Quota operations
int imap_get_quota(imap_session_t* session, const char* root,
                   uint64_t* used, uint64_t* total);
int imap_set_quota(imap_session_t* session, const char* root,
                   uint64_t limit);

// ACL operations
int imap_get_acl(imap_session_t* session, const char* mailbox,
                 char*** identifiers, char*** rights, size_t* count);
int imap_set_acl(imap_session_t* session, const char* mailbox,
                 const char* identifier, const char* rights);
int imap_delete_acl(imap_session_t* session, const char* mailbox,
                    const char* identifier);

// Utility functions
const char* imap_state_string(imap_state_t state);
void imap_set_progress_callback(imap_session_t* session,
                              void (*callback)(size_t current, size_t total));

// Memory management
imap_mailbox_t* imap_mailbox_create(void);
void imap_mailbox_destroy(imap_mailbox_t* mailbox);
imap_message_t* imap_message_create(void);
void imap_message_destroy(imap_message_t* message);

#endif 