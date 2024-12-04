#include "pop3.h"
#include "../memory.h"
#include <string.h>
#include <stdio.h>

// Initialize POP3 client
pop3_session_t* pop3_create_session(void) {
    pop3_session_t* session = kmalloc(sizeof(pop3_session_t));
    if (!session) return NULL;

    memset(session, 0, sizeof(pop3_session_t));
    session->state = POP3_STATE_DISCONNECTED;
    session->server_port = 110;  // Default POP3 port
    return session;
}

void pop3_destroy_session(pop3_session_t* session) {
    if (!session) return;

    if (session->state != POP3_STATE_DISCONNECTED) {
        pop3_disconnect(session);
    }

    if (session->username) kfree(session->username);
    if (session->password) kfree(session->password);
    if (session->server_timestamp) kfree(session->server_timestamp);

    // Free messages
    for (size_t i = 0; i < session->message_count; i++) {
        if (session->messages[i]) {
            pop3_message_destroy(session->messages[i]);
        }
    }
    if (session->messages) kfree(session->messages);

    kfree(session);
}

// Connection management
int pop3_connect(pop3_session_t* session, uint32_t server_ip, uint16_t server_port, int use_ssl) {
    if (!session || session->state != POP3_STATE_DISCONNECTED) return -1;

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
    if (pop3_receive_response(session, response, sizeof(response), 0) < 0) {
        if (session->ssl) {
            ssl_connection_destroy(session->ssl);
            session->ssl = NULL;
        }
        socket_close(session->socket);
        session->socket = NULL;
        return -1;
    }

    // Extract server timestamp if present (for APOP)
    char* timestamp_start = strchr(response, '<');
    char* timestamp_end = strrchr(response, '>');
    if (timestamp_start && timestamp_end && timestamp_end > timestamp_start) {
        size_t length = timestamp_end - timestamp_start + 1;
        session->server_timestamp = kmalloc(length + 1);
        if (session->server_timestamp) {
            memcpy(session->server_timestamp, timestamp_start, length);
            session->server_timestamp[length] = '\0';
        }
    }

    session->state = POP3_STATE_AUTHORIZATION;
    return 0;
}

int pop3_disconnect(pop3_session_t* session) {
    if (!session || session->state == POP3_STATE_DISCONNECTED) return -1;

    // Send QUIT command
    pop3_send_command(session, POP3_CMD_QUIT, NULL);

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

    session->state = POP3_STATE_DISCONNECTED;
    return 0;
}

int pop3_authenticate(pop3_session_t* session, const char* username, const char* password) {
    if (!session || !username || !password) return -1;
    if (session->state != POP3_STATE_AUTHORIZATION) return -1;

    // Store credentials
    session->username = strdup(username);
    session->password = strdup(password);

    // Send username
    if (pop3_send_command(session, POP3_CMD_USER, username) < 0) {
        return -1;
    }

    char response[1024];
    if (pop3_receive_response(session, response, sizeof(response), 0) < 0) {
        return -1;
    }

    // Send password
    if (pop3_send_command(session, POP3_CMD_PASS, password) < 0) {
        return -1;
    }

    if (pop3_receive_response(session, response, sizeof(response), 0) < 0) {
        return -1;
    }

    session->state = POP3_STATE_TRANSACTION;
    return 0;
}

// Message operations
int pop3_get_message_count(pop3_session_t* session, size_t* count, size_t* total_size) {
    if (!session || !count || !total_size) return -1;
    if (session->state != POP3_STATE_TRANSACTION) return -1;

    // Send STAT command
    if (pop3_send_command(session, POP3_CMD_STAT, NULL) < 0) {
        return -1;
    }

    char response[1024];
    if (pop3_receive_response(session, response, sizeof(response), 0) < 0) {
        return -1;
    }

    // Parse response
    if (sscanf(response, "+OK %zu %zu", count, total_size) != 2) {
        return -1;
    }

    return 0;
}

int pop3_list_messages(pop3_session_t* session) {
    if (!session) return -1;
    if (session->state != POP3_STATE_TRANSACTION) return -1;

    // Send LIST command
    if (pop3_send_command(session, POP3_CMD_LIST, NULL) < 0) {
        return -1;
    }

    char response[1024];
    if (pop3_receive_response(session, response, sizeof(response), 1) < 0) {
        return -1;
    }

    // Parse response and create message list
    char* line = strtok(response, "\r\n");
    if (!line || strncmp(line, "+OK", 3) != 0) {
        return -1;
    }

    // Count messages
    size_t count = 0;
    while ((line = strtok(NULL, "\r\n")) != NULL && strcmp(line, ".") != 0) {
        count++;
    }

    // Allocate message array
    session->messages = krealloc(session->messages, count * sizeof(pop3_message_t*));
    if (!session->messages) return -1;

    // Parse message list
    line = strtok(response, "\r\n");  // Skip first line
    size_t index = 0;
    while ((line = strtok(NULL, "\r\n")) != NULL && strcmp(line, ".") != 0) {
        uint32_t id, size;
        if (sscanf(line, "%u %u", &id, &size) != 2) continue;

        pop3_message_t* message = pop3_message_create();
        if (!message) continue;

        message->id = id;
        message->size = size;
        session->messages[index++] = message;
    }

    session->message_count = count;
    return 0;
}

