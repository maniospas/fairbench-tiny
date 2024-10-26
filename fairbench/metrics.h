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
        return total == 0 ? 0 : differences / total;
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
        return negatives == 0 ? 0 : falsePositives / negatives;
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
        return positives == 0 ? 0 : falseNegatives / positives;
    )

}} // fb::metric
#endif // FB_METRICS_H