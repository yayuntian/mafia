//
// Created by tyy on 2017/1/19.
//

#ifndef MAFIA_HTTP_H
#define MAFIA_HTTP_H

typedef struct httpclient_s {
    struct event_base *base;
    struct evhttp_uri *uri;

    const char *path;
    struct evhttp_connection *conn;

    char *url;
    const char *host;
    int port;

    char *useragent;
    int status;
} httpclient_t;


#define MAX_BULK_SIZE (32 * 1024 * 1024)

typedef struct bulk_s {
    char *data;
    int len;
    int offset;
    int count;

    char type[32];
    char guid[64];

    int batch;
    int time;
    int full;

    int last_time;

    volatile int lock;
} bulk_t;

extern bulk_t bulk;


httpclient_t *http_init(const char *url);
void http_exit(httpclient_t *client);
int http_post(httpclient_t *client, char *data, int len);

#endif //MAFIA_HTTP_H
