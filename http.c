//
// Created by tyy on 2017/1/19.
//

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include <event2/keyvalq_struct.h>
#include <event2/listener.h>
#include <event2/util.h>

#include "http.h"
#include "utils.h"


static void http_cb(struct evhttp_request *requset, void *ctx) {

    httpclient_t *client = (httpclient_t *) ctx;
    if (!requset) {
        fprintf(stderr, "request failed - please check network or server process!\n");
        client->status = -1;
        return;
    }

    client->status = 0;
    int code = evhttp_request_get_response_code(requset);
    const char *code_line = evhttp_request_get_response_code_line(requset);
    printf("Response: %d - %s\n", code, code_line);

    event_base_loopexit(client->base, NULL);
}


httpclient_t *http_init(const char *url) {
    httpclient_t *client = (httpclient_t *) malloc(sizeof(httpclient_t));
    if (client == NULL) {
        return NULL;
    }
    memset((void *) client, 0, sizeof(httpclient_t));


    client->base = event_base_new();
    client->uri = evhttp_uri_parse(url);

    client->host = evhttp_uri_get_host(client->uri);

    client->path = evhttp_uri_get_path(client->uri);
    if (!client->path) {
        client->path = "/";
    }
    int port = evhttp_uri_get_port(client->uri);
    client->port = (port == -1) ? 80 : port;

    client->conn = evhttp_connection_base_new(client->base,
                          NULL, client->host, client->port);

    evhttp_connection_set_retries(client->conn, -1);
    evhttp_connection_set_timeout(client->conn, 10);

    return client;
}


void http_exit(httpclient_t *client) {
    evhttp_connection_free(client->conn);
    evhttp_uri_free(client->uri);
    event_base_free(client->base);
    free(client);
}


int http_post(httpclient_t *client, char *data, int len) {

    struct evhttp_request * requset = evhttp_request_new(http_cb, (void *)client);

    if (client->useragent) {
        evhttp_add_header(requset->output_headers, "User-Agent", client->useragent);
    }
    evhttp_add_header(requset->output_headers, "Host", client->host);
    evhttp_add_header(requset->output_headers, "Connection", "Keep-Alive");

    evbuffer_add(requset->output_buffer, data, len);
    evhttp_make_request(client->conn, requset, EVHTTP_REQ_POST, client->path);

    event_base_dispatch(client->base);

    return 0;
}