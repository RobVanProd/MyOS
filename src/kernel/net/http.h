#ifndef HTTP_H
#define HTTP_H

#include <stdint.h>
#include "netstack.h"
#include "ssl.h"

// HTTP Methods
#define HTTP_METHOD_GET     "GET"
#define HTTP_METHOD_POST    "POST"
#define HTTP_METHOD_PUT     "PUT"
#define HTTP_METHOD_DELETE  "DELETE"
#define HTTP_METHOD_HEAD    "HEAD"
#define HTTP_METHOD_OPTIONS "OPTIONS"
#define HTTP_METHOD_PATCH   "PATCH"

// HTTP Status Codes
#define HTTP_STATUS_OK                  200
#define HTTP_STATUS_CREATED             201
#define HTTP_STATUS_ACCEPTED            202
#define HTTP_STATUS_NO_CONTENT          204
#define HTTP_STATUS_MOVED               301
#define HTTP_STATUS_FOUND               302
#define HTTP_STATUS_BAD_REQUEST         400
#define HTTP_STATUS_UNAUTHORIZED        401
#define HTTP_STATUS_FORBIDDEN           403
#define HTTP_STATUS_NOT_FOUND           404
#define HTTP_STATUS_SERVER_ERROR        500
#define HTTP_STATUS_NOT_IMPLEMENTED     501
#define HTTP_STATUS_SERVICE_UNAVAILABLE 503

// HTTP Header Fields
#define HTTP_HEADER_HOST            "Host"
#define HTTP_HEADER_CONTENT_TYPE    "Content-Type"
#define HTTP_HEADER_CONTENT_LENGTH  "Content-Length"
#define HTTP_HEADER_CONNECTION      "Connection"
#define HTTP_HEADER_USER_AGENT      "User-Agent"
#define HTTP_HEADER_ACCEPT          "Accept"
#define HTTP_HEADER_AUTHORIZATION   "Authorization"

// Content Types
#define HTTP_CONTENT_TYPE_HTML      "text/html"
#define HTTP_CONTENT_TYPE_TEXT      "text/plain"
#define HTTP_CONTENT_TYPE_JSON      "application/json"
#define HTTP_CONTENT_TYPE_XML       "application/xml"
#define HTTP_CONTENT_TYPE_FORM      "application/x-www-form-urlencoded"
#define HTTP_CONTENT_TYPE_MULTIPART "multipart/form-data"

// HTTP Header structure
typedef struct http_header {
    char* name;
    char* value;
    struct http_header* next;
} http_header_t;

// HTTP Request structure
typedef struct {
    char* method;
    char* url;
    char* version;
    http_header_t* headers;
    void* body;
    size_t body_length;
} http_request_t;

// HTTP Response structure
typedef struct {
    char* version;
    int status_code;
    char* status_text;
    http_header_t* headers;
    void* body;
    size_t body_length;
} http_response_t;

// HTTP Client structure
typedef struct {
    socket_t* socket;
    ssl_connection_t* ssl;
    char* host;
    uint16_t port;
    int use_ssl;
    char* user_agent;
    http_header_t* default_headers;
    void (*progress_callback)(size_t current, size_t total);
} http_client_t;

// Initialize HTTP client
http_client_t* http_client_create(void);
void http_client_destroy(http_client_t* client);

// Connection management
int http_client_connect(http_client_t* client, const char* host, uint16_t port, int use_ssl);
void http_client_disconnect(http_client_t* client);

// Request/Response management
http_request_t* http_request_create(const char* method, const char* url);
void http_request_destroy(http_request_t* request);
http_response_t* http_response_create(void);
void http_response_destroy(http_response_t* response);

// Header management
int http_add_header(http_header_t** headers, const char* name, const char* value);
const char* http_get_header(http_header_t* headers, const char* name);
void http_clear_headers(http_header_t** headers);

// High-level HTTP methods
http_response_t* http_get(http_client_t* client, const char* url);
http_response_t* http_post(http_client_t* client, const char* url, 
                          const void* body, size_t body_length,
                          const char* content_type);
http_response_t* http_put(http_client_t* client, const char* url,
                         const void* body, size_t body_length,
                         const char* content_type);
http_response_t* http_delete(http_client_t* client, const char* url);

// Low-level HTTP operations
int http_send_request(http_client_t* client, http_request_t* request);
http_response_t* http_receive_response(http_client_t* client);

// Form data handling
typedef struct {
    char* name;
    char* filename;
    char* content_type;
    void* data;
    size_t length;
} http_form_field_t;

typedef struct {
    http_form_field_t** fields;
    size_t field_count;
    char* boundary;
} http_form_data_t;

http_form_data_t* http_form_data_create(void);
void http_form_data_destroy(http_form_data_t* form_data);
int http_form_data_add_field(http_form_data_t* form_data, const char* name,
                            const void* data, size_t length);
int http_form_data_add_file(http_form_data_t* form_data, const char* name,
                           const char* filename, const char* content_type,
                           const void* data, size_t length);

// Cookie handling
typedef struct http_cookie {
    char* name;
    char* value;
    char* domain;
    char* path;
    uint32_t expires;
    int secure;
    int http_only;
    struct http_cookie* next;
} http_cookie_t;

int http_add_cookie(http_client_t* client, const char* name, const char* value);
const char* http_get_cookie(http_client_t* client, const char* name);
void http_clear_cookies(http_client_t* client);

// Authentication
int http_set_basic_auth(http_client_t* client, const char* username, const char* password);
int http_set_bearer_auth(http_client_t* client, const char* token);

// Utility functions
char* http_url_encode(const char* str);
char* http_url_decode(const char* str);
const char* http_status_text(int status_code);
void http_set_user_agent(http_client_t* client, const char* user_agent);
void http_set_progress_callback(http_client_t* client, 
                              void (*callback)(size_t current, size_t total));

#endif 