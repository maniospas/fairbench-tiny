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