#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "data.h"
#include <time.h>


static const char* RESET  = "\033[0m";
static const char* RED    = "\033[31m";
static const char* GREEN  = "\033[32m";


#ifdef _WIN32
  #include <windows.h>
  #include <io.h>        // for _fileno
  #define fileno _fileno // MSVC name difference
#else
  #include <unistd.h>
  #include <poll.h>
#endif
static void poll_for_data(FILE *f) {
#ifdef _WIN32
    // Windows version — no POSIX poll, just sleep briefly.
    // stdin and pipes usually block automatically anyway.
    Sleep(100); // milliseconds
#else
    // Unix-like version — efficiently waits until input becomes available.
    int fd = fileno(f);
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN; // wait for readable data
    int ret = poll(&pfd, 1, 100); // timeout: 100 ms

    if (ret < 0) {
        // poll failed (e.g. EINTR); fallback to a short sleep
        usleep(100000);
    }
#endif
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file.csv [--label colname] [--predict colname] [--threshold value]\n", argv[0]);
        return 2;
    }

    const char *filepath = NULL;
    const char *label_col = NULL;
    const char *predict_col = NULL;
    double threshold = 0.0;
    size_t min_samples = 0;
    MHASH_INDEX_UINT categorical_dimensions = 10;

    struct Config configs[MAX_COLS];
    int current_config = -1;

    double stream_interval = 0;
    double forget = 0;

    // Parse CLI args
    int in_comments = 0;
    for (int i = 1; i < argc; ++i) {
        if(argv[i][0] == 0)
            continue;
        if(in_comments) {
            if(argv[i][0]=='-') 
                in_comments = 0;
            else 
                continue;
        }
        if(argv[i][0]=='#') 
            in_comments = 1;
        else if (strcmp(argv[i], "--label") == 0 && i + 1 < argc)
            label_col = argv[++i];
        else if (strcmp(argv[i], "--predict") == 0 && i + 1 < argc)
            predict_col = argv[++i];
        else if (strcmp(argv[i], "--threshold") == 0 && i + 1 < argc)
            threshold = atof(argv[++i]);
        else if (strcmp(argv[i], "--members") == 0 && i + 1 < argc)
            min_samples = (unsigned long)atol(argv[++i]);
        else if (strcmp(argv[i], "--numbers") == 0 && i + 1 < argc)
            categorical_dimensions = (unsigned long)atol(argv[++i]);
        else if (strcmp(argv[i], "--stream") == 0 && i + 1 < argc) 
            stream_interval = (double)atof(argv[++i]);
        else if (strcmp(argv[i], "--forget") == 0 && i + 1 < argc) 
            forget = (double)atof(argv[++i]);
        else if (argv[i][0] != '-')
            filepath = argv[i];
    }

    // --- .fb argument file support ---
    if (filepath && strlen(filepath) > 3 && strcmp(filepath + strlen(filepath) - 3, ".fb") == 0) {
        FILE *fb = fopen(filepath, "r");
        if (!fb) {
            fprintf(stderr, "Error: could not open argument file '%s'\n", filepath);
            return 2;
        }
        filepath = NULL;

        char line[MAX_LINE_SIZE];
        int in_comments_fb = 0;

        while (fgets(line, sizeof(line), fb)) {
            // strip inline comments
            char *hash = strchr(line, '#');
            if (hash) *hash = '\0';
            // trim leading/trailing spaces
            char *start = line;
            while (isspace(*start)) start++;
            char *end = start + strlen(start);
            while (end > start && isspace(*(end - 1))) *--end = '\0';
            if (*start == '\0')
                continue;
            char *arg = strtok(start, " \t\r\n");
            while (arg) {
                if (arg[0] == 0) {
                    arg = strtok(NULL, " \t\r\n");
                    continue;
                }
                if (in_comments_fb) {
                    if (arg[0] == '-')
                        in_comments_fb = 0;
                    else {
                        arg = strtok(NULL, " \t\r\n");
                        continue;
                    }
                }
                // start of comment
                if (arg[0] == '#') {
                    in_comments_fb = 1;
                    arg = strtok(NULL, " \t\r\n");
                    continue;
                }

                // --- handle @column sections ---
                if (arg[0] == '@') {
                    current_config++;
                    if (current_config >= MAX_COLS) {
                        fprintf(stderr, "Error: too many column configs (> %d)\n", MAX_COLS);
                        return 2;
                    }
                    memset(&configs[current_config], 0, sizeof(struct Config));
                    configs[current_config].name = xstrdup(arg + 1);
                    configs[current_config].status = CONFIG_STATUS_AUTO;
                    configs[current_config].threshold = threshold;
                    arg = strtok(NULL, " \t\r\n");
                    continue;
                }
                if(!strcmp(arg, "--label")) {
                    if (current_config != -1) {
                        fprintf(stderr, "Error: can only set --label before setting a @column\n");
                        return 2;
                    }
                    char *next = strtok(NULL, " \t\r\n");
                    if (next)
                        label_col = xstrdup(next);
                } 
                else if(!strcmp(arg, "--predict")) {
                    if (current_config != -1) {
                        fprintf(stderr, "Error: can only set --predict before setting a @column\n");
                        return 2;
                    }
                    char *next = strtok(NULL, " \t\r\n");
                    if (next)
                        predict_col = xstrdup(next);
                } 
                else if(!strcmp(arg, "--threshold")) {
                    char *next = strtok(NULL, " \t\r\n");
                    if (next) {
                        double val = atof(next);
                        if (current_config == -1)
                            threshold = val;
                        else
                            configs[current_config].threshold = val;
                    }
                } 
                else if (strcmp(arg, "--stream") == 0) {
                    char *next = strtok(NULL, " \t\r\n");
                    if (current_config != -1) {
                        fprintf(stderr, "Error: can only set --stream before setting a @column\n");
                        return 2;
                    }
                    if (next)
                        stream_interval = (double)atof(next);
                }
                else if (strcmp(arg, "--forget") == 0) {
                    char *next = strtok(NULL, " \t\r\n");
                    if (current_config != -1) {
                        fprintf(stderr, "Error: can only set --forget before setting a @column\n");
                        return 2;
                    }
                    if (next)
                        forget = (double)atof(next);
                }
                else if(!strcmp(arg, "--numbers")) {
                    char *next = strtok(NULL, " \t\r\n");
                    if (current_config != -1) {
                        fprintf(stderr, "Error: can only set --numbers before setting a @column\n");
                        return 2;
                    }
                    if(next) 
                        categorical_dimensions = (unsigned long)atol(next);
                } 
                else if(!strcmp(arg, "--numeric")) {
                    if (current_config == -1) {
                        fprintf(stderr, "Error: can only declare --numeric after setting a @column\n");
                        return 2;
                    }
                    configs[current_config].status = CONFIG_STATUS_NUMERIC;
                }
                else if(!strcmp(arg, "--binary")) {
                    char *next = strtok(NULL, " \t\r\n");
                    if (current_config == -1) {
                        fprintf(stderr, "Error: can only set --binary after setting a @column\n");
                        return 2;
                    }
                    if (next) {
                        configs[current_config].status = CONFIG_STATUS_BINARY;
                        configs[current_config].binary = xstrdup(next);
                    }
                } 
                else if(!strcmp(arg, "--skip")) {
                    if (current_config == -1) {
                        fprintf(stderr, "Error: can only --skip after setting a @column\n");
                        return 2;
                    }
                    configs[current_config].status = CONFIG_STATUS_SKIP;
                } 
                else if(!strcmp(arg, "--members")) {
                    if (current_config != -1) {
                        fprintf(stderr, "Error: can only set --numbers before setting a @column\n");
                        return 2;
                    }
                    char *next = strtok(NULL, " \t\r\n");
                    if(next) min_samples = (unsigned long)atol(next);
                } 
                else if(arg[0] != '-' && arg[0] != 0) {
                    if (current_config != -1) {
                        fprintf(stderr, "Error: can only set a file path before setting a @column\n");
                        return 2;
                    }
                    filepath = xstrdup(arg);
                }

                arg = strtok(NULL, " \t\r\n");
            }
        }
        fclose(fb);
        if (!filepath && !stream_interval) {
            fprintf(stderr, "Error: no data file or --stream specified in .fb script\n");
            return 2;
        }
    }

    if (!filepath && !stream_interval) {
        fprintf(stderr, "Error: no input file or --stream specification provided.\n");
        return 2;
    }

    FILE *f = NULL;
    if(filepath) {
        f = fopen(filepath, "r");
        if (!f) {
            fprintf(stderr, "Error opening file: %s\n", filepath);
            return 2;
        }
    }
    else {
        f = stdin;
        if (!f) {
            fprintf(stderr, "Error getting stdin\n");
            return 2;
        }
    }

    // info
    char line[MAX_LINE_SIZE];
    char col_names[MAX_COLS][MAX_STR_LEN];
    size_t col_count = 0, col_pos = 0;
    MHASH_INDEX_UINT label_index = MHASH_EMPTY_SLOT;
    MHASH_INDEX_UINT predict_index = MHASH_EMPTY_SLOT;
    
    if(!filepath) {
        printf("\033[2J\033[H\n\n%s----- Live report -----%s\n", GREEN,RESET);
        printf("FairBench-tiny is running in --stream\n");
        if(!filepath)    
            printf("%sCurrently waiting on stdin%s because no data file was provided\n", RED,RESET);
        printf("\nWaiting for first header line...\n");
    }

    // Parse header
    if (!fgets(line, sizeof(line), f)) {
        fprintf(stderr, "Empty header line\n");
        return 2;
    }

    char delimiter = 0;
    for (int i = 0;; ++i) {
        char c = line[i];
        if (c == '\0') {
            fprintf(stderr, "Header line too large\n");
            return 2;
        }
        if (c == '\r' || c == ' ' || c=='\'' || c=='"') continue;
        if (c == '\n') break;
        if (is_delimiter(c)) {
            if (delimiter && delimiter != c) {
                fprintf(stderr, "Header has multiple delimiters\n");
                return 2;
            }
            delimiter = c;
            col_names[col_count][col_pos] = 0;
            col_count++;
            col_pos = 0;
            if(col_count>=MAX_COLS) {
                fprintf(stderr, "Too many columns in header\n");
                return 2;
            }
        } 
        else 
            col_names[col_count][col_pos++] = c;
        
    }
    col_names[col_count][col_pos] = 0;
    col_count++;
    printf("Detected %zu columns\n", col_count);

    // Init header mhash
    size_t map_table_size = col_count*col_count + col_count*2 + 1;
    MHASH_INDEX_UINT map_table[map_table_size];
    const char *col_ptrs[MAX_COLS];
    for (size_t i = 0; i < col_count; ++i)
        col_ptrs[i] = col_names[i];
    MHash map;
    if (mhash_init(&map, map_table, map_table_size, (const void **)col_ptrs, col_count, mhash_str_prefix)) {
        fprintf(stderr, "Error: too many columns in header\n");
        return 2;
    }

    // Resolve label/predict columns
    label_index = mhash_entry(&map, label_col ? label_col : "label");
    predict_index = mhash_entry(&map, predict_col ? predict_col : "predict");
    if (label_index == MHASH_EMPTY_SLOT) {
        fprintf(stderr, "Error: could not find label column\n");
        return 2;
    }
    if (predict_index == MHASH_EMPTY_SLOT) {
        fprintf(stderr, "Error: could not find predict column\n");
        return 2;
    }

    // column info (most of it will be useful later but preallocated anyway
    struct Column columns[MAX_COLS];
    unsigned long total_rows = 0;
    size_t col_start=0, col_end=0;
    double values[MAX_COLS];
    memset(columns, 0, sizeof(columns));

    // attach configs to columns
    for (int i = 0; i <= current_config; ++i) {
        MHASH_UINT pos = mhash_entry_pos(&map, configs[i].name);
        MHASH_INDEX_UINT idx = map.table[pos];
        if(idx == MHASH_EMPTY_SLOT) {
            fprintf(stderr, "Error: configured column '%s' not found in header\n", configs[i].name);
            return 2;
        }
        if(strcmp(col_ptrs[idx], configs[i].name)) {
            fprintf(stderr, "Error: configured column '%s' not found in header\n", configs[i].name);
            return 2;
        }
        if(columns[idx].config) {
            fprintf(stderr, "Error: configured column '%s' multiple times\n", configs[i].name);
            return 2;
        }
        columns[idx].config = &configs[i];
    }

    // Process data
    time_t start_time = time(NULL);
    if(stream_interval<0) stream_interval = 0;
    time_t last_report_print = start_time-(long int)stream_interval-1;
    while (1) {
        if (!fgets(line, sizeof(line), f)) {
            if(filepath) break;  // normal batch exit
            time_t now = time(NULL);
            if(difftime(now, last_report_print)>=stream_interval) {
                last_report_print = now;
                printf("\033[2J\033[H\n\n%s----- Live report (%.0f sec) -----%s\n", GREEN, difftime(now, start_time), RESET);
                printf("FairBench-tiny is running in --stream mode\n");
                if(!filepath)    
                    printf("%sCurrently waiting on stdin%s because no data file was provided\n", RED,RESET);
                if(!total_rows)
                    printf("\nWaiting for first data line...\n");
                else
                    print_report(
                        columns,
                        col_ptrs,
                        col_count, 
                        predict_index, 
                        label_index,
                        min_samples,
                        total_rows,
                        threshold
                    );
            }
            clearerr(f);          // EOF reached, wait for more
            poll_for_data(f);     // e.g. select(), poll(), or sleep()
            continue;
        }

        col_pos = 0;
        total_rows++;
        col_start = 0;
        col_end = 0;
        memset(values, 0, sizeof(values));
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
                    return 2;
                }
                if(col_end-col_start>=MAX_STR_LEN-1) {
                    fprintf(stderr, "Error: column value too large\n");
                    return 2;
                }
                line[col_end] = '\0'; // we will never go back
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
                        return 2;
                    }
                    //printf("Initialized column %s with first dimension %s\n", col_names[current_col], columns[current_col].dimension_names[0]);
                }
                // each column also has some explicit boolean value
                int has_been_processed = 0;
                int is_number = isdigit(line[col_start]) || line[col_start]=='-' || line[col_start]=='+';

                if(columns[current_col].config && columns[current_col].config->status) {
                    has_been_processed = 1;
                    if(columns[current_col].config->status==CONFIG_STATUS_NUMERIC) 
                        values[current_col] = atof(&line[col_start]);
                    else if(columns[current_col].config->status==CONFIG_STATUS_BINARY)
                         values[current_col] = strcmp(&line[col_start], columns[current_col].config->binary)?0:1;
                }
                
                if(!has_been_processed && (line[col_start]=='y' || line[col_start]=='Y' || line[col_start]=='1')) 
                    values[current_col] = 1.0;
                if(has_been_processed) {
                }
                else if(is_number && columns[current_col].num_dimensions >= categorical_dimensions) {
                    columns[current_col].active_dim = 0; // numeric: single global bucket
                } 
                else {
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
                        size_t close_dim = (columns[current_col].num_dimensions/4)*4+4; // reduce the number of reallocs by /4
                        size_t sz = close_dim * close_dim + close_dim * 2 + 1;
                        MHASH_INDEX_UINT *new_table = columns[current_col].map.table;
                        if(sz!=columns[current_col].map.table_size)
                            new_table = realloc(columns[current_col].map.table, sizeof(MHASH_INDEX_UINT) * sz);
                        if (mhash_init(&columns[current_col].map,
                                    new_table,
                                    sz,
                                    (const void**)columns[current_col].dimension_names,
                                    columns[current_col].num_dimensions,
                                    mhash_str_prefix)) {
                            fprintf(stderr, "Error: too many categorical values during column %s\n", col_names[current_col]);
                            return 2;
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

        double y_true = values[label_index];
        double y_pred = values[predict_index];
        if(forget) {
            for (size_t i = 0; i < col_count; ++i) {
                struct Stats *st = &columns[i].stats[columns[i].active_dim];
                st->tp = st->tp*(1-forget) + forget * y_true * y_pred;
                st->tn = st->tn*(1-forget) + forget * (1.0 - y_true) * (1.0 - y_pred);
                st->positives = (1-forget)*st->positives + forget*y_pred;
                st->labels = st->labels*(1-forget) + forget*y_true;
                st->count = (1-forget)*st->count+forget;
            }
        }
        else {
            for (size_t i = 0; i < col_count; ++i) {
                struct Stats *st = &columns[i].stats[columns[i].active_dim];
                st->tp += y_true * y_pred;
                st->tn += (1.0 - y_true) * (1.0 - y_pred);
                st->positives += y_pred;
                st->labels += y_true;
                st->count += 1.0;
            }
        }

        if (stream_interval) {
            time_t now = time(NULL);
            if(difftime(now, last_report_print)>=stream_interval) {
                last_report_print = now;
                printf("\033[2J\033[H\n\n%s----- Live report (%.0f sec) -----%s\n", GREEN, difftime(now, start_time), RESET);
                printf("FairBench-tiny is running in --stream mode\n");
                if(!filepath)    
                    printf("%sCurrently waiting on stdin%s because no data file was provided\n", RED,RESET);
                if(!total_rows)
                    printf("\nWaiting for first data line...\n");
                else
                    print_report(
                        columns,
                        col_ptrs,
                        col_count, 
                        predict_index, 
                        label_index,
                        min_samples,
                        total_rows,
                        threshold
                    );
            }
        }
    }
    fclose(f);
    if (total_rows == 0) {
        fprintf(stderr, "No data rows found (but headers were read)\n");
        return 2;
    }

    return print_report(
        columns,
        col_ptrs,
        col_count, 
        predict_index, 
        label_index,
        min_samples,
        total_rows,
        threshold
    );
}
