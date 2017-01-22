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


static char *post_data = "{\"dawn_ts0\":1.483470142498e+15,\"guid\""
        ":\"4a859fff6e5c4521aab187eee1cfceb8\",\"device_id\":\"26aae27e-ffe5-5fc8-9281-f8"
        "2cf4e288ee\",\"probe\":{\"name\":\"cloudsensor\",\"hostname\":\"iZbp1gd3xwhcctm4ax2ruwZ"
        "\"},\"appname\":\"cloudsensor\",\"type\":\"http\",\"kafka\":{\"topic\":\"cloudsensor\"},\"ag"
        "gregate_count\":1,\"http\":{\"latency_sec\":0,\"in_bytes\":502,\"status_code\":200,"
        "\"out_bytes\":8625,\"dst_port\":80,\"src_ip\":2008838371,\"xff\":\"\",\"url\":\"/PHP"
        "/index.html\",\"refer\":\"\",\"l4_protocol\":\"tcp\",\"in_pkts\":1,\"http_method\":1"
        ",\"out_pkts\":6,\"user_agent\":\"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_3"
        ") AppleWebKit/537.36 (KHTML, like Gecko) Chrome/43.0.2357.130 Safari/537.36 Jia"
        "nKongBao Monitor 1.1\",\"dst_ip\":1916214160,\"https_flag\":0,\"src_port\":43974,\""
        "latency_usec\":527491,\"host\":\"114.55.27.144\",\"url_query\":\"\"},\"probe_ts\":14"
        "83470142,\"dawn_ts1\":1.483470142498e+15,\"topic\":\"cloudsensor\"}";


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

#define BULK_COUNT 10000
char *gen_bulk_data(char *data, int len) {
    int i;
    int offset = 0;

    char *bulk_data = calloc(1, MAX_BULK_SIZE);
    if (!bulk_data) {
        fprintf(stderr, "calloc bulk data failed\n");
        exit(1);
    }

    const char *index = "{\"create\":{\"_index\":\"cc-test-2017.01.19\",\"_type\":\"http\"}}\n";
    int index_len = strlen(index);

    for (i = 0; i < BULK_COUNT; i++) {
        strncpy(bulk_data + offset, index, index_len);
        offset += index_len;
        strncpy(bulk_data + offset, data, len);
        offset += len;
        bulk_data[offset++] = '\n';
    }

    return bulk_data;
}


long time_current_usec() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long time = (tv.tv_sec * 1e6) + tv.tv_usec;
    return time;
}

#if 0
int main(int argc, char *argv[]) {
    int i;
    long start, end;

    if (argc < 2) {
        printf("Usage: %s <url>\n", argv[0]);
        exit(1);
    }

//    const char *url = "http://192.168.10.212:9200/_bulk";
    char *url = argv[1];
    httpclient_t *client = http_init(url);

    char *data = gen_bulk_data(post_data, strlen(post_data));
    int len = strlen(data);

    int loop = 1000;
    start = time_current_usec();
    for (i = 0; i < loop; i++) {
        http_post(client, data, len);

        printf("%.2f pps\n", (i + 1) * BULK_COUNT / (time_current_usec() - start * 1.0) * 1e6);
    }
    end = time_current_usec();

    long time_cost = end - start;
    printf("cost time: %ld us, %.2f pps\n", time_cost,
           loop * BULK_COUNT / (time_cost * 1.0) * 1e6);

    http_exit(client);

    return 0;
}
#endif