# fairBench-tiny

This is a high-performance C++ utility for evaluating basic fairness metrics from large tabular datasets.  
It provides a lightweight alternative to the [**FairBench**](https://github.com/mever-team/FairBench) Python framework
that is ideal for environments where speed and memory efficiency are critical.

## ✨ Features

- Fast parsing of large CSV/TSV files with bare metal code.  
- Aggregate group-wise metrics for classification (tpr, tnr, accuracy, pr).
- Color-coded output and exit codes for failing analysis. 
- Minimal energy footprint in a few kB of memory - set up as a worker.

## ⚡ Quickstart

Manual build: clone this repository and run `make`. This will
create the executable `build/fbt`. Use the *fbt* executable like so:

```bash
./fbt data.csv [--label colname] [--predict colname] [--threshold value] [--members min_count]
```

- data.csv` Path to the CSV/TSV data file to analyze. |
- --label &lt;colname> Name of the column containing true labels (default: *label*).
- --predict &lt;colname> Name of the column containing predicted labels (default: *predict*).
- --threshold &lt;value> Highlight values below this fairness threshold in red, and above 1-threshold in green (default: 0.0).
- --members &lt;min_count> Minimum number of samples required for a group to be included in the report. Groups with fewer members are ignored.

You can also pass as argument a configuration *.fb* file like the ones in the examples. 
This can contain line comments starting with `#` after setting the dataset file in the first line.
Run those files like so (you can still provide arguments externally, but these are overwritten by
file-specific options):

```bash
./fbt examples/test.fb
```


## 📘 Expected Input

The first line must contain column headers (group names, *label*, and *predict*). Columns may be separated by **comma `,`**, **tab `\t`**, or **semicolon `;`** — the first delimiter encountered is used. **Whitespace** is ignored everywhere.  

All rows must have the same number of columns as the header, and must contain **binary (0/1)** values for every column. 
Here is an example:

```csv
gender,region,label,predict
1,0,1,1
0,0,0,0
1,1,1,0
0,1,0,1
```