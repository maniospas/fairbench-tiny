/**
    Copyright 2024 Emmanouil Krasanakis

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#ifndef FB_METRICS_H
#define FB_METRICS_H
#include "types.h"

namespace fb {
namespace metric {
    
    // Error function
    METRIC(error, 
        int n = predict.size();
        double differences = 0;
        double total = 0;
        for (int i = 0; i < n; ++i) {
            if (predict[i] != label[i]) 
                differences += filter[i];
            total += filter[i];
        }
        return Explainable(
            total == 0 ? 0 : differences / total, 
            str(differences, "errors, ") + str(total, "samples")
        );
    )

    // False Positive Rate
    METRIC(fpr,
        int n = predict.size();
        double falsePositives = 0;
        double negatives = 0;
        for (int i = 0; i < n; ++i) {
            if (label[i] == 0) {  // Only consider actual negatives
                if (predict[i] != label[i]) 
                    falsePositives += filter[i];
                negatives += filter[i];
            }
        }
        return Explainable(
            negatives == 0 ? 0 : falsePositives / negatives, 
            str(falsePositives, "false positives, ") + str(negatives, "negatives")
        );
    )

    // False Negative Rate
    METRIC(fnr,
        int n = predict.size();
        double falseNegatives = 0;
        double positives = 0;
        for (int i = 0; i < n; ++i) {
            if (label[i] == 1) {  // Only consider actual positives
                if (predict[i] != label[i]) 
                    falseNegatives += filter[i];
                positives += filter[i];
            }
        }
        return Explainable(
            positives == 0 ? 0 : falseNegatives / positives, 
            str(falseNegatives, "false negatives, ") + str(positives, "positives")
        );
    )

}} // fb::metric
#endif // FB_METRICS_H