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

// compile: g++ fbt.cpp -O2 -o fbt -std=c++20

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include "fairbench/all.h"

using namespace std;
using namespace fb;

#define SEPARATOR (fb::CYAN_TEXT+"--------------------------------------------------------------------------\n"+fb::RESET_TEXT)

vector<string> split(const string& s, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

char auto_detect_delimiter(const string& line) {
    vector<char> delimiters = {',', '\t', ';', '|'};
    for (char delimiter : delimiters) {
        vector<string> tokens = split(line, delimiter);
        if (tokens.size() > 1) {
            return delimiter;
        }
    }
    return '\0'; // Return null character if no delimiter found
}

struct Arguments {
    string task;
    string filename;
    string predict_column = "predict";
    string label_column = "label";
    bool intersect = false;
    vector<string> sensitive_columns;
    double threshold = 1;
    char delimiter = '\0'; // Default: no delimiter specified
};

double stringToDouble(const std::string& str) {
    try {
        // Attempt to convert the string to a double
        size_t pos;
        double value = std::stod(str, &pos);
        if (pos != str.length()) {
            throw std::invalid_argument("String contains non-numeric characters");
        }
        return value;
    } catch (const std::invalid_argument& e) {
        throw std::runtime_error("Invalid argument: unable to convert string to double");
    } catch (const std::out_of_range& e) {
        throw std::runtime_error("Out of range: number is too large to fit in a double");
    }
}


Arguments parse_arguments(int argc, char* argv[]) {
    Arguments args;

    if (argc < 3) { // At least program name, task, and filename
        cerr << "Usage: " << argv[0] << " <task> <csv_file> [options]" << endl;
        cerr << "Available tasks:" << endl;
        cerr << "  report     Print a consise fairness report" << endl;
        cerr << "  details    Print detailed fairness report that includes intermediate values" << endl;
        cerr << "  cli        Command line exploration of a fairness report" << endl;
        cerr << "  silent     Compute a report without printing (pair with --threshold for testing)" << endl;
        cerr << "Options:" << endl;
        cerr << "  --predict <predict_column_name>      Column name for predict (default: 'predict')" << endl;
        cerr << "  --label <label_column_name>          Column name for label (default: 'label')" << endl;
        cerr << "  --sensitive <col1,col2,...>          Comma-separated list of sensitive columns (default: all other columns)" << endl;
        cerr << "  --intersect                          Adds non-empty intersections of sensitive" << endl;
        cerr << "  --delimiter <delimiter>              Delimiter character (one character only)" << endl;
        cerr << "  --threshold <threshold>              Threshold of when to consider deviations as biased. If it is exceeded"<<endl;
        cerr << "                                       for any report value, exit code 1 is generated, for example for CI.,"<<endl;
        cerr << "                                       (Default value is 1, which never triggers this behavior.)" << endl;
        exit(1);
    }

    int i = 1;
    args.task = argv[i];

    // Check that task does not start with '--'
    if (args.task.rfind("--", 0) == 0) {
        cerr << "First argument must be the task (e.g., 'report'), not an option." << endl;
        exit(1);
    }

    // Currently, only 'report' is supported
    if (args.task != "report" && args.task != "cli" && args.task != "details" && args.task != "silent") {
        cerr << "Unknown task: " << args.task << endl;
        cerr << "Available tasks: report, details, cli, silent" << endl;
        exit(1);
    }

    ++i;
    if (i >= argc) {
        cerr << "CSV file not specified" << endl;
        exit(1);
    }
    args.filename = argv[i];

    ++i;
    while (i < argc) {
        string arg = argv[i];

        if (arg == "--intersect") {
            args.intersect = true;
            
        } else if (arg == "--predict") {
            if (i + 1 >= argc) {
                cerr << "--predict option requires an argument" << endl;
                exit(1);
            }
            args.predict_column = argv[++i];
        } else if (arg == "--label") {
            if (i + 1 >= argc) {
                cerr << "--label option requires an argument" << endl;
                exit(1);
            }
            args.label_column = argv[++i];
        } else if (arg == "--sensitive") {
            if (i + 1 >= argc) {
                cerr << "--sensitive option requires an argument" << endl;
                exit(1);
            }
            string sensitive_arg = argv[++i];
            args.sensitive_columns = split(sensitive_arg, ',');
        } else if (arg == "--threshold") {
            if (i + 1 >= argc) {
                cerr << "--threshold option requires an argument" << endl;
                exit(1);
            }
            string threshold_arg = argv[++i];
            args.threshold = stringToDouble(threshold_arg);
        } else if (arg == "--delimiter") {
            if (i + 1 >= argc) {
                cerr << "--delimiter option requires an argument" << endl;
                exit(1);
            }
            string delimiter_arg = argv[++i];
            if (delimiter_arg.length() != 1) {
                cerr << "--delimiter argument must be a single character" << endl;
                exit(1);
            }
            args.delimiter = delimiter_arg[0];
        } else if (arg.rfind("--", 0) == 0) {
            cerr << "Unknown option: " << arg << endl;
            exit(1);
        } else {
            cerr << "Unexpected argument: " << arg << endl;
            cerr << "Usage: " << argv[0] << " <task> <csv_file> [options]" << endl;
            exit(1);
        }
        ++i;
    }

    return args;
}


Data read_csv(const Arguments& args) {
    const string& filename = args.filename;
    const string& predict_column = args.predict_column;
    const string& label_column = args.label_column;
    const vector<string>& sensitive_columns = args.sensitive_columns;
    char delimiter = args.delimiter;

    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Could not open file: " << filename << endl;
        exit(1);
    }

    // Parse first line
    string line;
    if (!getline(file, line)) {
        cerr << "Error reading the first line (column names)" << endl;
        exit(1);
    }
    if (delimiter == '\0') {
        delimiter = auto_detect_delimiter(line);
        if (delimiter == '\0') {
            cerr << "Could not determine delimiter from first line. Please specify the delimiter using --delimiter option." << endl;
            exit(1);
        }
    }

    // Split the first line to get column names
    vector<string> column_names = split(line, delimiter);
    map<string, int> column_indices;
    for (size_t i = 0; i < column_names.size(); ++i)
        column_indices[column_names[i]] = i;

    // Check for 'predict' and 'label' columns
    if (column_indices.find(predict_column) == column_indices.end()) {
        cerr << "Column \"" << predict_column << "\" not found in CSV file" << endl;
        exit(1);
    }

    if (column_indices.find(label_column) == column_indices.end()) {
        cerr << "Column \"" << label_column << "\" not found in CSV file" << endl;
        exit(1);
    }

    int predict_idx = column_indices[predict_column];
    int label_idx = column_indices[label_column];

    // If sensitive columns are not specified, consider all columns except 'predict' and 'label' as sensitive
    vector<string> sensitive_cols = sensitive_columns;
    if (sensitive_cols.empty()) {
        for (const auto& col : column_names) {
            if (col != predict_column && col != label_column) {
                sensitive_cols.push_back(col);
            }
        }
    } else {
        // Check that all specified sensitive columns exist
        for (const auto& col : sensitive_cols) {
            if (column_indices.find(col) == column_indices.end()) {
                cerr << "Sensitive column \"" << col << "\" not found in CSV file" << endl;
                exit(1);
            }
        }
    }

    Data data;
    for (const auto& col : sensitive_cols)
        data.sensitive[col] = vector<double>();

    int line_number = 1; // We have already read the header line
    while (getline(file, line)) {
        ++line_number;
        if (line.empty())
            continue;
        vector<string> tokens = split(line, delimiter);
        if (tokens.size() != column_names.size()) {
            cerr << "Mismatch in number of columns at line " << line_number << ": " << line << endl;
            exit(1);
        }

        // Parse 'predict' and 'label' columns
        double predict_value;
        double label_value;
        try {
            predict_value = stod(tokens[predict_idx]);
            label_value = stod(tokens[label_idx]);
        } catch (const invalid_argument& e) {
            cerr << "Invalid numeric value at line " << line_number << ": " << e.what() << endl;
            exit(1);
        } catch (const out_of_range& e) {
            cerr << "Numeric value out of range at line " << line_number << ": " << e.what() << endl;
            exit(1);
        }

        data.predict.push_back(predict_value);
        data.label.push_back(label_value);

        // Parse sensitive columns
        for (const auto& col : sensitive_cols) {
            int idx = column_indices[col];
            double value;
            try {
                value = stod(tokens[idx]);
            } catch (const invalid_argument& e) {
                cerr << "Invalid numeric value in sensitive column '" << col << "' at line " << line_number << ": " << e.what() << endl;
                exit(1);
            } catch (const out_of_range& e) {
                cerr << "Numeric value out of range in sensitive column '" << col << "' at line " << line_number << ": " << e.what() << endl;
                exit(1);
            }
            data.sensitive[col].push_back(value);
        }
    }
    if(args.intersect)
        add_intersections(data.sensitive);

    return data;
}

