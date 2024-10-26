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

#ifndef FB_TYPES_H
#define FB_TYPES_H

#include <iostream>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <string>
#include <iomanip>
using namespace std;

namespace fb {

    // actual data types
    class Data;
    typedef vector<double> Vec;
    typedef unordered_map<string, Vec> Sensitive;
    typedef double (*Metric)(const Vec&, const Vec&, const Vec&);
    typedef unordered_map<string, Metric> Metrics;
    typedef double (*Reduction)(const Data& data, Metric m);
    typedef unordered_map<string, Reduction> Reductions;
    typedef unordered_map<string, double> Results;
    typedef unordered_map<string, unordered_map<string, double>> Report;

    struct Data {
        Vec predict;
        Vec label;
        Sensitive sensitive;
        Data() {}
        Data(const Vec &predict, const Vec &label, const Sensitive &sensitive) 
            : predict(predict), label(label), sensitive(sensitive) {}
            
        void validate() {
            // check sensitive attributes
            for (const auto &entry : sensitive) {
                const Vec &filter = entry.second;
                int n = filter.size();
                for(int i=0; i<n; ++i) {
                    double value = filter[i];
                    if(value<0 || value>1) 
                        throw invalid_argument("Value "
                                               + std::to_string(value)
                                               + " at position "
                                               + std::to_string(i)
                                               + " of sensitive attribte \""
                                               + entry.first
                                               + "\" is not in the range [0,1].");
                }
            }
            // check predictions
            {
                int n = predict.size();
                for(int i=0; i<n; ++i) {
                    double value = predict[i];
                    if(value<0 || value>1) 
                        throw invalid_argument("Value "
                                               + std::to_string(value)
                                               + " at position "
                                               + std::to_string(i)
                                               + " of predict is not in the range [0,1].");
                }
            }
            // check labels
            {
                int n = label.size();
                for(int i=0; i<n; ++i) {
                    double value = label[i];
                    if(value<0 || value>1) 
                        throw invalid_argument("Value "
                                               + std::to_string(value)
                                               + " at position "
                                               + std::to_string(i)
                                               + " of label is not in the range [0,1].");
                }
            }
        }
    };

    // Helper methods
    void checkSize(const Vec &predict, const Vec &filter) {
        if (predict.size() != filter.size()) 
            throw invalid_argument("Vector size "
                                    + std::to_string(predict.size())
                                    + " is different than the sensitive attribute size "
                                    + std::to_string(filter.size())
                                    + ".");
    }

    // Register all declared symbols
    class Registry {
    public:
        Metrics metrics;
        Reductions reductions;
    };
    Registry registry;

} // namespace fb


// Create the METRIC and REDUCE macros to create and register respective callables
#define STRINGIFY(s) #s

#define METRIC(name, implementation) \
    double name(const fb::Vec &predict, const fb::Vec &label, const fb::Vec &filter); \
    static bool _reg_metric_##name = [](){ fb::registry.metrics[STRINGIFY(name)] = name; return true; }(); \
    double name(const fb::Vec &predict, const fb::Vec &label, const fb::Vec &filter) { \
    checkSize(predict, filter);\
    checkSize(label, filter);\
    implementation}

#define REDUCTION(name, implementation) \
    double name(const fb::Data& data, fb::Metric m); \
    static bool _reg_reduce_##name = [](){ fb::registry.reductions[STRINGIFY(name)] = name; return true; }(); \
    double name(const fb::Data& data, fb::Metric m) { \
    implementation}

#endif // FB_TYPES_H
