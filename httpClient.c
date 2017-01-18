/* standard includes */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <time.h>

/* libcurl (http://curl.haxx.se/libcurl/c) */
#include <curl/curl.h>

static char *post_data = "{\"dawn_ts0\":1.483470142498e+15,\"guid\""
        ":\"4a859fff6e5c4521aab187eee1cfceb8\",\"device_id\":\"26aae27e-ffe5-5fc8-9281-f8"
        "2cf4e288ee\",\"probe\":{\"name\":\"cloudsensor\",\"hostname\":\"iZbp1gd3xwhcctm4ax2ruwZ"
        "\"},\"appname\":\"cloudsensor\",\"type\":\"http\",\"kafka\":{\"topic\":\"cloudsensor\"},\"ag"
        "gregate_count\":1,\"http\":{\"latency_sec\":0,\"in_bytes\":502,\"status_code\":200,"
        "\"out_bytes\":8625,\"dst_port\":80,\"src_ip\":2008838371,\"xff\":\"\",\"url\":\"\\/PHP"
        "\\/index.html\",\"refer\":\"\",\"l4_protocol\":\"tcp\",\"in_pkts\":1,\"http_method\":1"
        ",\"out_pkts\":6,\"user_agent\":\"Mozilla\\/5.0 (Macintosh; Intel Mac OS X 10_10_3"
        ") AppleWebKit\\/537.36 (KHTML, like Gecko) Chrome\\/43.0.2357.130 Safari\\/537.36 Jia"
        "nKongBao Monitor 1.1\",\"dst_ip\":1916214160,\"https_flag\":0,\"src_port\":43974,\""
        "latency_usec\":527491,\"host\":\"114.55.27.144\",\"url_query\":\"\"},\"probe_ts\":14"
        "83470142,\"dawn_ts1\":1.483470142498e+15,\"topic\":\"cloudsensor\"}";

size_t curl_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    return size * nmemb;
}


int main(int argc, char *argv[]) {
    CURL *curl;
    CURLcode rcode;

    char *url = "http://192.168.10.52:9200/_bulk";

    if ((curl = curl_easy_init()) == NULL) {
        fprintf(stderr, "ERROR: Failed to create curl\n");
        exit(1);
    }

    /*  provide the URL to use in the request */
    curl_easy_setopt(curl, CURLOPT_URL, url);

    /*  set callback for writing received data */
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback);

    /* enable TCP keep-alive probing */
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L);

    /* set http user-agent */
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "mafia-beta/1.0");

    /* set http header */
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);


    int i = 0;
    struct timeval start, end;
    int loop = 1;
    int len = strlen(post_data);

    const char *index = "{\"create\":{\"_index\":\"cc-test-2017.01.18\",\"_type\":\"http\"}}";
    char *p = (char *) calloc(1, 64 * 1024 * 1024);
    if (p == NULL) {
        printf("malloc failed\n");
        exit(1);
    }

    snprintf(p, 64 * 1024 * 1024, "%s\n%s\n", index, post_data1);

    printf("%s", p);

    gettimeofday(&start, NULL);
    for (i = 0; i < loop; i++) {

        /* specify data to POST to server */
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, p);
        rcode = curl_easy_perform(curl);

        if (rcode != CURLE_OK) {
            fprintf(stderr, "ERROR: Failed to request url (%s) - curl said: %s",
                    url, curl_easy_strerror(rcode));
            exit(2);
        }

    }
    gettimeofday(&end, NULL);
    long time_cost = ((end.tv_sec - start.tv_sec) * 1000000 + \
                end.tv_usec - start.tv_usec);
    printf("cost time: %ld us, %.2f pps\n", time_cost, loop / (time_cost * 1.0) * 1000000);

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    return 0;
}