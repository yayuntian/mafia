//
// Created by tyy on 2017/1/13.
//
#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "wrapper.h"
#include "extractor.h"

struct enrichee enrichees[MAX_ENRICHEE];


int init() {
    for (int i = 0; i < MAX_ENRICHEE; i++) {
        enrichees[i].enriched_value = (char *) malloc(MAX_ENRICHED_VALUE_LEN);
    }

    return 0;
}


int ip_enricher(struct enrichee *enrichee__, int mode) {
    char value[MAX_ORIG_VAL_LEN] = {0,};

    strncpy(value, enrichee__->orig_value, enrichee__->orig_value_len);
    char *output = ip2JsonStr(value);

    if (!output) {
        enrichee__->orig_value_len = 0;
        enrichee__->enriched_value_len = 0;
        return 0;
    }

    int len = strlen(output);
    memset(enrichee__->enriched_value, 0, MAX_ENRICHED_VALUE_LEN);
    strncpy(enrichee__->enriched_value, output, len);
    enrichee__->enriched_value_len = len;

    return 0;
}

int ua_enricher(struct enrichee *enrichee__, int mode) {
    char value[MAX_ORIG_VAL_LEN] = {0,};

    // remove "
    strncpy(value, enrichee__->orig_value + 1, enrichee__->orig_value_len - 2);
    char *output = ua2JsonStr(value, enrichee__->orig_value_len - 2);

    if (!output) {
        enrichee__->orig_value_len = 0;
        enrichee__->enriched_value_len = 0;
        return 0;
    }

    int len = strlen(output);
    memset(enrichee__->enriched_value, 0, MAX_ENRICHED_VALUE_LEN);
    strncpy(enrichee__->enriched_value, output, len);
    enrichee__->enriched_value_len = len;
    return 0;
}


int time_enricher(struct enrichee *enrichee__, int mode) {
    int offset = 0;
    char *value = "\"@timestamp\":\"";

    memset(enrichee__->enriched_value, 0, MAX_ENRICHED_VALUE_LEN);
    strncpy(enrichee__->enriched_value, value, strlen(value));

    offset += strlen(value);

    time_t t = atoi(enrichee__->orig_value);
    t = (t == 0) ? time(NULL) : t;
    strftime(enrichee__->enriched_value + offset, MAX_ENRICHED_VALUE_LEN - offset,
             "%Y-%m-%dT%H:%M:%SZ", gmtime(&t));

    strcat(enrichee__->enriched_value, "\",");
    enrichee__->enriched_value_len = strlen(enrichee__->enriched_value);
    return 0;
}


void combine_enrichee(const char *buf, char *result, uint64_t ts0) {
    int i;
    int offset_buf = 0;
    int offset_result = 0;

    const char *next_clean_ptr;
    int next_clean_len;

    snprintf(result, MAX_PAYLOAD_SIZE, "{\"mafia_ts0\":%ld,", ts0);

    // scan and add item to json header
    int flag_add = 0;
    offset_result += strlen(result);
    for (i = 0; i < MAX_ENRICHEE; i++) {
        if (enrichees[i].use == 1 && enrichees[i].mode == ENR_ADD) {
            strncpy(result + offset_result, enrichees[i].enriched_value,
            enrichees[i].enriched_value_len);

            offset_result += enrichees[i].enriched_value_len;
            enrichees[i].use = 0;
            flag_add = 1;
        }
    }

    offset_buf = flag_add ? 1 : 0;

    // update or delete item
    for (i = 0; i < MAX_ENRICHEE; i++) {
        if (enrichees[i].use == 0) {
            continue;
        }
        enrichees[i].use = 0;

        // copy before i clean buf
        next_clean_ptr = buf + offset_buf;
        next_clean_len = enrichees[i].orig_value - (buf + offset_buf);

        strncpy(result + offset_result, next_clean_ptr, next_clean_len);

        offset_buf += next_clean_len + enrichees[i].orig_value_len;
        offset_result += next_clean_len;

        // copy i fix
        if (enrichees[i].enriched_value_len) {
            strncpy(result + offset_result, enrichees[i].enriched_value, enrichees[i].enriched_value_len);
            offset_result += enrichees[i].enriched_value_len;
        }
    }

    next_clean_ptr = buf + offset_buf;
    if (offset_buf < strlen(buf)) {
        next_clean_len = strlen(buf) - offset_buf;
        strncpy(result + offset_result, next_clean_ptr, next_clean_len);
        offset_result += next_clean_len;
    }
}