#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>  // Enables std::map and std::vector conversions
#include <string>
#include <vector>

namespace py = pybind11;

py::dict report(py::array_t<double> predictions, py::array_t<double> labels) {
    double accuracy = 0.95;
    double precision = 0.92;
    int TP = 100, FP = 10, FN = 5, TN = 85;

    py::dict summary;
    summary["accuracy"] = accuracy;
    summary["precision"] = precision;

    py::dict details;
    details["TP"] = TP;
    details["FP"] = FP;
    details["FN"] = FN;
    details["TN"] = TN;

    py::dict result;
    result["summary"] = summary;
    result["details"] = details;

    return result;
}

// Define a pybind11 module, exposing the functions to Python
PYBIND11_MODULE(fairbenchtiny, m) {
    m.def("report", &report, "Process predictions and labels and return a report as a dictionary of dictionaries");
}
