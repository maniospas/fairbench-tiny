#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    struct Stats stats[col_count];
    memset(stats, 0, sizeof(stats));
    int values[col_count];
    unsigned long positives = 0, negatives = 0;

    // Process data
    while (fgets(line, sizeof(line), f)) {
        memset(values, 0, sizeof(values));
        col_pos = 0;
        for (int i = 0;; ++i) {
            char c = line[i];
            if (c == '\0' || c == '\n') break;
            if (c == '\r' || c == ' ' || c=='\'' || c=='"') continue;
            if (c == delimiter) {
                if (++col_pos >= MAX_COLS) break;
                continue;
            }
            if (c == '1')
                values[col_pos] = 1;
            else if (c != '0') {
                fprintf(stderr, "Only 0/1 entries allowed\n");
                return 1;
            }
        }

        int y_true = values[label_index];
        int y_pred = values[predict_index];
        for (size_t i = 0; i < col_count; ++i)
            if (values[i]) {
                stats[i].tp += (unsigned long)(y_true * y_pred);
                stats[i].tn += (unsigned long)((1 - y_true) * (1 - y_pred));
                stats[i].positives += (unsigned long)y_pred;
                stats[i].labels += (unsigned long)y_true;
                stats[i].count += (unsigned long)values[i];
            }
        positives += (unsigned long)y_true;
        negatives += (unsigned long)(1 - y_true);
    }

    unsigned long total_rows = positives + negatives;
    if (total_rows == 0)
        total_rows = 1;

    printf("\n%s%-20s%s %sacc%s     %stpr%s     %stnr%s     %spr%s\n",
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
        if (stats[i].count < min_samples)
            continue;

        double tp = (double)stats[i].tp;
        double tn = (double)stats[i].tn;
        double count = (double)stats[i].count;
        double pred_pos = (double)stats[i].positives;
        double label_pos = (double)stats[i].labels;
        double label_neg = count - label_pos;

        double acc = count ? (tp + tn) / count : 0.0;
        double tpr = label_pos ? tp / label_pos : 0.0;
        double tnr = label_neg ? tn / label_neg : 0.0;
        double pr  = pred_pos ? tp / pred_pos : 0.0;

        // Accuracy
        if (acc < acc_min) acc_min = acc;
        if (acc > acc_max) acc_max = acc;
        acc_wsum += count;
        acc_wsumv += count * acc;

        // TPR
        if (tpr < tpr_min) tpr_min = tpr;
        if (tpr > tpr_max) tpr_max = tpr;
        tpr_wsum += count;
        tpr_wsumv += count * tpr;

        // TNR
        if (tnr < tnr_min) tnr_min = tnr;
        if (tnr > tnr_max) tnr_max = tnr;
        tnr_wsum += count;
        tnr_wsumv += count * tnr;

        // Precision
        if (pr < pr_min) pr_min = pr;
        if (pr > pr_max) pr_max = pr;
        pr_wsum += count;
        pr_wsumv += count * pr;


        const char *acc_color = color_for(acc, threshold);
        const char *tpr_color = color_for(tpr, threshold);
        const char *tnr_color = color_for(tnr, threshold);
        const char *pr_color  = color_for(pr, threshold);

        printf("%-20s %s%.3f%s  %s%.3f%s  %s%.3f%s  %s%.3f%s\n",
               col_names[i],
               acc_color, acc, RESET,
               tpr_color, tpr, RESET,
               tnr_color, tnr, RESET,
               pr_color, pr, RESET);
    }

    // Compute derived fairness and weighted mean quantities
    double acc_wmean = acc_wsum ? acc_wsumv / acc_wsum : 0.0;
    double tpr_wmean = tpr_wsum ? tpr_wsumv / tpr_wsum : 0.0;
    double tnr_wmean = tnr_wsum ? tnr_wsumv / tnr_wsum : 0.0;
    double pr_wmean  = pr_wsum  ? pr_wsumv  / pr_wsum  : 0.0;

    double acc_diff_fair = (acc_min > 0.0) ? 1.0 - (acc_max / acc_min) : 0.0;
    double tpr_diff_fair = (tpr_min > 0.0) ? 1.0 - (tpr_max / tpr_min) : 0.0;
    double tnr_diff_fair = (tnr_min > 0.0) ? 1.0 - (tnr_max / tnr_min) : 0.0;
    double pr_diff_fair  = (pr_min  > 0.0) ? 1.0 - (pr_max  / pr_min)  : 0.0;

    double acc_abs_fair = 1.0 - (acc_max - acc_min);
    double tpr_abs_fair = 1.0 - (tpr_max - tpr_min);
    double tnr_abs_fair = 1.0 - (tnr_max - tnr_min);
    double pr_abs_fair  = 1.0 - (pr_max  - pr_min);

    // Print final aggregate table (strategies as rows)
    printf("\n%s%-20s%s %sacc%s     %stpr%s     %stnr%s     %spr%s\n",
           CYAN, "Summary", RESET, BOLD, RESET, BOLD, RESET, BOLD, RESET, BOLD, RESET);

    const char *acc_col, *tpr_col, *tnr_col, *pr_col;

    // --- MIN ---
    acc_col = color_for(acc_min, threshold);
    tpr_col = color_for(tpr_min, threshold);
    tnr_col = color_for(tnr_min, threshold);
    pr_col  = color_for(pr_min, threshold);
    printf("%-20s %s%.3f%s  %s%.3f%s  %s%.3f%s  %s%.3f%s\n",
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
    printf("%-20s %s%.3f%s  %s%.3f%s  %s%.3f%s  %s%.3f%s\n",
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
    printf("%-20s %s%.3f%s  %s%.3f%s  %s%.3f%s  %s%.3f%s\n",
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
    printf("%-20s %s%.3f%s  %s%.3f%s  %s%.3f%s  %s%.3f%s\n",
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
