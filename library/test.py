import fairbenchtiny as fbt

predictions = [1, 0, 1, 1]
labels = [1, 0, 1, 0]
sensitive = {
    "male": [1, 0, 1, 0], 
    "female": [0, 1, 0, 1]
}

result = fbt.report(predictions, labels, sensitive)
for reduction, values in result.items():
    print(reduction)
    for metric, value in values.items():
        print(" ", metric, value["value"], value["explain"])