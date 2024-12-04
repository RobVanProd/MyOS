#include "http.h"
#include "../memory.h"
#include <string.h>
#include <stdio.h>

// Initialize HTTP client
http_client_t* http_client_create(void) {
    http_client_t* client = kmalloc(sizeof(http_client_t));
    if (!client) return NULL;

    memset(client, 0, sizeof(http_client_t));
    client->user_agent = strdup("MyOS/1.0");
    return client;
}

void http_client_destroy(http_client_t* client) {
    if (!client) return;

    http_client_disconnect(client);
    if (client->host) kfree(client->host);
    if (client->user_agent) kfree(client->user_agent);
    http_clear_headers(&client->default_headers);
    kfree(client);
}

// Connection management
int http_client_connect(http_client_t* client, const char* host, uint16_t port, int use_ssl) {
    if (!client || !host) return -1;

    client->host = strdup(host);
    client->port = port;
    client->use_ssl = use_ssl;

    // Create socket
    client->socket = socket_create(SOCK_STREAM, 0);
    if (!client->socket) return -1;

    // Connect to server
    if (socket_connect(client->socket, 0, port) < 0) {
        socket_close(client->socket);
        client->socket = NULL;
        return -1;
    }

    // Initialize SSL if needed
    if (use_ssl) {
        ssl_context_t* ssl_ctx = ssl_context_create();
        if (!ssl_ctx) {
            socket_close(client->socket);
            client->socket = NULL;
            return -1;
        }

        client->ssl = ssl_connection_create(ssl_ctx, client->socket, 0);
        if (!client->ssl) {
            ssl_context_destroy(ssl_ctx);
            socket_close(client->socket);
            client->socket = NULL;
            return -1;
        }

        if (ssl_connection_handshake(client->ssl) < 0) {
            ssl_connection_destroy(client->ssl);
            client->ssl = NULL;
            socket_close(client->socket);
            client->socket = NULL;
            return -1;
        }
    }

    return 0;
}

void http_client_disconnect(http_client_t* client) {
    if (!client) return;

    if (client->ssl) {
        ssl_connection_destroy(client->ssl);
        client->ssl = NULL;
    }

    if (client->socket) {
        socket_close(client->socket);
        client->socket = NULL;
    }
}

// Request/Response management
http_request_t* http_request_create(const char* method, const char* url) {
    if (!method || !url) return NULL;

    http_request_t* request = kmalloc(sizeof(http_request_t));
    if (!request) return NULL;

    memset(request, 0, sizeof(http_request_t));
    request->method = strdup(method);
    request->url = strdup(url);
    request->version = strdup("HTTP/1.1");

    if (!request->method || !request->url || !request->version) {
        http_request_destroy(request);
        return NULL;
    }

    return request;
}

void http_request_destroy(http_request_t* request) {
    if (!request) return;

    if (request->method) kfree(request->method);
    if (request->url) kfree(request->url);
    if (request->version) kfree(request->version);
    http_clear_headers(&request->headers);
    if (request->body) kfree(request->body);
    kfree(request);
}

http_response_t* http_response_create(void) {
    http_response_t* response = kmalloc(sizeof(http_response_t));
    if (!response) return NULL;

    memset(response, 0, sizeof(http_response_t));
    response->version = strdup("HTTP/1.1");
    return response;
}

void http_response_destroy(http_response_t* response) {
    if (!response) return;

    if (response->version) kfree(response->version);
    if (response->status_text) kfree(response->status_text);
    http_clear_headers(&response->headers);
    if (response->body) kfree(response->body);
    kfree(response);
}

// Header management
int http_add_header(http_header_t** headers, const char* name, const char* value) {
    if (!headers || !name || !value) return -1;

    http_header_t* header = kmalloc(sizeof(http_header_t));
    if (!header) return -1;

    header->name = strdup(name);
    header->value = strdup(value);
    header->next = NULL;

    if (!header->name || !header->value) {
        if (header->name) kfree(header->name);
        if (header->value) kfree(header->value);
        kfree(header);
        return -1;
    }

    if (!*headers) {
        *headers = header;
    } else {
        http_header_t* current = *headers;
        while (current->next) {
            current = current->next;
        }
        current->next = header;
    }

    return 0;
}

