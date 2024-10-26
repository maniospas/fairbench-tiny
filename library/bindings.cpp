#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>  // Enables std::map and std::vector conversions
#include <string>
#include <vector>
#include "../fairbench/types.h"
#include "../fairbench/metrics.h"
#include "../fairbench/reductions.h"
#include "../fairbench/report.h"

namespace py = pybind11;
using namespace fb;

std::vector<double> numpy_to_vector(const py::array_t<double>& array) {
    auto buf = array.request();
    double* ptr = static_cast<double*>(buf.ptr);
    return std::vector<double>(ptr, ptr + buf.size);
}

py::dict report(py::array_t<double> predict, 
                py::array_t<double> label, 
                py::dict sensitive) {
    
    // construct the data
    Data data;
    data.predict = std::move(numpy_to_vector(predict));
    data.label = std::move(numpy_to_vector(label));
    for (auto item : sensitive) {
        std::string key = py::str(item.first);
        py::array_t<double> array = item.second.cast<py::array_t<double>>();
        data.sensitive[key] = std::vector<double>(array.data(), array.data() + array.size());
    }

    // generate the report
    py::dict result;
    try {
        Report report = assessment(data, registry.metrics, registry.reductions);
        //print(report);
        for (const auto& outer_pair : report) {
            py::dict inner_dict;
            for (const auto& inner_pair : outer_pair.second) 
                inner_dict[inner_pair.first.c_str()] = inner_pair.second;
            result[outer_pair.first.c_str()] = inner_dict;
        }
    }
    catch (const invalid_argument &e) {
        throw py::value_error(e.what());
    }

    return result;
}

// Define a pybind11 module, exposing the functions to Python
PYBIND11_MODULE(fairbenchtiny, m) {
    m.def("report", &report, "Create a fairness report");
}
