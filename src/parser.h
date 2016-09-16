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
 *  Types:
 */
typedef struct {
    char *field;
    char *value;
} http_header_parameter;

typedef struct {
    http_header_parameter *params;
} http_header;

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

int connect(connection_id id, connection_info *info,
            parser_callbacks *callbacks);

int disconnect(connection_id id, transfer_direction direction);

int input(connection_id id, transfer_direction direction, const char *data,
          size_t length);

int close(connection_id id);

/*
 *  Utility methods
 */

/* To be continued... */

#ifdef __cplusplus
}
#endif

#endif /* PARSER_H */
