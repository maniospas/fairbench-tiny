# C++ Usage

Here is a quickstart with using this package in C++.
The main data structure that needs to be passed around
is `fb::Data` that holds predictive data, ground truth
data, and a sensitive attribute dictionary
from attribute name strings to `std::vector<double>` 
holding group memberships in the range `[0,1]`.

This is passed alongside a dictionary from strings to
function pointers for performance metrics, as well as
a dictionary from strings to function pointers
for reduction mechanisms. The source code provides
a registry of all available options that you can
use.

```cpp
// examples\main.cpp
#include "fairbench/all.h"
using namespace fb;

int main() {
    Data data;
    data.sensitive["male"] = {1, 0, 1, 0};
    data.sensitive["female"] = {0, 1, 0, 1};
    data.validate();  // asserts correct value ranges

    data.predict = {1, 0, 1, 0};
    data.label = {1, 0, 1, 1};

    Report report = assessment(data, registry.metrics, registry.reductions);
    print(report);
    return 0;
}
```

```bash
> g++ examples\main.cpp -O2 -o examples\main
> .\examples\main

               max            maxdiff        differential   gini
error          0.500          0.500          1.000          0.500
fpr            0.000          0.000          0.000          0.000
fnr            0.000          0.000          0.000          0.000
```