const char* http_get_header(http_header_t* headers, const char* name) {
    if (!headers || !name) return NULL;

    http_header_t* current = headers;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current->value;
        }
        current = current->next;
    }

    return NULL;
}

void http_clear_headers(http_header_t** headers) {
    if (!headers) return;

    http_header_t* current = *headers;
    while (current) {
        http_header_t* next = current->next;
        if (current->name) kfree(current->name);
        if (current->value) kfree(current->value);
        kfree(current);
        current = next;
    }

    *headers = NULL;
}

// High-level HTTP methods
http_response_t* http_get(http_client_t* client, const char* url) {
    if (!client || !url) return NULL;

    http_request_t* request = http_request_create(HTTP_METHOD_GET, url);
    if (!request) return NULL;

    if (http_send_request(client, request) < 0) {
        http_request_destroy(request);
        return NULL;
    }

    http_request_destroy(request);
    return http_receive_response(client);
}

http_response_t* http_post(http_client_t* client, const char* url,
                          const void* body, size_t body_length,
                          const char* content_type) {
    if (!client || !url) return NULL;

    http_request_t* request = http_request_create(HTTP_METHOD_POST, url);
    if (!request) return NULL;

    // Add content headers
    char length_str[32];
    snprintf(length_str, sizeof(length_str), "%zu", body_length);
    http_add_header(&request->headers, HTTP_HEADER_CONTENT_LENGTH, length_str);
    http_add_header(&request->headers, HTTP_HEADER_CONTENT_TYPE, content_type);

    // Set body
    request->body = kmalloc(body_length);
    if (!request->body) {
        http_request_destroy(request);
        return NULL;
    }
    memcpy(request->body, body, body_length);
    request->body_length = body_length;

    if (http_send_request(client, request) < 0) {
        http_request_destroy(request);
        return NULL;
    }

    http_request_destroy(request);
    return http_receive_response(client);
}

// Low-level HTTP operations
int http_send_request(http_client_t* client, http_request_t* request) {
    if (!client || !request) return -1;

    // Format request line
    char buffer[4096];
    int length = snprintf(buffer, sizeof(buffer), "%s %s %s\r\n",
                         request->method, request->url, request->version);

    // Add default headers
    http_header_t* header = client->default_headers;
    while (header) {
        length += snprintf(buffer + length, sizeof(buffer) - length,
                          "%s: %s\r\n", header->name, header->value);
        header = header->next;
    }

    // Add request headers
    header = request->headers;
    while (header) {
        length += snprintf(buffer + length, sizeof(buffer) - length,
                          "%s: %s\r\n", header->name, header->value);
        header = header->next;
    }

    // Add empty line to mark end of headers
    length += snprintf(buffer + length, sizeof(buffer) - length, "\r\n");

    // Send headers
    if (client->use_ssl) {
        if (ssl_connection_send(client->ssl, buffer, length) < 0) {
            return -1;
        }
    } else {
        if (socket_send(client->socket, buffer, length) < 0) {
            return -1;
        }
    }

    // Send body if present
    if (request->body && request->body_length > 0) {
        if (client->use_ssl) {
            if (ssl_connection_send(client->ssl, request->body, request->body_length) < 0) {
                return -1;
            }
        } else {
            if (socket_send(client->socket, request->body, request->body_length) < 0) {
                return -1;
            }
        }
    }

    return 0;
}

