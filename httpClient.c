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
    char *post_data;
    int post_len;
};


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

    if (http_req->post_data) {
        free(http_req->post_data);
    }
    free(http_req);
    http_req = NULL;
}


static void http_request_done(struct evhttp_request *req, void *ctx) {
    char buffer[256];
    int nread;

    if (req == NULL) {
        /* If req is NULL, it means an error occurred, but
         * sadly we are mostly left guessing what the error
         * might have been.  We'll do our best... */
        int errcode = EVUTIL_SOCKET_ERROR();
        fprintf(stderr, "some request failed - no idea which one though!\n");

        fprintf(stderr, "socket error = %s (%d)\n",
               evutil_socket_error_to_string(errcode), errcode);
        return;
    }

    fprintf(stderr, "Response line: %d %s\n",
            evhttp_request_get_response_code(req),
            evhttp_request_get_response_code_line(req));

    switch(req->response_code) {
        case HTTP_OK:
        {
            struct evbuffer* buf = evhttp_request_get_input_buffer(req);
            size_t len = evbuffer_get_length(buf);

            char *tmp = malloc(len+1);
            memcpy(tmp, evbuffer_pullup(buf, -1), len);
            tmp[len] = '\0';
            printf("%s\n", tmp);
            free(tmp);
            break;
        }
        default:
            break;
    }
}


void *start_http_requset(struct http_request *http_req) {

    struct evkeyvalq *output_headers;
    struct evbuffer *output_buffer;

    http_req->req = evhttp_request_new(http_request_done, http_req);

    const char *path = evhttp_uri_get_path(http_req->uri);

    output_headers = evhttp_request_get_output_headers(http_req->req);

    evhttp_add_header(output_headers, "Host", evhttp_uri_get_host(http_req->uri));

    /** Set the post data */
    output_buffer = evhttp_request_get_output_buffer(http_req->req);
    evbuffer_add(output_buffer, http_req->post_data, http_req->post_len);


    evhttp_make_request(http_req->cn, http_req->req, EVHTTP_REQ_POST, path ? path : "/");


    return http_req;
}


static char data[1024] = {0,};
static char ts[128] = {0,};

void gen_data() {
    memset(ts, 0, 128);
    time_t t = time(NULL);
    strftime(ts, 128, "%Y-%m-%dT%H:%M:%SZ", gmtime(&t));


    memset(data, 0, 1024);
    snprintf(data, 1024, "{\"@timestamp\":\"%s\","
                     "\"in_bytes\":502,"
                     "\"status_code\":200,"
                     "\"out_bytes\":8625,"
                     "\"dst_port\":80}",
             ts);
}



int main(int argc, char *argv[]) {
    const char *url = "http://192.168.10.212:9200/cc-2017.01.17/test";

    struct event_base *base = event_base_new();

    struct http_request *http_req = calloc(1, sizeof(struct http_request));
    http_req->uri = evhttp_uri_parse(url);
    http_req->base = base;


    int port = evhttp_uri_get_port(http_req->uri);
    http_req->cn = evhttp_connection_base_new(http_req->base,
                              NULL,
                              evhttp_uri_get_host(http_req->uri),
                              (port == -1 ? 80 : port));
    evhttp_connection_set_retries(http_req->cn, -1);
    evhttp_connection_set_timeout(http_req->cn, 10);


    int i;
    for (i = 0; i < 10000; i++) {

        gen_data();

        printf("%s\n", data);
        http_req->post_data = data;
        http_req->post_len = strlen(data);
        start_http_requset(http_req);

        event_base_dispatch(base);
        sleep(1);
    }

    http_request_free(http_req);
    event_base_free(base);

    return 0;
}