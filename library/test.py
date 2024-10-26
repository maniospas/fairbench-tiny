import numpy as np
import fairbenchtiny

predictions = np.array([1.0, 0.0, 1.0, 1.0])
labels = np.array([1.0, 0.0, 1.0, 0.0])

result = fairbenchtiny.report(predictions, labels)
print(result)