http_response_t* http_receive_response(http_client_t* client) {
    if (!client) return NULL;

    http_response_t* response = http_response_create();
    if (!response) return NULL;

    char buffer[4096];
    int received;

    // Receive status line
    if (client->use_ssl) {
        received = ssl_connection_receive(client->ssl, buffer, sizeof(buffer));
    } else {
        received = socket_receive(client->socket, buffer, sizeof(buffer));
    }

    if (received <= 0) {
        http_response_destroy(response);
        return NULL;
    }

    // Parse status line
    char* line = strtok(buffer, "\r\n");
    if (!line) {
        http_response_destroy(response);
        return NULL;
    }

    char version[16];
    int status_code;
    char status_text[256];
    if (sscanf(line, "%s %d %[^\r\n]", version, &status_code, status_text) != 3) {
        http_response_destroy(response);
        return NULL;
    }

    response->status_code = status_code;
    response->status_text = strdup(status_text);

    // Parse headers
    while ((line = strtok(NULL, "\r\n")) != NULL && *line) {
        char* colon = strchr(line, ':');
        if (!colon) continue;

        *colon = '\0';
        char* value = colon + 1;
        while (*value == ' ') value++;

        http_add_header(&response->headers, line, value);
    }

    // Read body if present
    const char* content_length = http_get_header(response->headers, HTTP_HEADER_CONTENT_LENGTH);
    if (content_length) {
        size_t length = atoi(content_length);
        if (length > 0) {
            response->body = kmalloc(length);
            if (!response->body) {
                http_response_destroy(response);
                return NULL;
            }

            size_t total_received = 0;
            while (total_received < length) {
                if (client->use_ssl) {
                    received = ssl_connection_receive(client->ssl,
                                                   (char*)response->body + total_received,
                                                   length - total_received);
                } else {
                    received = socket_receive(client->socket,
                                           (char*)response->body + total_received,
                                           length - total_received);
                }

                if (received <= 0) {
                    http_response_destroy(response);
                    return NULL;
                }

                total_received += received;

                if (client->progress_callback) {
                    client->progress_callback(total_received, length);
                }
            }

            response->body_length = length;
        }
    }

    return response;
}

// Form data handling
http_form_data_t* http_form_data_create(void) {
    http_form_data_t* form_data = kmalloc(sizeof(http_form_data_t));
    if (!form_data) return NULL;

    memset(form_data, 0, sizeof(http_form_data_t));
    form_data->boundary = strdup("------------------------boundary");
    return form_data;
}

void http_form_data_destroy(http_form_data_t* form_data) {
    if (!form_data) return;

    for (size_t i = 0; i < form_data->field_count; i++) {
        if (form_data->fields[i]) {
            if (form_data->fields[i]->name) kfree(form_data->fields[i]->name);
            if (form_data->fields[i]->filename) kfree(form_data->fields[i]->filename);
            if (form_data->fields[i]->content_type) kfree(form_data->fields[i]->content_type);
            if (form_data->fields[i]->data) kfree(form_data->fields[i]->data);
            kfree(form_data->fields[i]);
        }
    }

    if (form_data->fields) kfree(form_data->fields);
    if (form_data->boundary) kfree(form_data->boundary);
    kfree(form_data);
}

// Utility functions
char* http_url_encode(const char* str) {
    if (!str) return NULL;

    size_t length = strlen(str);
    char* encoded = kmalloc(length * 3 + 1);
    if (!encoded) return NULL;

    char* p = encoded;
    while (*str) {
        if (isalnum(*str) || *str == '-' || *str == '_' || *str == '.' || *str == '~') {
            *p++ = *str;
        } else if (*str == ' ') {
            *p++ = '+';
        } else {
            sprintf(p, "%%%02X", (unsigned char)*str);
            p += 3;
        }
        str++;
    }
    *p = '\0';

    return encoded;
}

char* http_url_decode(const char* str) {
    if (!str) return NULL;

    size_t length = strlen(str);
    char* decoded = kmalloc(length + 1);
    if (!decoded) return NULL;

    char* p = decoded;
    while (*str) {
        if (*str == '%') {
            if (str[1] && str[2]) {
                char hex[3] = { str[1], str[2], 0 };
                *p++ = (char)strtol(hex, NULL, 16);
                str += 2;
            }
        } else if (*str == '+') {
            *p++ = ' ';
        } else {
            *p++ = *str;
        }
        str++;
    }
    *p = '\0';

    return decoded;
}

void http_set_user_agent(http_client_t* client, const char* user_agent) {
    if (!client) return;

    if (client->user_agent) {
        kfree(client->user_agent);
    }

    client->user_agent = strdup(user_agent);
}

void http_set_progress_callback(http_client_t* client,
                              void (*callback)(size_t current, size_t total)) {
    if (!client) return;
    client->progress_callback = callback;
} 