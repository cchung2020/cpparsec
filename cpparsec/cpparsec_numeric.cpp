#include "cpparsec.h"
#include "cpparsec_char.h"
#include "cpparsec_numeric.h"

namespace cpparsec {

    // =========================== Numeric Parsers ============================
    namespace helper {
        auto stoi(const std::string& str) {
            return std::stoi(str);
        }

        auto stol(const std::string& str) {
            return std::stol(str);
        }
    }

    // Parses an int
    Parser<int> int_() {
        return many1(digit()).transform<int>(helper::stoi);
    }

    // Parses an int faster
    Parser<int> int2_() {
        return many1(digit2()).transform<int>(helper::stoi);
    }

};
