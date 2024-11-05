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
                Explainable result = reductionFunc(data, metricFunc);
                report[reductionName][metricName] = std::move(result);
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
    const string CYAN_TEXT = "\033[1;36m";
    const string YELLOW_TEXT = "\033[1;33m";
    const string MAGENTA_TEXT = "\033[1;35m";
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

    void details(const Report &results, int spacing = 20) {
        for (const auto &metric : results.begin()->second) 
            for (const auto &reduction : results) {
                Explainable value = reduction.second.at(metric.first);
                cout << fixed << setprecision(3);
                cout << CYAN_TEXT<< left << setw(spacing) << (metric.first+" "+reduction.first) << RESET_TEXT << " " << value.get() << "   " << value.str() << "\n";
            }
    }

    void print(const Report &results, int spacing = 15) {
        double maxValue = -1.0;

        // Find the maximum value in the report
        for (const auto &reduction : results) {
            for (const auto &metric : reduction.second) {
                double value = metric.second.get();
                if (value > maxValue) {
                    maxValue = value;
                }
            }
        }

        cout << CYAN_TEXT << left << setw(spacing) << " ";
        for (const auto &reduction : results)
            cout << setw(spacing) << reduction.first;
        cout << RESET_TEXT << endl;

        if (!results.empty()) {
            for (const auto &metric : results.begin()->second) {
                cout << CYAN_TEXT << setw(spacing) << metric.first << RESET_TEXT;
                for (const auto &reduction : results) {
                    double value = reduction.second.at(metric.first).get();
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