namespace Terminal {
    #ifdef _WIN32
    #include <windows.h>

    // Function to enable virtual terminal processing and set UTF-8 encoding
    void enableVirtualTerminalProcessing() {
        // Enable Virtual Terminal Processing on Windows
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE) {
            return;
        }

        DWORD dwMode = 0;
        if (!GetConsoleMode(hOut, &dwMode)) {
            return;
        }

        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (!SetConsoleMode(hOut, dwMode)) {
            return;
        }

        // Set console output to UTF-8
        SetConsoleOutputCP(CP_UTF8);
    }
    #else
    void enableVirtualTerminalProcessing(){}
    #endif
}


void start_cli(const Report &report, bool verbose=false) {
    cout << "\033[2J";
    cout << "Available commands for the exploration of the report:\n";
    cout << "  exit             Exit the command line interface\n";
    cout << "  reset            Go back to the full report\n";
    cout << "  transpose        Transpose the current view\n";
    cout << "  view <name>      Focus on a row or column given its name\n";
    cout << "  details          Provide verbose details about each value\n";
    cout << SEPARATOR;
    if(verbose)
        details(report);
    else
        print(report);
    cout << SEPARATOR;
}

void cli(const Report &_report) {
    Report report = _report;
    start_cli(report);

    string input;
    while(true) {
        cout << "> ";
        getline(cin, input, '\n');
        auto command = split(input, ' ');
        try {
            if(command.size()==1 && command[0]=="exit")
                return;
            if(command.size()==2 && command[0]=="view") {
                report = access(report, command[1]);
                start_cli(report);
                continue;
            }
            if(command.size()==1 && command[0]=="transpose") {
                report = transpose(report);
                start_cli(report);
                continue;
            }
            if(command.size()==1 && command[0]=="reset") {
                report = _report;
                start_cli(report);
                continue;
            }
            if(command.size()==1 && command[0]=="details") {
                start_cli(report, true);
                continue;
            }
        }
        catch (const invalid_argument &e) {
            cerr << RED_TEXT << e.what() << endl << RESET_TEXT;
            continue;
        }
        cout << RED_TEXT << "Unknown command or format\n" << RESET_TEXT;
    }
}

