#include "cpparsec.h"
#include "cpparsec_numeric.h"

using namespace cpparsec;

Parser<std::pair<int, std::string>> cubeParser() {
    return CPPARSEC_DEFN(std::pair<int, std::string>) {
        CPPARSEC_SAVE(cubeNum, int_() << space());
        CPPARSEC_SAVE(cubeColor, string_("red") | string_("green") | string_("blue"));

        return std::pair{ cubeNum, cubeColor };
    };
}

Parser<std::pair<int, std::string>> cubeParser2() {
    return (int_() << space()).pair_with((string_("red") | string_("green") | string_("blue")));
}

