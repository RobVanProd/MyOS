#include "imap.h"
#include "../memory.h"
#include <string.h>
#include <stdio.h>

// Initialize IMAP client
imap_session_t* imap_create_session(void) {
    imap_session_t* session = kmalloc(sizeof(imap_session_t));
    if (!session) return NULL;

    memset(session, 0, sizeof(imap_session_t));
    session->state = IMAP_STATE_DISCONNECTED;
    session->server_port = 143;  // Default IMAP port
    session->tag_counter = 1;
    return session;
}

void imap_destroy_session(imap_session_t* session) {
    if (!session) return;

    if (session->state != IMAP_STATE_DISCONNECTED) {
        imap_disconnect(session);
    }

    if (session->username) kfree(session->username);
    if (session->password) kfree(session->password);
    if (session->capabilities.auth_mechanisms) kfree(session->capabilities.auth_mechanisms);
    if (session->current_mailbox) imap_mailbox_destroy(session->current_mailbox);

    kfree(session);
}

// Connection management
int imap_connect(imap_session_t* session, uint32_t server_ip, uint16_t server_port, int use_ssl) {
    if (!session || session->state != IMAP_STATE_DISCONNECTED) return -1;

    session->server_ip = server_ip;
    session->server_port = server_port;
    session->use_ssl = use_ssl;

    // Create socket
    session->socket = socket_create(SOCK_STREAM, 0);
    if (!session->socket) return -1;

    // Connect to server
    if (socket_connect(session->socket, server_ip, server_port) < 0) {
        socket_close(session->socket);
        session->socket = NULL;
        return -1;
    }

    // Initialize SSL if needed
    if (use_ssl) {
        ssl_context_t* ssl_ctx = ssl_context_create();
        if (!ssl_ctx) {
            socket_close(session->socket);
            session->socket = NULL;
            return -1;
        }

        session->ssl = ssl_connection_create(ssl_ctx, session->socket, 0);
        if (!session->ssl) {
            ssl_context_destroy(ssl_ctx);
            socket_close(session->socket);
            session->socket = NULL;
            return -1;
        }

        if (ssl_connection_handshake(session->ssl) < 0) {
            ssl_connection_destroy(session->ssl);
            session->ssl = NULL;
            socket_close(session->socket);
            session->socket = NULL;
            return -1;
        }
    }

    // Receive server greeting
    char response[1024];
    if (imap_receive_response(session, response, sizeof(response)) < 0) {
        if (session->ssl) {
            ssl_connection_destroy(session->ssl);
            session->ssl = NULL;
        }
        socket_close(session->socket);
        session->socket = NULL;
        return -1;
    }

    session->state = IMAP_STATE_NOT_AUTHENTICATED;

    // Get server capabilities
    return imap_capability(session);
}

int imap_disconnect(imap_session_t* session) {
    if (!session || session->state == IMAP_STATE_DISCONNECTED) return -1;

    // Send LOGOUT command
    char command[32];
    snprintf(command, sizeof(command), "A%u LOGOUT\r\n", session->tag_counter++);
    
    if (session->use_ssl) {
        ssl_connection_send(session->ssl, command, strlen(command));
    } else {
        socket_send(session->socket, command, strlen(command));
    }

    // Close SSL connection if active
    if (session->ssl) {
        ssl_connection_destroy(session->ssl);
        session->ssl = NULL;
    }

    // Close socket
    if (session->socket) {
        socket_close(session->socket);
        session->socket = NULL;
    }

    session->state = IMAP_STATE_DISCONNECTED;
    return 0;
}

