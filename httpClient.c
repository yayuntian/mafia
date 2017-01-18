/* standard includes */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <time.h>

/* libcurl (http://curl.haxx.se/libcurl/c) */
#include <curl/curl.h>

static char *post_data = "{\n"
        "    \"dawn_ts0\": 1483468791574000,\n"
        "    \"guid\": \"31\",\n"
        "    \"device_id\": \"79bf7e53-d92f-5cdd-a7c3-3e9e97685c2c\",\n"
        "    \"probe\": {\n"
        "        \"name\": \"cloudsensor\"\n"
        "    },\n"
        "    \"appname\": \"cloudsensor\",\n"
        "    \"type\": \"tcp\",\n"
        "    \"kafka\": {\n"
        "        \"topic\": \"cloudsensor\"\n"
        "    },\n"
        "    \"aggregate_count\": 1,\n"
        "    \"tcp\": {\n"
        "        \"src_isp\": 0,\n"
        "        \"l4_proto\": 6,\n"
        "        \"out_bytes\": 0,\n"
        "        \"dst_port\": 3306,\n"
        "        \"retransmitted_out_fin_pkts\": 0,\n"
        "        \"client_latency_sec\": 0,\n"
        "        \"window_zero_size\": 0,\n"
        "        \"src_ipv4\": 178257969,\n"
        "        \"topic\": \"tcp\",\n"
        "        \"in_pkts\": 0,\n"
        "        \"src_region\": 0,\n"
        "        \"retransmitted_out_payload_pkts\": 0,\n"
        "        \"dst_ipv4\": 178808905,\n"
        "        \"ts\": 1483468791,\n"
        "        \"final_status\": 3,\n"
        "        \"retransmitted_in_syn_pkts\": 0,\n"
        "        \"src_ip\": 178257969,\n"
        "        \"server_latency_sec\": 0,\n"
        "        \"l4_protocol\": 6,\n"
        "        \"bytes_in\": 0,\n"
        "        \"src_port\": 57366,\n"
        "        \"retransmitted_out_syn_pkts\": 0,\n"
        "        \"retransmitted_in_ack_pkts\": 0,\n"
        "        \"out_pkts\": 0,\n"
        "        \"device_id\": \"79bf7e53-d92f-5cdd-a7c3-3e9e97685c2c\",\n"
        "        \"guid\": \"31\",\n"
        "        \"bytes_out\": 0,\n"
        "        \"retransmitted_in_payload_pkts\": 0,\n"
        "        \"dst_ip\": 178808905,\n"
        "        \"in_bytes\": 0,\n"
        "        \"retransmitted_out_ack_pkts\": 0,\n"
        "        \"server_latency_usec\": 3314,\n"
        "        \"client_latency_usec\": 3740006,\n"
        "        \"retransmitted_in_fin_pkts\": 0\n"
        "    },\n"
        "    \"probe_ts\": 1483468791,\n"
        "    \"dawn_ts1\": 1483468791574000,\n"
        "    \"topic\": \"cloudsensor\"\n"
        "}";



//int i = 0;
//struct timeval start, end;
//gettimeofday(&start, NULL);
//for (i = 0; i < 10000; i++) {
//
//}
//gettimeofday(&end, NULL);
//long time_cost = ((end.tv_sec - start.tv_sec) * 1000000 + \
//            end.tv_usec - start.tv_usec);
//printf("cost time: %ld us, %.2f pps\n", time_cost, 10000 / (time_cost * 1.0) * 1000000);


size_t curl_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    return size * nmemb;
}


int main(int argc, char *argv[]) {
    CURL *curl;
    CURLcode rcode;

    char *url = "http://192.168.10.212:9200/cc-2017.01.18/test";

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


    /* specify data to POST to server */
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
    rcode = curl_easy_perform(curl);

    if (rcode != CURLE_OK) {
        fprintf(stderr, "ERROR: Failed to request url (%s) - curl said: %s",
                url, curl_easy_strerror(rcode));
        exit(2);
    }

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    return 0;
}