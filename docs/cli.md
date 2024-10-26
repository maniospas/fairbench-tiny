# Command Line Tool

## Installation

The fairbench-tiny command line tool is an executable that you can
use to efficiently analyse (large) csv files given that the analysed
numeric data fit in memory. You can find a version from the platform
from the main repository page, or build it yourself by cloning this
repository and running the following (there are no other dependencies).

```bash
> g++ fbt.cpp -O2 -o fbt
```

Use the executable command line tool (e.g., `fbt.exe`)
to get a help reminde of hot it is used by opening a console 
and running it with no arguments:

```bash
> .\fbt
```

## Fairness report

Compute a fairness report for a `csv` dataset like below.
When nothing else is specified, the dataset is by default 
considered to contain numeric `label` and `predict` 
columns, and only sensitive attributes in the rest of
the columns. The generated report contains various 
performance assessment strategies (rows) and methods
to reduce their computation across all attribute values
(columns). Larger assessments indicate greater biases
of certain types.

```bash
> .\fbt report examples\test.csv
Report on 3 sensitive attributes: women,men,nonbin
               max            maxdiff        differential   gini
error          1.000          0.667          0.667          0.242
fpr            1.000          1.000          1.000          0.444
fnr            1.000          1.000          1.000          0.333
```

You may optionally stipulate analysed column names like below.
Notice the difference in assessment when focusing only on certain columns.

```bash
> .\fbt report examples\test.csv --label label --predict predict --sensitive men,women
Report on 2 sensitive attributes: women,men
               max            maxdiff        differential   gini
error          1.000          0.667          0.667          0.250
fpr            1.000          0.500          0.500          0.167
fnr            1.000          1.000          1.000          0.500
```