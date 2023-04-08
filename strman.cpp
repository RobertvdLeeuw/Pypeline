#include <regex>

#include "common.h"

bool endsWith(string const & value, string const & ending) {
    if (ending.size() > value.size()) return false;  // Substring larger than string
    return equal(ending.rbegin(), ending.rend(), value.rbegin());
}

bool startsWith(string const & value, string const & start) {
    if (start.size() > value.size()) return false;  // Substring larger than string
    return equal(start.begin(), start.end(), value.begin());
}

string trim(const string& input) {
    return regex_replace(input, std::regex(R"((^(^\s+|\s+$)|\r))"), "");
}

string getProjectPath(const string& file_path) {
    return regex_replace(file_path, std::regex("[a-zA-Z0-9]+\\.py"), "test.json");
}