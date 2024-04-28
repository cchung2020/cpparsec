#include "cpparsec.h"

namespace cpparsec {
    // =========================== Numeric Parsers ============================

    // Parses an int
    inline Parser<int> int_() {
        return many1(digit()).transform<int>([](auto&& s) { return std::stoi(s); });
    }

};