int imap_authenticate(imap_session_t* session, const char* username, const char* password) {
    if (!session || !username || !password) return -1;
    if (session->state != IMAP_STATE_NOT_AUTHENTICATED) return -1;

    // Store credentials
    session->username = strdup(username);
    session->password = strdup(password);

    // Try LOGIN command first (if not disabled)
    if (!session->capabilities.login_disabled) {
        char command[1024];
        snprintf(command, sizeof(command), "A%u LOGIN \"%s\" \"%s\"\r\n",
                session->tag_counter++, username, password);

        if (session->use_ssl) {
            if (ssl_connection_send(session->ssl, command, strlen(command)) < 0) {
                return -1;
            }
        } else {
            if (socket_send(session->socket, command, strlen(command)) < 0) {
                return -1;
            }
        }

        char response[1024];
        if (imap_receive_response(session, response, sizeof(response)) < 0) {
            return -1;
        }

        if (strstr(response, "OK")) {
            session->state = IMAP_STATE_AUTHENTICATED;
            return 0;
        }
    }

    // If LOGIN failed or is disabled, try AUTHENTICATE PLAIN
    if (session->capabilities.auth_plain) {
        // TODO: Implement AUTHENTICATE PLAIN
        return -1;
    }

    return -1;
}

int imap_capability(imap_session_t* session) {
    if (!session) return -1;

    char command[32];
    snprintf(command, sizeof(command), "A%u CAPABILITY\r\n", session->tag_counter++);

    if (session->use_ssl) {
        if (ssl_connection_send(session->ssl, command, strlen(command)) < 0) {
            return -1;
        }
    } else {
        if (socket_send(session->socket, command, strlen(command)) < 0) {
            return -1;
        }
    }

    char response[1024];
    if (imap_receive_response(session, response, sizeof(response)) < 0) {
        return -1;
    }

    // Parse capabilities
    memset(&session->capabilities, 0, sizeof(imap_capabilities_t));

    char* cap_start = strstr(response, "CAPABILITY");
    if (!cap_start) return -1;

    cap_start += 10;  // Skip "CAPABILITY "
    char* cap_end = strstr(cap_start, "\r\n");
    if (!cap_end) return -1;

    *cap_end = '\0';
    char* cap = strtok(cap_start, " ");
    while (cap) {
        if (strcmp(cap, "IMAP4rev1") == 0) session->capabilities.imap4rev1 = 1;
        else if (strcmp(cap, "STARTTLS") == 0) session->capabilities.starttls = 1;
        else if (strcmp(cap, "LOGINDISABLED") == 0) session->capabilities.login_disabled = 1;
        else if (strcmp(cap, "AUTH=PLAIN") == 0) session->capabilities.auth_plain = 1;
        else if (strcmp(cap, "AUTH=LOGIN") == 0) session->capabilities.auth_login = 1;
        else if (strcmp(cap, "AUTH=CRAM-MD5") == 0) session->capabilities.auth_cram_md5 = 1;
        else if (strcmp(cap, "IDLE") == 0) session->capabilities.idle = 1;
        else if (strcmp(cap, "NAMESPACE") == 0) session->capabilities.namespace = 1;
        else if (strcmp(cap, "LITERAL+") == 0) session->capabilities.literal_plus = 1;
        else if (strcmp(cap, "QUOTA") == 0) session->capabilities.quota = 1;
        else if (strcmp(cap, "ACL") == 0) session->capabilities.acl = 1;
        else if (strcmp(cap, "UIDPLUS") == 0) session->capabilities.uidplus = 1;

        cap = strtok(NULL, " ");
    }

    return 0;
}

