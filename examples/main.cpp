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

#include "../fairbench/all.h"
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
