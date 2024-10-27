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

#ifndef FB_REPORT_H
#define FB_REPORT_H
#include "types.h"

namespace fb {

    // Assessment function to compute all combinations
    Report assessment(const Data& data, const Metrics& metrics, const Reductions& reductions) {
        Report report;
        for (const auto &metricEntry : metrics) {
            const string &metricName = metricEntry.first;
            Metric metricFunc = metricEntry.second;

            for (const auto &reductionEntry : reductions) {
                const string &reductionName = reductionEntry.first;
                Reduction reductionFunc = reductionEntry.second;

                // Compute the result of the reduction with the metric
                double result = reductionFunc(data, metricFunc);
                report[reductionName][metricName] = result;
            }
        }
        return report;
    }

    // Print function to display the 2D table results
    /*void print(const Report &results, int spacing=15) {
        cout << left << setw(spacing) << " ";
        for (const auto &metric : results.begin()->second) 
            cout << setw(spacing) << metric.first;
        cout << endl;

        for (const auto &reduction : results) {
            cout << setw(spacing) << reduction.first;
            for (const auto &metric : reduction.second) {
                cout << setw(spacing) << metric.second;
            }
            cout << endl;
        }
    }*/

    // Transposed print function to display the 2D table results
    const string RED_TEXT = "\033[1;31m";
    const string RESET_TEXT = "\033[0m";

    Report access(const Report &report, const string &key) {
        Report ret;
        auto topLevelIt = report.find(key);
        if(topLevelIt != report.end()) 
            ret[key] = topLevelIt->second;
        else {
            ret[key] = Results();
            for (const auto& [topKey, innerMap] : report) {
                auto secondLevelIt = innerMap.find(key);
                if(secondLevelIt==innerMap.end())
                    throw invalid_argument("Report entries were not found for "+key);
                ret[key][topKey] = secondLevelIt->second;
            }
        }
        
        return std::move(ret);
    }

    Report transpose(const Report &report) {
        Report transposed;
        for (const auto& [topKey, innerMap] : report) 
            for (const auto& [innerKey, value] : innerMap) 
                transposed[innerKey][topKey] = value;
        return std::move(transposed);
    }

    void print(const Report &results, int spacing = 15) {
        double maxValue = -1.0;

        // Find the maximum value in the report
        for (const auto &reduction : results) {
            for (const auto &metric : reduction.second) {
                if (metric.second > maxValue) {
                    maxValue = metric.second;
                }
            }
        }

        cout << left << setw(spacing) << " ";
        for (const auto &reduction : results)
            cout << setw(spacing) << reduction.first;
        cout << endl;

        if (!results.empty()) {
            for (const auto &metric : results.begin()->second) {
                cout << setw(spacing) << metric.first;
                for (const auto &reduction : results) {
                    double value = reduction.second.at(metric.first);
                    cout << fixed << setprecision(3);  // Set formatting to .3f

                    // Print the value in red if it's the maximum
                    if (value == maxValue) {
                        cout << RED_TEXT << setw(spacing) << value << RESET_TEXT;
                    } else {
                        cout << setw(spacing) << value;
                    }
                }
                cout << endl;
            }
        }
    }

} // fb
#endif // FB_REPORT_H