// Mailbox operations
int imap_select(imap_session_t* session, const char* mailbox) {
    if (!session || !mailbox) return -1;
    if (session->state != IMAP_STATE_AUTHENTICATED &&
        session->state != IMAP_STATE_SELECTED) return -1;

    char command[1024];
    snprintf(command, sizeof(command), "A%u SELECT \"%s\"\r\n",
             session->tag_counter++, mailbox);

    if (session->use_ssl) {
        if (ssl_connection_send(session->ssl, command, strlen(command)) < 0) {
            return -1;
        }
    } else {
        if (socket_send(session->socket, command, strlen(command)) < 0) {
            return -1;
        }
    }

    char response[4096];
    if (imap_receive_response(session, response, sizeof(response)) < 0) {
        return -1;
    }

    // Parse response and update mailbox information
    if (session->current_mailbox) {
        imap_mailbox_destroy(session->current_mailbox);
    }

    session->current_mailbox = imap_mailbox_create();
    if (!session->current_mailbox) return -1;

    session->current_mailbox->name = strdup(mailbox);

    // Parse EXISTS, RECENT, UNSEEN, etc.
    char* line = strtok(response, "\r\n");
    while (line) {
        if (strstr(line, "EXISTS")) {
            sscanf(line, "* %u EXISTS", &session->current_mailbox->messages);
        } else if (strstr(line, "RECENT")) {
            sscanf(line, "* %u RECENT", &session->current_mailbox->recent);
        } else if (strstr(line, "UNSEEN")) {
            sscanf(line, "* OK [UNSEEN %u]", &session->current_mailbox->unseen);
        } else if (strstr(line, "UIDNEXT")) {
            sscanf(line, "* OK [UIDNEXT %u]", &session->current_mailbox->uidnext);
        } else if (strstr(line, "UIDVALIDITY")) {
            sscanf(line, "* OK [UIDVALIDITY %u]", &session->current_mailbox->uidvalidity);
        }
        line = strtok(NULL, "\r\n");
    }

    session->state = IMAP_STATE_SELECTED;
    return 0;
}

// Message operations
int imap_fetch_message(imap_session_t* session, uint32_t uid, imap_message_t* message) {
    if (!session || !message) return -1;
    if (session->state != IMAP_STATE_SELECTED) return -1;

    char command[1024];
    snprintf(command, sizeof(command),
             "A%u UID FETCH %u (FLAGS INTERNALDATE RFC822.SIZE ENVELOPE BODY[])\r\n",
             session->tag_counter++, uid);

    if (session->use_ssl) {
        if (ssl_connection_send(session->ssl, command, strlen(command)) < 0) {
            return -1;
        }
    } else {
        if (socket_send(session->socket, command, strlen(command)) < 0) {
            return -1;
        }
    }

    // Allocate initial buffer
    size_t buffer_size = 65536;  // 64KB initial
    char* buffer = kmalloc(buffer_size);
    if (!buffer) return -1;

    size_t total_size = 0;
    int in_body = 0;
    size_t body_start = 0;

    // Receive response in chunks
    while (1) {
        if (total_size + 1024 > buffer_size) {
            buffer_size *= 2;
            char* new_buffer = krealloc(buffer, buffer_size);
            if (!new_buffer) {
                kfree(buffer);
                return -1;
            }
            buffer = new_buffer;
        }

        int received;
        if (session->use_ssl) {
            received = ssl_connection_receive(session->ssl,
                                           buffer + total_size,
                                           buffer_size - total_size);
        } else {
            received = socket_receive(session->socket,
                                   buffer + total_size,
                                   buffer_size - total_size);
        }

        if (received <= 0) {
            kfree(buffer);
            return -1;
        }

        total_size += received;
        buffer[total_size] = '\0';

        if (!in_body) {
            char* body_marker = strstr(buffer, "\r\n\r\n");
            if (body_marker) {
                in_body = 1;
                body_start = body_marker - buffer + 4;
            }
        }

        if (strstr(buffer, "\r\nA%u OK", session->tag_counter - 1)) {
            break;
        }

        if (session->progress_callback) {
            session->progress_callback(total_size, buffer_size);
        }
    }

    // Parse headers and metadata
    char* line = strtok(buffer, "\r\n");
    while (line && line < buffer + body_start) {
        if (strstr(line, "FLAGS")) {
            // Parse flags
            char* flags_start = strchr(line, '(');
            char* flags_end = strrchr(line, ')');
            if (flags_start && flags_end) {
                *flags_end = '\0';
                flags_start++;
                char* flag = strtok(flags_start, " ");
                while (flag) {
                    if (strcmp(flag, "\\Seen") == 0) message->flags.seen = 1;
                    else if (strcmp(flag, "\\Answered") == 0) message->flags.answered = 1;
                    else if (strcmp(flag, "\\Flagged") == 0) message->flags.flagged = 1;
                    else if (strcmp(flag, "\\Deleted") == 0) message->flags.deleted = 1;
                    else if (strcmp(flag, "\\Draft") == 0) message->flags.draft = 1;
                    else if (strcmp(flag, "\\Recent") == 0) message->flags.recent = 1;
                    flag = strtok(NULL, " ");
                }
            }
        } else if (strstr(line, "ENVELOPE")) {
            // Parse envelope
            // TODO: Implement envelope parsing
        }
        line = strtok(NULL, "\r\n");
    }

    // Store message body
    message->body = kmalloc(total_size - body_start);
    if (!message->body) {
        kfree(buffer);
        return -1;
    }
    memcpy(message->body, buffer + body_start, total_size - body_start);
    message->body_length = total_size - body_start;

    kfree(buffer);
    return 0;
}

