# Command Line Tool

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

Compute a fairness report for a `csv` dataset by like so:

```bash
> .\fbt report examples\test.csv
```