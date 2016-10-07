/*
 *  HTTP parser API.
 */
#ifndef PARSER_H
#define PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

/*
 *  Globals:
 */
#define HTTP_VERSION_1_1    "HTTP/1.1"
#define HTTP_VERSION        (HTTP_VERSION_1_1)

/*
 *  Types:
 */
typedef struct {
    char *field;
    char *value;
} http_header_parameter;

typedef struct {
    char                    *method;
    char                    *url;
    char                    *status;
    unsigned int            status_code;
    unsigned int            paramc;
    http_header_parameter   *paramv;
} http_header;


typedef struct {
    http_header     *header;
    char            *body;
    size_t          body_length;
    unsigned long   chunkc;
    size_t         *chunkv;
    char            *dec_body;
    size_t          dec_body_length;
} http_message;

typedef unsigned long connection_id;

/*  Connection is represented by two abstract endpoints, which are titled for
    easy identification by C-strings. */
#define ENDPOINT_MAX 255

typedef struct {
    char endpoint_1[ENDPOINT_MAX];
    char endpoint_2[ENDPOINT_MAX];
} connection_info;


typedef enum transfer_direction transfer_direction;
enum transfer_direction {
    IN                 = 0,    /* from endpoint_1 to endpoint_2. */
    OUT                = 1     /* from endpoint_2 to endpoint_1. */
};

/*  User-defined callbacks.
    To inform parser about needed action, other then default "nothing to do", 
    callback should return non-zero code;
    In all other cases you should return 0. */
typedef int (*parser_cb)(connection_id id, void *data, size_t length);

typedef struct {
    parser_cb http_request_received;
    parser_cb http_request_body_started;
    parser_cb http_request_body_data;
    parser_cb http_request_body_finished;
    parser_cb http_response_received;
    parser_cb http_response_body_started;
    parser_cb http_response_body_data;
    parser_cb http_response_body_finished;
} parser_callbacks;

/*
 *  General parser interface:
 */
int parser_connect(connection_id id, connection_info *info,
            parser_callbacks *callbacks);

int parser_disconnect(connection_id id, transfer_direction direction);

int parser_input(connection_id id, transfer_direction direction, const char *data,
          size_t length);

int parser_connection_close(connection_id id);

/*
 *  Utility methods
 */
http_message *http_message_struct(void);

http_header *http_header_clone(const http_header *source);

http_message *http_message_clone(const http_message *source);

int http_message_set_method(http_message *message,
                            const char *method, size_t length);

int http_message_set_url(http_message *message,
                         const char *url, size_t length);

int http_message_set_status(http_message *message,
                            const char *status, size_t length);

int http_message_set_status_code(http_message *message, int status_code);

int http_message_set_field(http_message *message, 
                           const char *field, size_t f_length,
                           const char *value, size_t v_length);

char *http_message_get_field(const http_message *message,
                             const char *field, size_t length);

int http_message_add_field(http_message *message,
                           const char *field, size_t length);

int http_message_del_field(http_message *message,
                           const char *field, size_t length);

char *http_message_raw(const http_message *message);

#ifdef __cplusplus
}
#endif

#endif /* PARSER_H */
