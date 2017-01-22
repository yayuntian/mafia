#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <getopt.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>

#include "extractor.h"
#include "wrapper.h"
#include "http.h"
#include "kafkaConsumer.h"
#include "utils.h"


FILE *fp = NULL;
static uint64_t rx_count = 0;

static void init_bulk() {
    bulk.data = calloc(1, MAX_BULK_SIZE);
    if (!bulk.data) {
        fprintf(stderr, "calloc memory failed\n");
        exit(1);
    }

    bulk.time = 1;
    bulk.batch = 10000;
}


size_t write_data_log(const char *data, size_t length) {
    char *filename = kconf.out_file;

    if (!fp) {
        if ((fp = fopen(filename, "wb")) == NULL) {
            log_err("open file error: %s\n", filename);
            return 0;
        }
    }
    size_t ret = fwrite(data, sizeof(char), length, fp);

    return ret;
}


#ifdef PERF
void echo_perf() {
    long time_cost = ((kconf.end.tv_sec - kconf.start.tv_sec) * 1000000 + \
            kconf.end.tv_usec - kconf.start.tv_usec);

    fprintf(stderr, "# const time: %ld, rx cnt: %ld, byt: %ld\n",
            time_cost, kconf.msg_cnt, kconf.rx_byt);

    fprintf(stderr, "# %.2f pps, %.2f MBps\n",
            kconf.msg_cnt / (time_cost * 1.0) * 1000000,
            kconf.rx_byt / (time_cost * 1.0 * 1024 * 1024) * 1000000);
}
#endif


void gen_index(const char *topic) {

    char index[128] = {0,};

    snprintf(index, 128, "cc-%s-%s-", topic, remove_quotes(bulk.guid));
    time_t t = time(NULL);
    strftime(index + strlen(index), 128, "%Y.%m.%d", gmtime(&t));

    int ret = snprintf(bulk.data + bulk.offset, MAX_PAYLOAD_SIZE,
             "{\"create\":{\"_index\":\"%s\",\"_type\":\"%s\"}\n", index, remove_quotes(bulk.type));

    bulk.offset += ret;
}



void payload_callback(rd_kafka_message_t *rkmessage) {
    const char *buf = (char *)rkmessage->payload;
    const int buf_len = (int)rkmessage->len;

#ifdef PERF
    kconf.rx_byt += (int)rkmessage->len;
    if (rx_count == 0) {
        gettimeofday(&kconf.start, NULL);
    }
#endif
    if (++rx_count == kconf.msg_cnt) {
#ifdef PERF
        gettimeofday(&kconf.end, NULL);
        echo_perf();
#endif
        kconf.run = 0;
    }

    long ts0 = time_current_usec();
    // parser key-value, process
    extract(buf, buf + buf_len);

    spinlock(&bulk.lock);

    bulk.count++;
    if (bulk.offset + MAX_PAYLOAD_SIZE > MAX_BULK_SIZE) {
        bulk.full = 1;
    }

    // gen es bulk ops
    if (kconf.filename != NULL) {
        gen_index("readfile");
    } else {
        gen_index(rd_kafka_topic_name(rkmessage->rkt));
    }

    bulk.offset += snprintf(bulk.data + bulk.offset,
                            MAX_PAYLOAD_SIZE, "{\"mafia_ts0\":%ld,", ts0);

    bulk.offset += combine_enrichee(buf, bulk.data + bulk.offset);

    // process "a":b,}
    if (bulk.data[bulk.offset - 1] == '}' && bulk.data[bulk.offset - 2] == ',') {
        bulk.offset -= 2;
    } else {
        bulk.offset -= 1;
    }

    long ts1 = time_current_usec();
    bulk.offset += snprintf(bulk.data + bulk.offset, MAX_PAYLOAD_SIZE,
             ",\"mafia_ts1\":%ld, \"mafia_latency_usec\":%ld}\n",
             ts1, ts1 - ts0);

    bulk.data[bulk.offset] = '\0';

    spinunlock(&bulk.lock);

    log(KLOG_DEBUG, "%s\n", bulk.data);
}


void read_file(void)
{
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(kconf.filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Open file error\n");
        exit(1);
    }

    while ((read = getline(&line, &len, fp)) != -1) {
        line[read - 1] = '\0';
//        printf("[%zu]%s", read, line);

        rd_kafka_message_t msg_t;
        msg_t.payload = (void *) line;
        msg_t.len = read;
        payload_callback(&msg_t);

        usleep(10 * 1000);

        if (kconf.run == 0) {
            break;
        }
    }

    fclose(fp);
    if (line) {
        free(line);
    }

    return;
}


struct kafkaConf kconf = {
    .run = 1,
    .msg_cnt = -1,
    .filename = NULL,
    .out_file = NULL,
    .verbosity = KLOG_INFO,
    .partition = RD_KAFKA_PARTITION_UA,
    .brokers = "localhost:9092",
    .group = "rdkafka_consumer_mafia",
    .topic_count = 0,
    .es_url = "http://localhost:9200/_bulk",
#ifdef PERF
    .rx_byt = 0,
#endif
    .payload_cb = payload_callback,
    .offset = RD_KAFKA_OFFSET_STORED
};


