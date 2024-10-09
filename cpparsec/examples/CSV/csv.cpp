#include <vector>
#include <string>
#include "../../cpparsec.h"
#include "../../cpparsec_numeric.h"

using namespace cpparsec;

// Parses a csv line
Parser<std::vector<std::string>> csv_line() {
    auto nonCommaChar = [](char c) { return (c != ','); };
    return sep_by1(many(char_satisfy(nonCommaChar)), char_(','));
}

