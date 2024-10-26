#include "fairbench/types.h"
#include "fairbench/reductions.h"
#include "fairbench/metrics.h"
#include "fairbench/report.h"

// COMPILE: g++ main.cpp -O2 -o main.exe
using namespace fb;

int main() {
    Data data;
    data.sensitive["male"] = {1, 0, 1, 0};
    data.sensitive["female"] = {0, 1, 0, 1};

    data.predict = {1, 2, 3, 4};
    data.label = {1, 2, 3, 3};

    try {
        Report report = assessment(data, registry.metrics, registry.reductions);
        print(report);
    }
    catch (const invalid_argument &e) {
        cerr << RED_TEXT << "Invalid argument: " << RESET_TEXT << e.what() << endl;
    }

    return 0;
}
