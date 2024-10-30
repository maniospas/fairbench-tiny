# Command line tool

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
When nothing else is specified, the dataset is 
considered to contain numeric `label` and `predict` 
columns, and only sensitive attributes in the rest of
the columns. The generated report computes various 
performance assessment strategies (rows) and methods
to reduce their computation across all attribute values
(columns). Larger assessments always indicate greater biases.
Furthermore, the worst value per reduction strategy is
colored red.

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

You may optionally stipulate the sensitive attribute columns like below.
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

You may further specify a bias threshold; biases larger than this
are considered problematic. When encountered, such values trigger an 
exit mechanism code of 1. You can use this in continuous integration
pipelines.


## Command line interface

A command line interface enables interactive report exploration.
The interface keeps asking for exploratory operations. 
After being initiated the first time
(and after passing threshold checks) the exploration process only
terminates if `exit` is given. Here is an example: 
