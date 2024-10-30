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
terminates if `exit` is given. Some example steps follow, where
the first's input (last line) focuses on the `fnr` reduction, 
and then second's input asks for details on all displayed values. 

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
> view fnr
```

```bash
--------------------------------------------------------------------------
               fnr            
gini           0.333
differential   1.000
maxdiff        1.000
max            1.000
--------------------------------------------------------------------------
> details
```

```bash
--------------------------------------------------------------------------
gini fnr             0.333
  |men                0.000000 (0 false negatives, 1 positives)
  |women              1.000000 (1 false negatives, 1 positives)
  |nonbin             1.000000 (1 false negatives, 1 positives)
differential fnr     1.000
  |men                0.000000 (0 false negatives, 1 positives)
  |women              1.000000 (1 false negatives, 1 positives)
  |nonbin             1.000000 (1 false negatives, 1 positives)
maxdiff fnr          1.000
  |men                0.000000 (0 false negatives, 1 positives)
  |women              1.000000 (1 false negatives, 1 positives)
  |nonbin             1.000000 (1 false negatives, 1 positives)
max fnr              1.000
  |men                0.000000 (0 false negatives, 1 positives)
  |women              1.000000 (1 false negatives, 1 positives)
  |nonbin             1.000000 (1 false negatives, 1 positives)
--------------------------------------------------------------------------
>
```