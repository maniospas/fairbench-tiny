# fairbench-tiny

A performant C++ implementation of several fairness assessment functionalities that are 
originally offerred by the [fairbench](https://github.com/mever-team/FairBench) framework.
These are made type-safe and are exposed through the following equivalent interfaceas:

- C++ library
- Python library
- Command-line tool for csv data evaluation

*This repository's code is still under development and not mature enough to use in production.*

**License:** Apache 2.0


## Sneak peek

Generate reports like the one below and get details on which computations
took place to determine offending values.

```plaintext
--------------------------------------------------------------------------
Report on 3 sensitive attributes: women,men,nonbin
               max            maxdiff        differential   gini
error          1.000          0.667          0.667          0.242
fpr            1.000          1.000          1.000          0.444
fnr            1.000          1.000          1.000          0.333
--------------------------------------------------------------------------
```

## Quickstart

[C++](docs/cpp.md) <br>
[Python](docs/python.md) <br>
[Command line](docs/cli.md) <br>
