#include "cpparsec.h"

namespace cpparsec {
    // =========================== Numeric Parsers ============================
    // Parses an int
    inline Parser<int> int_() {
        return many1(digit()).transform<int>([](auto&& s) { return std::stoi(s); });
    }

    // Parses an int faster
    inline Parser<int> int2_() {
        return many1(digit2()).transform<int>([](auto&& s) { return std::stoi(s); });
    }

};
