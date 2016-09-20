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
#define DEBUG 0

#if !DEBUG
#   define DBG_PARSER_ERROR
#   define DBG_HTTP_CALLBACK
#   define DBG_HTTP_CALLBACK_DATA
#   define DBG_PARSER_TYPE
#else
#   define DBG_PARSER_ERROR                                                 \
        printf ("DEBUG: Parser error: %s\n",                                \
                http_errno_name(parser->http_errno));

#   define DBG_HTTP_CALLBACK                                                \
        printf ("DEBUG: %s\n", __FUNCTION__);

#   define DBG_HTTP_CALLBACK_DATA                                           \
        printf ("DEBUG: %s: ", __FUNCTION__);                               \
        char *out = malloc ((length + 1) * sizeof(char));                   \
        memset (out, 0, length + 1);                                        \
        memcpy (out, at, length);                                           \
        printf("%s\n", out);                                                \
        free(out); 

#   define DBG_PARSER_TYPE                                                  \
        printf ("DEBUG: Parser type: %d\n", parser->type);
#endif

/*
 *  Goods:
 */
#define MIN(a,b) a < b ? a : b
#define MAX(a,b) a > b ? a : b

#define CREATE_HTTP_HEADER(header)                                          \
    header = malloc(sizeof(http_header));                                   \
    memset(header, 0, sizeof(http_header));

#define DESTROY_HTTP_HEADER(header)                                         \
    http_header *hdr=header;                                                \
    if (hdr) {                                                              \
        if (hdr->url) free(hdr->url);                                       \
        if (hdr->status) free(hdr->status);                                 \
        for (int i = 0; i < hdr->paramc; i++) {                             \
            if (hdr->paramv[i].field) free(hdr->paramv[i].field);           \
            if (hdr->paramv[i].value) free(hdr->paramv[i].value);           \
        }                                                                   \
        if (hdr->paramv) free(hdr->paramv);                                 \
        free (hdr);                                                         \
        header=NULL;                                                        \
    }

#define APPEND_HTTP_HEADER_PARAM(paramc, paramv)                            \
    paramc++;                                                               \
    if (paramv == NULL) {                                                   \
        paramv = malloc(sizeof(http_header_parameter));                     \
        memset(paramv, 0, sizeof(http_header_parameter));                   \
    } else {                                                                \
        paramv = realloc(paramv, paramc * sizeof(http_header_parameter));   \
        memset(&(paramv[paramc - 1]), 0, sizeof(http_header_parameter));    \
    }

#define APPEND_CHARS(dst, src, len)                                         \
    size_t old_len;                                                         \
    if (dst == NULL) {                                                      \
        dst = malloc ((len + 1) * sizeof(char));                            \
        memset(dst, 0 , len + 1);                                           \
        memcpy(dst, at, len);                                               \
    } else {                                                                \
        old_len = strlen(dst);                                              \
        dst = realloc (dst, (old_len + len + 1) * sizeof(char));            \
        memset(dst + old_len, 0, len + 1);                                  \
        memcpy(dst + old_len, src, len);                                    \
    }


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
    unsigned int            in_field;
    unsigned int            have_body;
    unsigned int            body_started;
    unsigned int            need_decode;
    unsigned int            need_decompress;
} connection_context;


/*
 *  Node.js http_parser's callbacks (parser->settings):
 */
/* For in-callback using only! */
#define CONTEXT         ((connection_context*)parser->data)
#define ID              (CONTEXT->id)
#define CALLBACKS       (CONTEXT->callbacks)
#define IN_FIELD        (CONTEXT->in_field)
#define HAVE_BODY       (CONTEXT->have_body)
#define BODY_STARTED    (CONTEXT->body_started)
#define NEED_DECODE     (CONTEXT->need_decode)
#define NEED_DECOMPRESS (CONTEXT->need_decompress)

#define HEADER    (CONTEXT->header)
#define URL       (HEADER->url)
#define STATUS    (HEADER->status)
#define PARAMC    (HEADER->paramc)
#define PARAMV    (HEADER->paramv)


/*
 *  Internal callbacks:
 */
int _on_message_begin(http_parser *parser) {
    DBG_HTTP_CALLBACK
    CREATE_HTTP_HEADER(HEADER);
    return 0;
}

int _on_url(http_parser *parser, const char *at, size_t length) {
    DBG_HTTP_CALLBACK_DATA
    if (at != NULL && length > 0) {
        APPEND_CHARS(URL, at, length);
    }
    return 0;
}

int _on_status(http_parser *parser, const char *at, size_t length) {
    DBG_HTTP_CALLBACK_DATA
    if (at != NULL && length > 0) {
        APPEND_CHARS(STATUS, at, length);
    }
    return 0;
}

int _on_header_field(http_parser *parser, const char *at, size_t length) {
    DBG_HTTP_CALLBACK
    if (at != NULL && length > 0) {
        if (!IN_FIELD) {
            IN_FIELD = 1;
            APPEND_HTTP_HEADER_PARAM(PARAMC, PARAMV);
        }
        APPEND_CHARS(PARAMV[PARAMC - 1].field, at, length);
    }
    return 0;
}

int _on_header_value(http_parser *parser, const char *at, size_t length) {
    DBG_HTTP_CALLBACK
    IN_FIELD = 0;
    if (at != NULL && length > 0) {
        APPEND_CHARS(PARAMV[PARAMC - 1].value, at, length);
    }
    return 0;
}

int _on_headers_complete(http_parser *parser) {
    DBG_HTTP_CALLBACK
    int skip = 0;
    switch (parser->type) {
        case HTTP_REQUEST:
            skip = CALLBACKS->http_request_received(ID, HEADER, 
                                                    sizeof(HEADER));
            break;
        case HTTP_RESPONSE:
            skip = CALLBACKS->http_response_received(ID, HEADER, 
                                                     sizeof(HEADER));
            break;
        default:
            break;
    }

    DESTROY_HTTP_HEADER(HEADER);
    
    return skip;
}

int _on_body(http_parser *parser, const char *at, size_t length) {
    DBG_HTTP_CALLBACK_DATA
    HAVE_BODY = 1;
    switch (parser->type) {
        case HTTP_REQUEST:
            if (BODY_STARTED == 0) {
                CALLBACKS->http_request_body_started(ID, NULL, 0);
            }
            CALLBACKS->http_request_body_data(ID, NULL, 0);
            break;
        case HTTP_RESPONSE:
            if (BODY_STARTED == 0) {
                CALLBACKS->http_response_body_started(ID, NULL, 0);
            }
            CALLBACKS->http_response_body_data(ID, NULL, 0);
            break;
        default:
            break;
    }

    BODY_STARTED = 1;
    return 0;
}

int _on_message_complete(http_parser *parser) {
    DBG_HTTP_CALLBACK
    if (HAVE_BODY) {
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
    HAVE_BODY = 0;
    BODY_STARTED = 0;
    NEED_DECODE = 0;
    NEED_DECOMPRESS = 0;

    return 0;
}

int _on_chunk_header(http_parser *parser) {
    DBG_HTTP_CALLBACK
    if (BODY_STARTED == 0) {
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
        BODY_STARTED = 1;
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
    length = strnlen(data, length);
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
