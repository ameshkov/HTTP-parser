/*
 *  HTTP parser internals.
 *  Based on http parser API from Node.js project. 
 */
#include "nodejs_http_parser/http_parser.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
 *  Debug helpers:
 */

#define DBG_HTTP_CALLBACK \
     printf ("DEBUG: %s\n", __FUNCTION__); \

#define DBG_HTTP_CALLBACK_DATA \
    printf ("DEBUG: %s\n", __FUNCTION__); \
    char *out = malloc ((length + 1) * sizeof(char)); \
    memset (out, 0, length + 1); \
    memcpy (out, at, length); \
    printf("%s\n", out); \
    free(out); \

#define DBG_PARSER_TYPE \
    printf ("DEBUG: Parser type: %d\n", parser->type); 

/*
 *  Private stuff:
 */

typedef struct {
    connection_id           id;
    connection_info         *info;
    parser_callbacks        *callbacks;
    http_parser             *parser;
    http_parser_settings    *settings;
    http_header             *header;
    int                     have_body;
} connection_context;


/*
 *  Node.js http_parser's callbacks (parser->settings)
 */

/* For in-callback using only! */
#define CONTEXT   ((connection_context*)parser->data)
#define ID        ((CONTEXT)->id)
#define CALLBACKS ((CONTEXT)->callbacks)

int _on_message_begin(http_parser *parser) {
    return 0;
}

int _on_url(http_parser *parser, const char *at, size_t length) {
    return 0;
}

int _on_status(http_parser *parser, const char *at, size_t length) {
    return 0;
}

int _on_header_field(http_parser *parser, const char *at, size_t length) {
    return 0;
}

int _on_header_value(http_parser *parser, const char *at, size_t length) {
    return 0;
}

int _on_headers_complete(http_parser *parser) {
    int skip = 0;
    switch (parser->type) {
        case HTTP_REQUEST:
            skip = CALLBACKS->http_request_received(ID, NULL, 0);
            break;
        case HTTP_RESPONSE:
            skip = CALLBACKS->http_response_received(ID, NULL, 0);
            break;
        default:
            /**!-TODO: It's impossible in theory, some error handling needed... */
            break;
    }
    /**!-TODO: Handle message skipping here. */
    return 0;
}

int _on_body(http_parser *parser, const char *at, size_t length) {
    CONTEXT->have_body = 1;
    return 0;
}

int _on_message_complete(http_parser *parser) {
    if (CONTEXT->have_body) {
        switch (parser->type) {
            case HTTP_REQUEST:
                CALLBACKS->http_request_body_finished(ID, NULL, 0);
                break;
            case HTTP_RESPONSE:
                CALLBACKS->http_response_body_finished(ID, NULL, 0);
                break;
            default:
                /**!-TODO: It's impossible in theory, some error handling needed... */
                break;
        }
    }

    /* Re-init parser before next message. */
    http_parser_init(parser, HTTP_BOTH);
    CONTEXT->have_body = 0;

    return 0;
}

int _on_chunk_header(http_parser *parser) {
    return 0;
}

int _on_chunk_complete(http_parser *parser) {
    return 0;
}


http_parser_settings _settings = {
    .on_message_begin       = _on_message_begin,
    .on_url                 = _on_url,
    .on_status              = _on_status,
    .on_header_field        = _on_header_field,
    .on_header_value        = _on_header_value,
    .on_headers_complete    = _on_headers_complete,
    .on_body                = _on_body,
    .on_message_complete    = _on_message_complete,
    .on_chunk_header        = _on_chunk_header,
    .on_chunk_complete      = _on_chunk_complete
};

connection_context *context = NULL;

/*
 *  API implementaion:
 */

int connect(connection_id id, connection_info *info,
            parser_callbacks *callbacks) {
    context = malloc(sizeof(connection_context));
    memset (context, 0, sizeof(connection_context));
    
    context->id = id;
    context->info = info;
    context->callbacks = callbacks;

    context->settings = &_settings;
    context->parser = malloc(sizeof(http_parser));

    context->parser->data = context;
    
    http_parser_init(context->parser, HTTP_BOTH);

    return 0;
}

int disconnect(connection_id id, transfer_direction direction) {
    return 0;
}

int input(connection_id id, transfer_direction direction, const char *data,
          size_t length) {
    size_t done;
    done = http_parser_execute (context->parser, context->settings,
                                data, length);
    
    printf ("http_parser_execute - parsed %ld/%ld bytes.\n", done, length);
    return 0;
}

int close(connection_id id) {
    return 0;
}
