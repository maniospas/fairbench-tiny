import numpy as np
import fairbenchtiny as fbt

predictions = [1, 0, 1, 1]
labels = [1, 0, 1, 0]
sensitive = {
    "male": [1, 0, 1, 0], 
    "female": [0, 1, 0, 1]
}

result = fbt.report(predictions, labels, sensitive)
print(result)
