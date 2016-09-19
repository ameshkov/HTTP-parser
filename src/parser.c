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
#define DEBUG 1

#define DBG_PARSER_ERROR \
    printf ("DEBUG: Parser error: %s\n", \
            http_errno_name(parser->http_errno));

#define DBG_HTTP_CALLBACK

#define DBG_HTTP_CALLBACK_1 \
    printf ("DEBUG: %s\n", __FUNCTION__); \
    DBG_PARSER_ERROR

#define DBG_HTTP_CALLBACK_DATA \
    printf ("DEBUG: %s\n", __FUNCTION__); \
    char *out = malloc ((length + 1) * sizeof(char)); \
    memset (out, 0, length + 1); \
    memcpy (out, at, length); \
    printf("%s\n", out); \
    free(out); 

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
    size_t                  done;
    int                     have_body;
    int                     body_started;
    int                     need_decode;
    int                     need_decompress;
} connection_context;


/*
 *  Node.js http_parser's callbacks (parser->settings)
 */

/* For in-callback using only! */
#define CONTEXT   ((connection_context*)parser->data)
#define ID        ((CONTEXT)->id)
#define CALLBACKS ((CONTEXT)->callbacks)

int _on_message_begin(http_parser *parser) {
    DBG_HTTP_CALLBACK
    return 0;
}

int _on_url(http_parser *parser, const char *at, size_t length) {
    DBG_HTTP_CALLBACK
    return 0;
}

int _on_status(http_parser *parser, const char *at, size_t length) {
    DBG_HTTP_CALLBACK
    return 0;
}

int _on_header_field(http_parser *parser, const char *at, size_t length) {
    DBG_HTTP_CALLBACK
    return 0;
}

int _on_header_value(http_parser *parser, const char *at, size_t length) {
    DBG_HTTP_CALLBACK
    return 0;
}

int _on_headers_complete(http_parser *parser) {
    DBG_HTTP_CALLBACK
    int skip = 0;
    switch (parser->type) {
        case HTTP_REQUEST:
            skip = CALLBACKS->http_request_received(ID, NULL, 0);
            break;
        case HTTP_RESPONSE:
            skip = CALLBACKS->http_response_received(ID, NULL, 0);
            break;
        default:
            break;
    }
    /**!-TODO: Handle message skipping here. */

    return 0;
}

int _on_body(http_parser *parser, const char *at, size_t length) {
    DBG_HTTP_CALLBACK
    CONTEXT->have_body = 1;
    switch (parser->type) {
        case HTTP_REQUEST:
            if (CONTEXT->body_started == 0) {
                CALLBACKS->http_request_body_started(ID, NULL, 0);
            }
            CALLBACKS->http_request_body_data(ID, NULL, 0);
            break;
        case HTTP_RESPONSE:
            if (CONTEXT->body_started == 0) {
                CALLBACKS->http_response_body_started(ID, NULL, 0);
            }
            CALLBACKS->http_response_body_data(ID, NULL, 0);
            break;
        default:
            break;
    }

    CONTEXT->body_started = 1;
    return 0;
}

int _on_message_complete(http_parser *parser) {
    DBG_HTTP_CALLBACK
    if (CONTEXT->have_body) {
        switch (parser->type) {
            case HTTP_REQUEST:
                CALLBACKS->http_request_body_finished(ID, NULL, 0);
                break;
            case HTTP_RESPONSE:
                CALLBACKS->http_response_body_finished(ID, NULL, 0);
                break;
            default:
                break;
        }
    }

    /* Re-init parser before next message. */
    http_parser_init(parser, HTTP_BOTH);

    CONTEXT->header = NULL;
    CONTEXT->have_body = 0;
    CONTEXT->body_started = 0;
    CONTEXT->need_decode = 0;
    CONTEXT->need_decompress = 0;

    return 0;
}

int _on_chunk_header(http_parser *parser) {
    DBG_HTTP_CALLBACK
    if (CONTEXT->body_started == 0) {
        switch (parser->type) {
            case HTTP_REQUEST:
                CALLBACKS->http_request_body_started(ID, NULL, 0);
                break;
            case HTTP_RESPONSE:
                CALLBACKS->http_response_body_started(ID, NULL, 0);
                break;
            default:
                break;
        }
        CONTEXT->body_started = 1;
    }
    return 0;
}

int _on_chunk_complete(http_parser *parser) {
    DBG_HTTP_CALLBACK
    switch (parser->type) {
        case HTTP_REQUEST:
            CALLBACKS->http_request_body_data(ID, NULL, 0);
            break;
        case HTTP_RESPONSE:
            CALLBACKS->http_response_body_data(ID, NULL, 0);
            break;
        default:
            break;
    }
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

#define INPUT_LENGTH_AT_ERROR 1

int input(connection_id id, transfer_direction direction, const char *data,
          size_t length) {
    context->done = 0;

    if (HTTP_PARSER_ERRNO(context->parser) != HPE_OK) {
        http_parser_init(context->parser, HTTP_BOTH);    
    }

    context->done = http_parser_execute (context->parser, context->settings,
                                         data, length);

    while (context->done < length)
    {
        if (HTTP_PARSER_ERRNO(context->parser) != HPE_OK) {
            http_parser_init(context->parser, HTTP_BOTH);    
        }
        http_parser_execute (context->parser, context->settings,
                             data + context->done, INPUT_LENGTH_AT_ERROR);
        context->done+=INPUT_LENGTH_AT_ERROR;
    }

    return 0;
}

int close(connection_id id) {
    return 0;
}
