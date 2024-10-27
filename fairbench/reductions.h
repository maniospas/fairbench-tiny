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

#ifndef FB_REDUCE_H
#define FB_REDUCE_H
#include "types.h"
#include <string>
#include <unordered_map>

namespace fb {
namespace reduce {

    // Reduction function to find the maximum metric across sensitive groups
    REDUCTION(max,
        double maxMetric = 0.0;
        Dict desc;
        for (const auto &entry : data.sensitive) {
            const string &key = entry.first;
            const Vec &filter = entry.second;
            Explainable result = m(data.predict, data.label, filter);
            if (result.get() > maxMetric) 
                maxMetric = result.get();
            desc[key] = result;
        }
        return Explainable(maxMetric, desc);
    )

    // Reduction function to find the maximum difference between metric results across sensitive groups
    REDUCTION(maxdiff,
        Vec results;
        Dict desc;
        for (const auto &entry : data.sensitive) {
            const Vec &filter = entry.second;
            Explainable result = m(data.predict, data.label, filter);
            results.push_back(result.get());
            desc[entry.first] = result;
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
        return Explainable(maxDiff, desc);
    )


    // Reduction function to find the maximum difference between metric results across sensitive groups
    REDUCTION(differential,
        Vec results;
        Dict desc;
        for (const auto &entry : data.sensitive) {
            const Vec &filter = entry.second;
            Explainable result = m(data.predict, data.label, filter);
            results.push_back(result.get());
            desc[entry.first] = result;
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
        return Explainable(1-minRatio, desc);
    )

    // Reduction function to compute the Gini coefficient across metric results for each sensitive group
    REDUCTION(gini,
        Vec results;
        Dict desc;
        for (const auto &entry : data.sensitive) {
            const Vec &filter = entry.second;
            Explainable result = m(data.predict, data.label, filter);
            results.push_back(result.get());
            desc[entry.first] = result;
        }
        double giniNumerator = 0;
        double totalSum = 0;
        int n = results.size();
        for (double value : results) 
            totalSum += value;
        if (totalSum == 0) 
            return Explainable(0, desc);
        // Gini coefficient numerator: sum of absolute differences
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) 
                giniNumerator += abs(results[i] - results[j]);
        }
        // Gini coefficient formula
        return Explainable(giniNumerator / (2 * n * totalSum), desc);
    )


}} // fb::reduce
#endif // FB_REDUCE_H