#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MHASH_MAX_HASHES 5

#include "mhash/mhash.h"
#include "mhash/mhash_str.h"


#define MAX_COLS 32
#define MAX_STR_LEN 128
#define MAX_LINE_SIZE 4096

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

#include <ctype.h>
#include <stdlib.h>

static inline size_t minimum(size_t a, size_t b) {
    return (a < b) ? a : b;
}

static char is_delimiter(char c) {
    return c == ',' || c == '\t' || c == ';';
}

struct Stats {
    unsigned long tp;
    unsigned long tn;
    unsigned long positives;
    unsigned long labels;
    unsigned long count;
};

struct Column {
    size_t num_dimensions;
    MHash map;
    char** dimension_names;
    struct Stats *stats;
    MHASH_INDEX_UINT active_dim;
};

static const char *color_for(double v, double threshold) {
    if (v < threshold)
        return RED;
    if (v > 1-threshold)
        return GREEN;
    return RESET;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file.csv [--label colname] [--predict colname] [--threshold value]\n", argv[0]);
        return 1;
    }

    const char *filepath = NULL;
    const char *label_col = NULL;
    const char *predict_col = NULL;
    double threshold = 0.0;
    size_t min_samples = 0;

    // Parse CLI args
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--label") == 0 && i + 1 < argc)
            label_col = argv[++i];
        else if (strcmp(argv[i], "--predict") == 0 && i + 1 < argc)
            predict_col = argv[++i];
        else if (strcmp(argv[i], "--threshold") == 0 && i + 1 < argc)
            threshold = atof(argv[++i]);
        else if (strcmp(argv[i], "--members") == 0 && i + 1 < argc)
            min_samples = (unsigned long)atol(argv[++i]);
        else if (argv[i][0] != '-')
            filepath = argv[i];
    }

    if (!filepath) {
        fprintf(stderr, "Error: no input file provided.\n");
        return 1;
    }

    FILE *f = fopen(filepath, "r");
    if (!f) {
        fprintf(stderr, "Error opening file\n");
        return 1;
    }

    char line[MAX_LINE_SIZE];
    char col_names[MAX_COLS][MAX_STR_LEN];
    size_t col_count = 0, col_pos = 0;
    MHASH_INDEX_UINT label_index = MHASH_EMPTY_SLOT;
    MHASH_INDEX_UINT predict_index = MHASH_EMPTY_SLOT;

    // Parse header
    if (!fgets(line, sizeof(line), f)) {
        fprintf(stderr, "Empty header line\n");
        return 1;
    }

    char delimiter = 0;
    for (int i = 0;; ++i) {
        char c = line[i];
        if (c == '\0') {
            fprintf(stderr, "Header line too large\n");
            return 1;
        }
        if (c == '\r' || c == ' ' || c=='\'' || c=='"') continue;
        if (c == '\n') break;
        if (is_delimiter(c)) {
            if (delimiter && delimiter != c) {
                fprintf(stderr, "Header has multiple delimiters\n");
                return 1;
            }
            delimiter = c;
            col_names[col_count][col_pos] = 0;
            col_count++;
            col_pos = 0;
            if(col_count>=MAX_COLS) {
                fprintf(stderr, "Too many columns in header\n");
                return 1;
            }
        } 
        else 
            col_names[col_count][col_pos++] = c;
        
    }
    col_names[col_count][col_pos] = 0;
    col_count++;
    printf("Detected %zu columns\n", col_count);

    // Init mhash
    size_t map_table_size = col_count*col_count + col_count*2 + 1;
    MHASH_INDEX_UINT map_table[map_table_size];
    const char *col_ptrs[MAX_COLS];
    for (size_t i = 0; i < col_count; ++i)
        col_ptrs[i] = col_names[i];
    MHash map;
    if (mhash_init(&map, map_table, map_table_size, (const void **)col_ptrs, col_count, mhash_str_prefix)) {
        fprintf(stderr, "Error: too many columns in header\n");
        return 1;
    }

    // Resolve label/predict columns
    label_index = mhash_entry(&map, label_col ? label_col : "label");
    predict_index = mhash_entry(&map, predict_col ? predict_col : "predict");
    if (label_index == MHASH_EMPTY_SLOT) {
        fprintf(stderr, "Error: could not find label column\n");
        return 1;
    }
    if (predict_index == MHASH_EMPTY_SLOT) {
        fprintf(stderr, "Error: could not find predict column\n");
        return 1;
    }

    // Stats
    struct Column columns[MAX_COLS];
    unsigned long total_rows = 0;
    size_t col_start=0, col_end=0;
    int values[col_count];
    memset(columns, 0, sizeof(columns));

    // Process data
    while (fgets(line, sizeof(line), f)) {
        col_pos = 0;
        total_rows++;
        col_start = 0;
        col_end = 0;
        memset(values, 0, sizeof(int)*col_count);
        for (size_t i = 0;; ++i) {
            char c = line[i];
            if (c == '\r' || c == ' ' || c=='\'' || c=='"') {
                if(col_start>=col_end)
                    col_start = i+1;
                continue;
            }
            if (c == delimiter || c == '\0' || c == '\n') {
                size_t current_col = col_pos;
                if (++col_pos >= MAX_COLS) 
                    break;
                // process column
                if (col_end <= col_start) {
                    fprintf(stderr, "Error: empty_column\n");
                    return 1;
                }
                if(col_end-col_start>=MAX_STR_LEN-1) {
                    fprintf(stderr, "Error: column value too large\n");
                    return 1;
                }
                // each column also have some explicit boolean value
                if (line[col_start]=='y' || line[col_start]=='1') 
                    values[current_col] = 1;
                // initialize mhash for the column
                if(columns[current_col].num_dimensions==0) {
                    size_t table_size = 1;
                    columns[current_col].dimension_names = malloc(sizeof(char**));
                    columns[current_col].stats = malloc(sizeof(struct Stats));
                    memset(columns[current_col].stats, 0, sizeof(struct Stats));
                    columns[current_col].num_dimensions = 1;
                    columns[current_col].dimension_names[0] = malloc(col_end - col_start + 1);
                    memcpy(columns[current_col].dimension_names[0], &line[col_start], col_end - col_start);
                    columns[current_col].dimension_names[0][col_end - col_start] = '\0';
                    if (mhash_init(&columns[current_col].map, 
                        malloc(sizeof(MHASH_INDEX_UINT)*table_size), 
                        table_size, 
                        (const void**) columns[current_col].dimension_names, 
                        1, 
                        mhash_str_prefix
                    )) {
                        fprintf(stderr, "Error: too many categorical values during column %s\n", col_names[current_col]);
                        return 1;
                    }
                    //printf("Initialized column %s with first dimension %s\n", col_names[current_col], columns[current_col].dimension_names[0]);
                }
                int is_number = isdigit(line[col_start]) || line[col_start]=='-' || line[col_start]=='+';
                if (is_number) {
                    columns[current_col].active_dim = 0; // numeric: single global bucket
                } 
                else {
                    line[col_end] = '\0'; // we will never go back
                    MHASH_UINT pos = mhash_entry_pos(&columns[current_col].map, &line[col_start]);
                    MHASH_INDEX_UINT dim_idx = columns[current_col].map.table[pos];
                    int match = 0;
                    if (dim_idx != MHASH_EMPTY_SLOT) {
                        char *dim_name = columns[current_col].dimension_names[dim_idx];
                        size_t dim_len = strlen(dim_name);
                        if (dim_len == (size_t)(col_end - col_start) && memcmp(dim_name, &line[col_start], dim_len) == 0)
                            match = 1;
                    }
                    if (!match) {
                        size_t old = columns[current_col].num_dimensions++;
                        columns[current_col].dimension_names = realloc(columns[current_col].dimension_names, sizeof(char*) * columns[current_col].num_dimensions);
                        columns[current_col].dimension_names[old] = malloc(col_end - col_start + 1);
                        memcpy(columns[current_col].dimension_names[old], &line[col_start], col_end - col_start);
                        columns[current_col].dimension_names[old][col_end-col_start] = '\0';
                        columns[current_col].stats = realloc(columns[current_col].stats, sizeof(struct Stats) * columns[current_col].num_dimensions);
                        memset(&columns[current_col].stats[old], 0, sizeof(struct Stats));
                        size_t sz = columns[current_col].num_dimensions * columns[current_col].num_dimensions + columns[current_col].num_dimensions * 2 + 1;
                        if (sz > 128 * columns[current_col].num_dimensions)
                            sz = 128 * columns[current_col].num_dimensions;
                        MHASH_INDEX_UINT *new_table = malloc(sizeof(MHASH_INDEX_UINT) * sz);
                        if (mhash_init(&columns[current_col].map,
                                    new_table,
                                    sz,
                                    (const void**)columns[current_col].dimension_names,
                                    columns[current_col].num_dimensions,
                                    mhash_str_prefix)) {
                            fprintf(stderr, "Error: too many categorical values during column %s\n", col_names[current_col]);
                            return 1;
                        }
                        columns[current_col].active_dim = old;
                    }
                    else 
                        columns[current_col].active_dim = dim_idx;
                }
                col_start = i+1;
                if (c == '\0' || c == '\n') 
                    break;
                continue;
            }
            col_end = i+1;
        }

        int y_true = values[label_index];
        int y_pred = values[predict_index];
        for (size_t i = 0; i < col_count; ++i) {
            struct Stats *st = &columns[i].stats[columns[i].active_dim];
            st->tp += (unsigned long)(y_true && y_pred);
            st->tn += (unsigned long)(!y_true && !y_pred);
            st->positives += (unsigned long)y_pred;
            st->labels += (unsigned long)y_true;
            st->count++;
        }
    }

    if (total_rows == 0) {
        fprintf(stderr, "No data rows found (but headers were read)\n");
        return 1;
    }

    printf("\n%s%-30s%s %sacc%s     %stpr%s     %stnr%s     %spr%s\n",
           CYAN, "Groups", RESET, BOLD, RESET, BOLD, RESET, BOLD, RESET, BOLD, RESET);

    // Initialize aggregate accumulators
    double acc_min = 1.0, acc_max = 0.0, acc_wsum = 0.0, acc_wsumv = 0.0;
    double tpr_min = 1.0, tpr_max = 0.0, tpr_wsum = 0.0, tpr_wsumv = 0.0;
    double tnr_min = 1.0, tnr_max = 0.0, tnr_wsum = 0.0, tnr_wsumv = 0.0;
    double pr_min  = 1.0, pr_max  = 0.0, pr_wsum  = 0.0, pr_wsumv  = 0.0;

    // Re-run over valid columns to accumulate aggregates
    for (size_t i = 0; i < col_count; ++i) {
        if (i == label_index || i == predict_index)
            continue;
        struct Column *col = &columns[i];
        if (col->num_dimensions == 0 || !col->stats)
            continue;
        for (size_t d = 0; d < col->num_dimensions; ++d) {
            struct Stats *st = &col->stats[d];
            if (st->count < min_samples)
                continue;
            double tp = (double)st->tp;
            double tn = (double)st->tn;
            double count = (double)st->count;
            double pred_pos = (double)st->positives;
            double label_pos = (double)st->labels;
            double label_neg = count - label_pos;
            double acc = count ? (tp + tn) / count : 0.0;
            double tpr = label_pos ? tp / label_pos : 0.0;
            double tnr = label_neg ? tn / label_neg : 0.0;
            double pr  = pred_pos ? tp / pred_pos : 0.0;
            if (acc < acc_min) acc_min = acc;
            if (acc > acc_max) acc_max = acc;
            acc_wsum += count;
            acc_wsumv += count * acc;
            if (tpr < tpr_min) tpr_min = tpr;
            if (tpr > tpr_max) tpr_max = tpr;
            tpr_wsum += count;
            tpr_wsumv += count * tpr;
            if (tnr < tnr_min) tnr_min = tnr;
            if (tnr > tnr_max) tnr_max = tnr;
            tnr_wsum += count;
            tnr_wsumv += count * tnr;
            if (pr < pr_min) pr_min = pr;
            if (pr > pr_max) pr_max = pr;
            pr_wsum += count;
            pr_wsumv += count * pr;
            const char *acc_color = color_for(acc, threshold);
            const char *tpr_color = color_for(tpr, threshold);
            const char *tnr_color = color_for(tnr, threshold);
            const char *pr_color  = color_for(pr, threshold);
            const char *dim_name = col->num_dimensions==1?"[number]":col->dimension_names[d];
            printf("%-15s%-15s %s%.3f%s  %s%.3f%s  %s%.3f%s  %s%.3f%s\n",
                   col_names[i], dim_name,
                   acc_color, acc, RESET,
                   tpr_color, tpr, RESET,
                   tnr_color, tnr, RESET,
                   pr_color, pr, RESET);
        }
    }



    // Compute derived fairness and weighted mean quantities
    double acc_wmean = acc_wsum ? acc_wsumv / acc_wsum : 0.0;
    double tpr_wmean = tpr_wsum ? tpr_wsumv / tpr_wsum : 0.0;
    double tnr_wmean = tnr_wsum ? tnr_wsumv / tnr_wsum : 0.0;
    double pr_wmean  = pr_wsum  ? pr_wsumv  / pr_wsum  : 0.0;

    double acc_diff_fair = (acc_min > 0.0) ? (acc_min / acc_max) : 0.0;
    double tpr_diff_fair = (tpr_min > 0.0) ? (tpr_min / tpr_max) : 0.0;
    double tnr_diff_fair = (tnr_min > 0.0) ? (tnr_min / tnr_max) : 0.0;
    double pr_diff_fair  = (pr_min  > 0.0) ? (pr_min  / pr_max)  : 0.0;

    double acc_abs_fair = 1.0 - (acc_max - acc_min);
    double tpr_abs_fair = 1.0 - (tpr_max - tpr_min);
    double tnr_abs_fair = 1.0 - (tnr_max - tnr_min);
    double pr_abs_fair  = 1.0 - (pr_max  - pr_min);

    // Print final aggregate table (strategies as rows)
    printf("\n%s%-30s%s %sacc%s     %stpr%s     %stnr%s     %spr%s\n",
           CYAN, "Summary", RESET, BOLD, RESET, BOLD, RESET, BOLD, RESET, BOLD, RESET);

    const char *acc_col, *tpr_col, *tnr_col, *pr_col;

    // --- MIN ---
    acc_col = color_for(acc_min, threshold);
    tpr_col = color_for(tpr_min, threshold);
    tnr_col = color_for(tnr_min, threshold);
    pr_col  = color_for(pr_min, threshold);
    printf("%-30s %s%.3f%s  %s%.3f%s  %s%.3f%s  %s%.3f%s\n",
           "min",
           acc_col, acc_min, RESET,
           tpr_col, tpr_min, RESET,
           tnr_col, tnr_min, RESET,
           pr_col, pr_min, RESET);

    // --- WEIGHTED MEAN ---
    acc_col = color_for(acc_wmean, threshold);
    tpr_col = color_for(tpr_wmean, threshold);
    tnr_col = color_for(tnr_wmean, threshold);
    pr_col  = color_for(pr_wmean, threshold);
    printf("%-30s %s%.3f%s  %s%.3f%s  %s%.3f%s  %s%.3f%s\n",
           "weighted mean",
           acc_col, acc_wmean, RESET,
           tpr_col, tpr_wmean, RESET,
           tnr_col, tnr_wmean, RESET,
           pr_col, pr_wmean, RESET);

    // --- DIFFERENTIALLY FAIR ---
    acc_col = color_for(acc_diff_fair, threshold);
    tpr_col = color_for(tpr_diff_fair, threshold);
    tnr_col = color_for(tnr_diff_fair, threshold);
    pr_col  = color_for(pr_diff_fair, threshold);
    printf("%-30s %s%.3f%s  %s%.3f%s  %s%.3f%s  %s%.3f%s\n",
           "differentially fair",
           acc_col, acc_diff_fair, RESET,
           tpr_col, tpr_diff_fair, RESET,
           tnr_col, tnr_diff_fair, RESET,
           pr_col, pr_diff_fair, RESET);

    // --- ABSOLUTELY FAIR ---
    acc_col = color_for(acc_abs_fair, threshold);
    tpr_col = color_for(tpr_abs_fair, threshold);
    tnr_col = color_for(tnr_abs_fair, threshold);
    pr_col  = color_for(pr_abs_fair, threshold);
    printf("%-30s %s%.3f%s  %s%.3f%s  %s%.3f%s  %s%.3f%s\n",
           "absolutely fair",
           acc_col, acc_abs_fair, RESET,
           tpr_col, tpr_abs_fair, RESET,
           tnr_col, tnr_abs_fair, RESET,
           pr_col, pr_abs_fair, RESET);


    printf("\nSamples: %lu\n", total_rows);
    printf("Threshold: %.2f\n", threshold);

    fclose(f);
    return 0;

}
