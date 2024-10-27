# Python Interface

After installing the fairbenchtiny library in your (virtual) environment,
you can import it from its name. This grants access to a report generation
method that takes as inputs numpy arrays of doubles or data types that pybind can convert
to numpy arrays, such as lists. Here is an example:

```python
# main.py
import fairbenchtiny as fbt

predictions = [1, 0, 1, 1]
labels = [1, 0, 1, 0]
sensitive = {
    "male": [1, 0, 1, 0], 
    "female": [0, 1, 0, 1]
}

report = fbt.report(predictions, labels, sensitive)
```

This report is a dictionary of reduction strategies,
which are dictionaries of metric asssessment values,
which in turn comprise a dictionary of a `"value`" number
and an `"explain"` string. Here is an example of
accessing the contents of this dictionary that continues
from above:

```python
for reduction, values in report.items():
    print(reduction)
    for metric, value in values.items():
        print(" ", metric, value["value"], value["explain"])
```

```bash
.\python main.py

max
  error 0.5
  |male               0.000000 (0 errors, 2 samples)
  |female             0.500000 (1 errors, 2 samples)
  fpr 0.5
  |male               0.000000 (0 false positives, 0 negatives)
  |female             0.500000 (1 false positives, 2 negatives)
  fnr 0.0
  |male               0.000000 (0 false negatives, 2 positives)
  |female             0.000000 (0 false negatives, 0 positives)
maxdiff
  error 0.5
  |male               0.000000 (0 errors, 2 samples)
  |female             0.500000 (1 errors, 2 samples)
  fpr 0.5
  |male               0.000000 (0 false positives, 0 negatives)
  |female             0.500000 (1 false positives, 2 negatives)
  fnr 0.0
  |male               0.000000 (0 false negatives, 2 positives)
  |female             0.000000 (0 false negatives, 0 positives)
differential
  error 1.0
  |male               0.000000 (0 errors, 2 samples)
  |female             0.500000 (1 errors, 2 samples)
  fpr 1.0
  |male               0.000000 (0 false positives, 0 negatives)
  |female             0.500000 (1 false positives, 2 negatives)
  fnr 0.0
  |male               0.000000 (0 false negatives, 2 positives)
  |female             0.000000 (0 false negatives, 0 positives)
gini
  error 0.5
  |male               0.000000 (0 errors, 2 samples)
  |female             0.500000 (1 errors, 2 samples)
  fpr 0.5
  |male               0.000000 (0 false positives, 0 negatives)
  |female             0.500000 (1 false positives, 2 negatives)
  fnr 0.0
  |male               0.000000 (0 false negatives, 2 positives)
  |female             0.000000 (0 false negatives, 0 positives)
```