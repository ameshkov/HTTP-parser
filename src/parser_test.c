/*
 *  HTTP parser test.
 */
#include "parser.h"
#include "test_streams.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define INPUT_PORTION 10

/*
 *  Helpers:
 */
#define DBG_CALLBACK

#ifndef DBG_CALLBACK
#define DBG_CALLBACK \
     printf ("CALLBACK [%s]:\n", __FUNCTION__);
#endif

void print_header(http_header *header) {
    if (!header) return;
    printf("\n");
    if (header->method) printf("METHOD: %s\n", header->method);
    if (header->url) printf("URL: %s\n", header->url);
    if (header->status) printf("STATUS: %s\n", header->status);
    if (header->status_code) printf("STATUS CODE: %d\n", header->status_code);
    for (int i = 0; i < header->paramc; i++) {
        printf ("%s: %s\n", header->paramv[i].field, header->paramv[i].value);
    }
    return;
}

/*
 *  User-defined callback for testing;
 */
int request_received(connection_id id, void *data, size_t length) {
    DBG_CALLBACK
    print_header((http_header *)((http_message *)data)->header);
    return 0;
}

int request_body_started(connection_id id, void *data, size_t length) {
    DBG_CALLBACK
    return 0;
}

int request_body_data(connection_id id, void *data, size_t length) {
    DBG_CALLBACK
    return 0;
}

int request_body_finished(connection_id id, void *data, size_t length) {
    DBG_CALLBACK
    return 0;
}

int response_received(connection_id id, void *data, size_t length) {
    DBG_CALLBACK
    print_header((http_header *)((http_message *)data)->header);
    return 0;
}

int response_body_started(connection_id id, void *data, size_t length) {
    DBG_CALLBACK
    return 0;
}

int response_body_data(connection_id id, void *data, size_t length) {
    DBG_CALLBACK
    return 0;
}

int response_body_finished(connection_id id, void *data, size_t length) {
    DBG_CALLBACK
    return 0;
}


parser_callbacks test_callbacks = {
    request_received,
    request_body_started,
    request_body_data,
    request_body_finished,
    response_received,
    response_body_started,
    response_body_data,
    response_body_finished
};


int main(int argc, char **argv) {
    int pos = 0, length = strlen (test_stream);
    parser_callbacks *callbacks = &test_callbacks; 

    connect (1, NULL, callbacks);

    do {
        input (1, 1, test_stream + pos, INPUT_PORTION);
    } while ((pos += INPUT_PORTION) < length);

    return 0;
}
