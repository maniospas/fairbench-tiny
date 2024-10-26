#ifndef FB_REDUCE_H
#define FB_REDUCE_H
#include "types.h"

namespace fb {
namespace reduce {

    // Reduction function to find the maximum metric across sensitive groups
    REDUCTION(max,
        double maxMetric = 0.0;
        for (const auto &entry : data.sensitive) {
            const string &key = entry.first;
            const Vec &filter = entry.second;
            double result = m(data.predict, data.label, filter);
            if (result > maxMetric) 
                maxMetric = result;
        }
        return maxMetric;
    )

    // Reduction function to find the maximum difference between metric results across sensitive groups
    REDUCTION(maxdiff,
        Vec results;
        for (const auto &entry : data.sensitive) {
            const Vec &filter = entry.second;
            double result = m(data.predict, data.label, filter);
            results.push_back(result);
        }
        double maxDiff = 0.0;
        int n = results.size();
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = 0; j < n; ++j) {
                double diff = abs(results[i] - results[j]);
                if (diff > maxDiff) 
                    maxDiff = diff;
            }
        }
        return maxDiff;
    )


    // Reduction function to find the maximum difference between metric results across sensitive groups
    REDUCTION(differential,
        Vec results;
        for (const auto &entry : data.sensitive) {
            const Vec &filter = entry.second;
            double result = m(data.predict, data.label, filter);
            results.push_back(result);
        }
        double minRatio = 1.0;
        for (size_t i = 0; i < results.size(); ++i) {
            for (size_t j = 0; j < results.size(); ++j) {
                if(results[i]==results[j])
                    continue;
                double ratio = results[i]<results[j]?(results[i]/results[j]):(results[j]/results[i]);
                if (ratio < minRatio) 
                    minRatio = ratio;
            }
        }
        return 1-minRatio;
    )

    // Reduction function to compute the Gini coefficient across metric results for each sensitive group
    REDUCTION(gini,
        Vec results;
        for (const auto &entry : data.sensitive) {
            const Vec &filter = entry.second;
            double result = m(data.predict, data.label, filter);
            results.push_back(result);
        }
        double giniNumerator = 0;
        double totalSum = 0;
        int n = results.size();
        for (double value : results) 
            totalSum += value;
        if (totalSum == 0) 
            return 0;
        // Gini coefficient numerator: sum of absolute differences
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) 
                giniNumerator += abs(results[i] - results[j]);
        }
        // Gini coefficient formula
        return giniNumerator / (2 * n * totalSum);
    )


}} // fb::reduce
#endif // FB_REDUCE_H