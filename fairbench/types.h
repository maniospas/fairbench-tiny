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
    };

    // Helper methods
    void checkSize(const Vec &predict, const Vec &label) {
        if (predict.size() != label.size()) 
            throw invalid_argument("Vectors are of different sizes.");
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
    checkSize(predict, label);\
    checkSize(predict, filter);\
    implementation}

#define REDUCTION(name, implementation) \
    double name(const fb::Data& data, fb::Metric m); \
    static bool _reg_reduce_##name = [](){ fb::registry.reductions[STRINGIFY(name)] = name; return true; }(); \
    double name(const fb::Data& data, fb::Metric m) { \
    implementation}

#endif // FB_TYPES_H
