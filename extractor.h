#ifndef MAFIA_EXTRACTOR_H
#define MAFIA_EXTRACTOR_H

#define MAX_INTERESTED_PAIRS 8
#define MAX_ENRICHEE 32
#define MAX_ORIG_VAL_LEN 1024
#define MAX_ENRICHED_VALUE_LEN 4096
#define MAX_PAYLOAD_SIZE    8192

#if __GNUC__ >= 3
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

#define ENR_IGNORE 1
#define ENR_DELETE 2
#define ENR_UPDATE 3
#define ENR_ADD    4

#include <stdint.h>

struct enrichee {
    int orig_name_len;
    const char *orig_name;

    int orig_value_len;
    const char *orig_value; // 如果是string的话包括双引号

    int enriched_value_len;
    char *enriched_value;

    int use;
    int mode;
};

typedef int (*enricher)(struct enrichee *enrichee__, int mode);

struct interested_pair {
    int mode;
    int name_len;
    char *name;
    enricher enricher__;
};


extern struct enrichee enrichees[MAX_ENRICHEE];

int extract(const char *buf, const char *buf_end);
int register_enricher(const char *interested_name, int mode, enricher enricher__);


int init();
int ip_enricher(struct enrichee *enrichee__, int mode);
int ua_enricher(struct enrichee *enrichee__, int mode);
int time_enricher(struct enrichee *enrichee__, int mode);
void combine_enrichee(const char *buf, char *result, uint64_t ts0);

#endif // MAFIA_EXTRACTOR_H
