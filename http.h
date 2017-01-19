//
// Created by tyy on 2017/1/19.
//

#ifndef MAFIA_HTTP_H
#define MAFIA_HTTP_H

typedef struct httpclient_s  httpclient_t;


struct httpclient_s {
    struct event_base *base;
    struct evhttp_uri *uri;

    const char *path;
    struct evhttp_connection *conn;

    char *url;
    const char *host;
    int port;

    char *useragent;
    int status;
};

#endif //MAFIA_HTTP_H
