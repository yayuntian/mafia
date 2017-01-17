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


struct http_request {
    struct evhttp_uri *uri;
    struct event_base *base;
    struct evhttp_connection *cn;
    struct evhttp_request *req;

    const char *path;

    char *post_data;
    int post_len;

    int status;
};



char err_buf[8192] = {0,};
int err_len = 0;


static inline void print_request_head_info(struct evkeyvalq *header) {
    struct evkeyval *first_node = header->tqh_first;
    while (first_node) {
        printf("key:%s value:%s\n", first_node->key, first_node->value);
        first_node = first_node->next.tqe_next;
    }
}


void http_request_free(struct http_request *http_req) {
    evhttp_connection_free(http_req->cn);
    evhttp_uri_free(http_req->uri);

    free(http_req);
    http_req = NULL;
}


static void http_request_done(struct evhttp_request *req, void *ctx) {

    struct http_request *http_req = (struct http_request *) ctx;

    if (req == NULL) {
        /* If req is NULL, it means an error occurred, but
         * sadly we are mostly left guessing what the error
         * might have been.  We'll do our best... */
        int errcode = EVUTIL_SOCKET_ERROR();
        fprintf(stderr, "some request failed - no idea which one though!\n");

        http_req->status = -1;
        return;
    }


    http_req->status = 0;
//    fprintf(stderr, "Response line: %d %s\n",
//            evhttp_request_get_response_code(req),
//            evhttp_request_get_response_code_line(req));

    event_base_loopexit(http_req->base, NULL);
}


void *start_http_requset(struct http_request *http_req) {

    struct evkeyvalq *output_headers;
    struct evbuffer *output_buffer;

    http_req->req = evhttp_request_new(http_request_done, http_req);

    output_headers = evhttp_request_get_output_headers(http_req->req);

//    evhttp_add_header(output_headers, "Host", evhttp_uri_get_host(http_req->uri));

    /** Set the post data */
    output_buffer = evhttp_request_get_output_buffer(http_req->req);
    evbuffer_add(output_buffer, http_req->post_data, http_req->post_len);


    evhttp_make_request(http_req->cn, http_req->req, EVHTTP_REQ_POST, http_req->path);


    return http_req;
}


static char data[1024] = {0,};
static char ts[128] = {0,};
static int port = 0;

void gen_data() {
    memset(ts, 0, 128);
    time_t t = time(NULL);
    strftime(ts, 128, "%Y-%m-%dT%H:%M:%SZ", gmtime(&t));


    memset(data, 0, 1024);
    snprintf(data, 1024, "{\"@timestamp\":\"%s\","
                     "\"incr_item\":%d,"
                     "\"dawn_ts0\":1.483470142498e+15,"
                     "\"guid\":\"4a859fff6e5c4521aab187eee1cfceb8\","
                     "\"device_id\":\"26aae27e-ffe5-5fc8-9281-f82cf4e288ee\","
                     "\"probe\":{\"name\":\"cloudsensor\","
                     "\"hostname\":\"iZbp1gd3xwhcctm4ax2ruwZ\"},"
                     "\"appname\":\"cloudsensor\",\"type\":\"http\","
                     "\"kafka\":{\"topic\":\"cloudsensor\"},"
                     "\"aggregate_count\":1,\"http\":{\"latency_sec\":0,"
                     "\"in_bytes\":502,\"status_code\":200,"
                     "\"out_bytes\":8625,\"dst_port\":80,"
                     "\"src_ip\":2008838371,\"xff\":\"\","
                     "\"url\":\"\\/PHP\\/index.html\","
                     "\"refer\":\"\",\"l4_protocol\":\"tcp\","
                     "\"in_pkts\":1,\"http_method\":1,\"out_pkts\":6,"
                     "\"user_agent\":\"Mozilla\\/5.0 (Macintosh;"
                     " Intel Mac OS X 10_10_3) AppleWebKit\\/537.36 (KHTML,"
                     " like Gecko) Chrome\\/43.0.2357.130 Safari\\/537.36"
                     " JianKongBao Monitor 1.1\",\"dst_ip\":1916214160,"
                     "\"https_flag\":0,\"src_port\":43974,\"latency_usec\":527491,"
                     "\"host\":\"114.55.27.144\",\"url_query\":\"\"},"
                     "\"probe_ts\":1483470142,\"dawn_ts1\":1.483470142498e+15,"
                     "\"topic\":\"cloudsensor\"}",
             ts, port++);
}


int main(int argc, char *argv[]) {
    const char *url = "http://192.168.10.212:9200/cc-2017.01.17/test";

    struct event_base *base = event_base_new();

    struct http_request *http_req = calloc(1, sizeof(struct http_request));
    http_req->uri = evhttp_uri_parse(url);
    http_req->base = base;

    http_req->path = evhttp_uri_get_path(http_req->uri);

    http_req->path = http_req->path ? http_req->path : "/";


    int port = evhttp_uri_get_port(http_req->uri);
    http_req->cn = evhttp_connection_base_new(http_req->base,
                              NULL,
                              evhttp_uri_get_host(http_req->uri),
                              (port == -1 ? 80 : port));
    evhttp_connection_set_retries(http_req->cn, -1);
    evhttp_connection_set_timeout(http_req->cn, 10);

    struct timeval start, end;

    int i;
    int loop = 10000;
    gettimeofday(&start, NULL);
    for (i = 0; i < loop; i++) {
        if (http_req->status != -1) {
            gen_data();
        } else {
            printf("send last data failed, try again.\n");
        }

//        printf("%s\n", data);
        http_req->post_data = data;
        http_req->post_len = strlen(data);

        start_http_requset(http_req);

        event_base_dispatch(base);


//        sleep(1);
    }
    gettimeofday(&end, NULL);

    long time_cost = ((end.tv_sec - start.tv_sec) * 1000000 + \
            end.tv_usec - start.tv_usec);
    printf("cost time: %ld us, %.2f pps\n", time_cost, loop / (time_cost * 1.0) * 1000000);


    http_request_free(http_req);
    event_base_free(base);

    return 0;
}