#ifndef DATA_H
#define DATA_H

#include <stdlib.h>
#include "mhash/mhash.h"
#include "mhash/mhash_str.h"

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

#define MAX_COLS 64
#define MAX_STR_LEN 128
#define MAX_LINE_SIZE 4096

#define CONFIG_STATUS_AUTO 0       // follows global defaults
#define CONFIG_STATUS_SKIP 1       // skips the column
#define CONFIG_STATUS_NUMERIC 2    // custom number of categories (uses Config.categories)
#define CONFIG_STATUS_BINARY 3     // custom number of categories (uses Config.binary)

struct Config {
    char* name;
    int status;
    double threshold;
    union {
        int categories;
        char* binary;
    };
};

struct Stats {
    double tp;
    double tn;
    double positives;
    double labels;
    double count;
};

struct Column {
    MHash map;
    size_t num_dimensions;
    MHASH_INDEX_UINT active_dim;
    char** dimension_names;
    struct Stats *stats;
    struct Config *config;
};

void print_report(
    const struct Column *columns,
    const char **col_names,
    size_t col_count, 
    MHASH_INDEX_UINT predict_index, 
    MHASH_INDEX_UINT label_index,
    size_t min_samples,
    size_t total_rows,
    double threshold
);


static inline char *xstrdup(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);
    if (!p) {
        fprintf(stderr, "Error: out of memory copying string\n");
        exit(1);
    }
    memcpy(p, s, n);
    return p;
} 

static inline char is_delimiter(char c) {
    return c == ',' || c == '\t' || c == ';';
}

#endif // DATA_H