int main(int argc, char* argv[]) {
    Terminal::enableVirtualTerminalProcessing();
    Arguments args = parse_arguments(argc, argv);

    if(args.task == "report" || args.task == "details" || args.task == "silent") {
        try {
            Report report;
            { // scope this to get the data cleared by RAI
                Data data = read_csv(args);
                bool printComma = false;
                cout << "Report on " << data.sensitive.size() << " sensitive attributes: ";
                for(const auto& attr : data.sensitive) {
                    if(printComma)
                        cout << ",";
                    printComma = true;
                    cout << attr.first;
                }
                cout << "\n";
                cout << SEPARATOR;
                report = assessment(data, registry.metrics, registry.reductions);
            }
            if(args.task == "details")
                details(report);
            else if(args.task == "report")
                print(report);
            cout << SEPARATOR;
            // check for thresholds
            std::ostringstream output;
            for (const auto& entry : report) 
                for (const auto& value : entry.second) 
                    if (value.second.get() > args.threshold) {
                        output << "\n" << std::fixed << std::setprecision(3);
                        output << left << std::setw(20) << (entry.first + " " + value.first) << " " << RED_TEXT << value.second.get() << RESET_TEXT;
                    }
            string desc = output.str();
            if(desc.size()) {
                std::cout << "The following values exceeded the value set by --threshold "+to_string(args.threshold)+":" << desc << "\n";
                return 1;
            }
        }
        catch (const invalid_argument &e) {
            cerr << e.what() << endl;
            return 1;
        }
        return 0;
    }

    if(args.task == "cli") {
        Data data = read_csv(args);
        try {
            Report report;
            { // scope this to get the data cleared by RAI
                Data data = read_csv(args);
                report = assessment(data, registry.metrics, registry.reductions);
                // check for thresholds
                std::ostringstream output;
                for (const auto& entry : report) 
                    for (const auto& value : entry.second) 
                        if (value.second.get() > args.threshold) {
                            output << "\n" << std::fixed << std::setprecision(3);
                            output << left << std::setw(20) << (entry.first + " " + value.first) 
                                << " " << RED_TEXT << value.second.get() << RESET_TEXT;
                        }
                string desc = output.str();
                if(desc.size()) {
                    std::cout << "The following values exceeded the maximum bias --threshold "+to_string(args.threshold)+":" << desc << "\n";
                    return 1;
                }
            }
            cli(report);
        }
        catch (const invalid_argument &e) {
            cerr << e.what() << endl;
            return 1;
        }
        return 0;
    }

    cerr << "Unknown task: " << args.task << endl;
    return 1;
}
