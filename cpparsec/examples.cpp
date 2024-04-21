//#include "cpparsec.h"
//
//using namespace cpparsec;
//
//Parser<std::pair<int, std::string>> cubeParser() {
//    return CPPARSEC_DEFN(std::pair<int, std::string>) {
//        CPPARSEC_SAVE(cubeNum, int_() << space());
//        CPPARSEC_SAVE(cubeColor, string_("red") | string_("green") | string_("blue"));
//
//        return std::pair{ cubeNum, cubeColor };
//    };
//}
//
//// return Parser<T>() = [](InputStream& input) -> ParseResult<T> ;
//Parser<std::tuple<int, std::string>> cubeParser2() {
//    return (int_() << space()) & (string_("red") | string_("green") | string_("blue"));
//}