pop3_message_t* pop3_retrieve_message(pop3_session_t* session, uint32_t msg_id) {
    if (!session || session->state != POP3_STATE_TRANSACTION) return NULL;

    // Send RETR command
    char command[32];
    snprintf(command, sizeof(command), "%u", msg_id);
    if (pop3_send_command(session, POP3_CMD_RETR, command) < 0) {
        return NULL;
    }

    // Allocate buffer for message
    char* buffer = kmalloc(65536);  // 64KB initial buffer
    if (!buffer) return NULL;

    size_t buffer_size = 65536;
    size_t total_size = 0;

    // Receive message
    char line[1024];
    while (1) {
        int received = pop3_receive_response(session, line, sizeof(line), 0);
        if (received < 0) {
            kfree(buffer);
            return NULL;
        }

        // Check for end of message
        if (strcmp(line, ".") == 0) break;

        // Resize buffer if needed
        if (total_size + received + 2 > buffer_size) {
            buffer_size *= 2;
            char* new_buffer = krealloc(buffer, buffer_size);
            if (!new_buffer) {
                kfree(buffer);
                return NULL;
            }
            buffer = new_buffer;
        }

        // Append line to buffer
        memcpy(buffer + total_size, line, received);
        total_size += received;
        buffer[total_size++] = '\r';
        buffer[total_size++] = '\n';

        if (session->progress_callback) {
            session->progress_callback(total_size, buffer_size);
        }
    }

    // Create message object
    pop3_message_t* message = pop3_message_create();
    if (!message) {
        kfree(buffer);
        return NULL;
    }

    message->id = msg_id;
    message->content = buffer;
    message->content_length = total_size;

    // Parse headers
    pop3_get_message_headers(session, msg_id, message);

    return message;
}

// Message management
pop3_message_t* pop3_message_create(void) {
    pop3_message_t* message = kmalloc(sizeof(pop3_message_t));
    if (!message) return NULL;

    memset(message, 0, sizeof(pop3_message_t));
    return message;
}

void pop3_message_destroy(pop3_message_t* message) {
    if (!message) return;

    if (message->uid) kfree(message->uid);
    if (message->from) kfree(message->from);
    if (message->to) kfree(message->to);
    if (message->subject) kfree(message->subject);
    if (message->date) kfree(message->date);
    if (message->content) kfree(message->content);
    kfree(message);
}

// Command functions
int pop3_send_command(pop3_session_t* session, const char* command,
                     const char* params) {
    if (!session || !command) return -1;

    char buffer[1024];
    int length;

    if (params) {
        length = snprintf(buffer, sizeof(buffer), "%s %s\r\n", command, params);
    } else {
        length = snprintf(buffer, sizeof(buffer), "%s\r\n", command);
    }

    if (session->use_ssl) {
        return ssl_connection_send(session->ssl, buffer, length);
    } else {
        return socket_send(session->socket, buffer, length);
    }
}

int pop3_receive_response(pop3_session_t* session, char* buffer,
                         size_t size, int multi_line) {
    if (!session || !buffer) return -1;

    int total = 0;
    int in_body = 0;

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

        // Check for end of response
        if (!multi_line) {
            if (strstr(buffer, "\r\n")) break;
        } else {
            if (!in_body) {
                if (strstr(buffer, "\r\n")) in_body = 1;
            } else {
                if (strstr(buffer, "\r\n.\r\n")) break;
            }
        }
    }

    return total;
}

// Utility functions
const char* pop3_state_string(pop3_state_t state) {
    switch (state) {
        case POP3_STATE_DISCONNECTED:  return "Disconnected";
        case POP3_STATE_AUTHORIZATION: return "Authorization";
        case POP3_STATE_TRANSACTION:   return "Transaction";
        case POP3_STATE_UPDATE:        return "Update";
        default:                       return "Unknown";
    }
}

void pop3_set_progress_callback(pop3_session_t* session,
                              void (*callback)(size_t current, size_t total)) {
    if (!session) return;
    session->progress_callback = callback;
} 