static void usage(const char *argv0) {

    printf("mafia - ClearClouds Message Tool\n"
    "Copyright (c) 2011-2017 WuXi Juyun System, Ltd.\n"
            "Version: 0.1beta\n"
            "\n");

    printf("Usage: %s <options> [topic1 topic2 ...]\n", argv0);

    printf("General options:\n"
            " -g <group>      Consumer group (%s)\n"
            " -b <brokes>     Broker address (%s)\n"
            " -u <url>        Elasticsearch url (%s)\n"
            " -f <file>       Consumer from json file\n"
            " -w <out-file>   Write to disk file\n"
            "                 1 - not regiest json cb, do parser, copy, write disk\n"
            "                 2 - not parser json, do copy, write disk\n"
            "                 3 - not parser json, do copy, write /dev/null\n"
            " -o <offset>     Offset to start consuming from:\n"
            "                 beginning[-2] | end[-1] | stored[-1000]\n"
            " -c <cnt>        Exit after consumering this number (-1)\n"
            " -q              Be quiet\n"
            " -e              Exit consumer when last message\n"
            " -d              Debug mode\n"
            " -h              Show help\n",
    kconf.group, kconf.brokers, kconf.es_url);

    exit(1);
}


/**
 * Parse command line arguments
 */
static void argparse (int argc, char **argv) {
    int i, opt;

    while ((opt = getopt(argc, argv, "g:b:s:w:o:u:c:f:qdh")) != -1) {
        switch (opt) {
            case 'b':
                kconf.brokers = optarg;
                break;
            case 'g':
                kconf.group = optarg;
                break;
            case 'f':
                kconf.filename = optarg;
                break;
            case 'u':
                kconf.es_url = optarg;
                break;
            case 'w':
                kconf.out_file = optarg;
                break;
            case 'c':
                kconf.msg_cnt = strtoll(optarg, NULL, 10);
                break;
            case 'q':
                kconf.verbosity = KLOG_ERR;
                break;
            case 'd':
                kconf.verbosity = KLOG_DEBUG;
                break;
            case 'o':
                if (!strcmp(optarg, "end"))
                    kconf.offset = RD_KAFKA_OFFSET_END;
                else if (!strcmp(optarg, "beginning"))
                    kconf.offset = RD_KAFKA_OFFSET_BEGINNING;
                else if (!strcmp(optarg, "stored"))
                    kconf.offset = RD_KAFKA_OFFSET_STORED;
                else {
                    kconf.offset = strtoll(optarg, NULL, 10);
                    if (kconf.offset < 0)
                        kconf.offset = RD_KAFKA_OFFSET_TAIL(-kconf.offset);
                }
                break;
            default:
                usage(argv[0]);
                break;
        }
    }

    if (kconf.filename != NULL) {
        fprintf(stderr, "%% Read message from file: %s\n", kconf.filename);
        return;
    }

    kconf.topic_count = argc - optind;
    for (i = 0; i < kconf.topic_count; i++) {
        kconf.topic[i] = argv[optind + i];
    }

    if (!kconf.brokers || !kconf.group || !kconf.topic_count) {
        usage(argv[0]);
    }
}



void bulk_post(void *arg) {
    int cur_time;
    bulk.last_time = time(NULL);
    int send = 0;
    int wait_count = 0;

    printf("post thread start time: %d\n", bulk.last_time);

    httpclient_t *client = http_init(kconf.es_url);

    while (1) {
        cur_time = time(NULL);
        if (cur_time > bulk.last_time + bulk.time) {
            send = 1;
            bulk.last_time = cur_time;
        }

        if (bulk.count && (send || bulk.count > bulk.batch || bulk.full)) {

            printf("post send: count: %d, time: %d, full: %d\n",
                   bulk.count, send, bulk.full);
            spinlock(&bulk.lock);

            if (kconf.out_file) {
                write_data_log(bulk.data, bulk.offset);
            } else {
                http_post(client, bulk.data, bulk.offset);
            }

            // reset bulk buffer
            bulk.offset = 0;
            bulk.full = 0;
            bulk.count = 0;
            send = 0;

            spinunlock(&bulk.lock);
        } else {
            wait_count++;
        }

        if (wait_count > 100) {
            sleep(1);
        }
    }
}



int main(int argc, char **argv) {
    argparse(argc, argv);

    init();
    ipwrapper_init();
    init_bulk();

    pthread_t post_thread;
    if(pthread_create(&post_thread, NULL, (void *)bulk_post, NULL)) {
        fprintf(stderr, "Error creating post thread\n");
        exit(1);
    }


    // update
    register_enricher("src_ip", ENR_UPDATE, ip_enricher);
    register_enricher("dst_ip", ENR_UPDATE, ip_enricher);
    register_enricher("user_agent", ENR_UPDATE, ua_enricher);
    // add new item
    register_enricher("ts", ENR_ADD, time_enricher);

    // read value
    register_enricher("type", ENR_GET, type_enricher);
    register_enricher("guid", ENR_GET, guid_enricher);


    if (kconf.filename != NULL) {
        read_file();
        // send post
        bulk.batch = 0;
        sleep(5);
        exit(0);
    }

    init_kafka_consumer();

    return 0;
}
