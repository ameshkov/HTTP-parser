/*
 *  HTTP parser test.
 */
#include "parser.h"
#include "test_streams.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define INPUT_PORTION 150

/*
 *  Helpers:
 */
#define DBG_CALLBACK \
     printf ("CALLBACK [%s]:\n", __FUNCTION__);

/*
 *  User-defined callback for testing;
 */

int request_received(connection_id id, void *data, size_t length) {
    DBG_CALLBACK
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