// Memory management
imap_mailbox_t* imap_mailbox_create(void) {
    imap_mailbox_t* mailbox = kmalloc(sizeof(imap_mailbox_t));
    if (!mailbox) return NULL;

    memset(mailbox, 0, sizeof(imap_mailbox_t));
    return mailbox;
}

void imap_mailbox_destroy(imap_mailbox_t* mailbox) {
    if (!mailbox) return;

    if (mailbox->name) kfree(mailbox->name);
    if (mailbox->delimiter) kfree(mailbox->delimiter);
    kfree(mailbox);
}

imap_message_t* imap_message_create(void) {
    imap_message_t* message = kmalloc(sizeof(imap_message_t));
    if (!message) return NULL;

    memset(message, 0, sizeof(imap_message_t));
    return message;
}

void imap_message_destroy(imap_message_t* message) {
    if (!message) return;

    if (message->subject) kfree(message->subject);
    if (message->from) kfree(message->from);
    if (message->to) kfree(message->to);
    if (message->cc) kfree(message->cc);
    if (message->date) kfree(message->date);
    if (message->body) kfree(message->body);
    if (message->body_structure) kfree(message->body_structure);

    for (size_t i = 0; i < message->flags.custom_count; i++) {
        if (message->flags.custom[i]) kfree(message->flags.custom[i]);
    }
    if (message->flags.custom) kfree(message->flags.custom);

    kfree(message);
}

// Utility functions
const char* imap_state_string(imap_state_t state) {
    switch (state) {
        case IMAP_STATE_DISCONNECTED:     return "Disconnected";
        case IMAP_STATE_NOT_AUTHENTICATED: return "Not Authenticated";
        case IMAP_STATE_AUTHENTICATED:    return "Authenticated";
        case IMAP_STATE_SELECTED:         return "Selected";
        case IMAP_STATE_LOGOUT:           return "Logout";
        default:                          return "Unknown";
    }
}

void imap_set_progress_callback(imap_session_t* session,
                              void (*callback)(size_t current, size_t total)) {
    if (!session) return;
    session->progress_callback = callback;
}

// Internal helper function
static int imap_receive_response(imap_session_t* session, char* buffer, size_t size) {
    if (!session || !buffer) return -1;

    int total = 0;
    char tag[16];
    snprintf(tag, sizeof(tag), "A%u", session->tag_counter - 1);

    while (1) {
        int received;
        if (session->use_ssl) {
            received = ssl_connection_receive(session->ssl,
                                           buffer + total,
                                           size - total - 1);
        } else {
            received = socket_receive(session->socket,
                                   buffer + total,
                                   size - total - 1);
        }

        if (received <= 0) return -1;
        total += received;
        buffer[total] = '\0';

        if (strstr(buffer, tag)) break;
    }

    return total;
} 