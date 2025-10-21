#include "data.h"

static const char* RESET  = "\033[0m";
static const char* RED    = "\033[31m";
static const char* GREEN  = "\033[32m";
static const char* CYAN   = "\033[36m";
static const char* BOLD   = "\033[1m";
#define CHECK_COLS if(acc_col==RED || tpr_col==RED || tnr_col==RED || pr_col==RED) return_code=1;

static const char *color_for(double v, double threshold) {
    if (v < threshold)
        return RED;
    if (v > 1-threshold)
        return GREEN;
    return RESET;
}

int print_report(
    const struct Column *columns,
    const char **col_names,
    size_t col_count, 
    MHASH_INDEX_UINT predict_index, 
    MHASH_INDEX_UINT label_index,
    size_t min_samples,
    size_t total_rows,
    double threshold
) {
    int return_code = 0;
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
        const struct Column *col = &columns[i];
        if (col->num_dimensions == 0 || !col->stats)
            continue;
        for (size_t d = 0; d < col->num_dimensions; ++d) {
            struct Stats *st = &col->stats[d];
            if (st->count < min_samples)
                continue;
            if(col->config && col->config->status==CONFIG_STATUS_SKIP)
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
    CHECK_COLS
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
    CHECK_COLS
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
    CHECK_COLS
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
    CHECK_COLS
    printf("%-30s %s%.3f%s  %s%.3f%s  %s%.3f%s  %s%.3f%s\n",
           "absolutely fair",
           acc_col, acc_abs_fair, RESET,
           tpr_col, tpr_abs_fair, RESET,
           tnr_col, tnr_abs_fair, RESET,
           pr_col, pr_abs_fair, RESET);


    printf("\nSamples: %lu\n", total_rows);
    printf("Threshold: %.2f\n", threshold);
    return return_code;
}