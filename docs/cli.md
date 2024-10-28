# Command Line Tool

## Installation

The fairbench-tiny command line tool is an executable that you can
use to efficiently analyse (large) csv files given that the analysed
numeric data fit in memory. You can find Windows and Linux binaries
from the project's latest [release](https://github.com/maniospas/fairbench-tiny/releases/latest). 
Or you can build it yourself by cloning this
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

Compute a fairness report for a `csv` dataset and show it like below. 
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
--------------------------------------------------------------------------
Report on 3 sensitive attributes: women,men,nonbin
               max            maxdiff        differential   gini
error          1.000          0.667          0.667          0.242
fpr            1.000          1.000          1.000          0.444
fnr            1.000          1.000          1.000          0.333
--------------------------------------------------------------------------
```

You may optionally stipulate analysed column names like below.
Notice the difference in assessment when focusing only on certain columns.

```bash
> .\fbt report examples\test.csv --label label --predict predict --sensitive men,women
--------------------------------------------------------------------------
Report on 2 sensitive attributes: women,men
               max            maxdiff        differential   gini
error          1.000          0.667          0.667          0.250
fpr            1.000          0.500          0.500          0.167
fnr            1.000          1.000          1.000          0.500
--------------------------------------------------------------------------
```

You may further specify a threshold over which biases
are considered prohibitive. This affects the 
values considered as problematic, and triggers an exit 
code of 1 if they are encountered.


## Command line interface

A command line interface enables interactive report exploration.
This is similar to some simplifications. The interface keeps asking
for exploratory operations until `exit` is given. Once initiated
(this includes passing threshold checks) it does not terminate for
any other reason.

```bash
> .\fbt cli examples\test.csv

Available commands for the exploration of the report:
  exit             Exit the command line interface
  reset            Go back to the full report
  transpose        Transpose the current view
  view <name>      Focus on a row or column given its name
  details          Provide verbose details about each value
--------------------------------------------------------------------------
               fnr            fpr            error
gini           0.333          0.444          0.242
differential   1.000          1.000          0.667
maxdiff        1.000          1.000          0.667
max            1.000          1.000          1.000
--------------------------------------------------------------------------
> 
```
