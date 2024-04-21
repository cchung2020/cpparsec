#ifndef CPPARSEC_CHAR_H
#define CPPARSEC_CHAR_H

#include "cpparsec.h"

namespace cpparsec {

    // ========================== CHARACTER PARSERS ===========================

    // Parses a single character
    Parser<char> char_(char c);

    // Parses any character
    Parser<char> any_char();

    // Parses a single character that satisfies a constraint
    // Faster than try_(any_char().satisfy(cond))
    Parser<char> char_satisfy(UnaryPredicate<char> auto cond, std::string&& err_msg);

    // Parses a single string
    Parser<std::string> string_(const std::string& str);

    // Parses a single letter
    Parser<char> letter();

    // Parses a single digit
    Parser<char> digit();

    // Parses a single digit, no error message
    Parser<char> digit2();

    // Parses a single space
    Parser<char> space();

    // Skips zero or more spaces
    Parser<std::monostate> spaces();

    // Skips one or more spaces
    Parser<std::monostate> spaces1();

    // Parses a single newline '\n'
    Parser<char> newline();

    // Parses a single uppercase letter 
    Parser<char> upper();

    // Parses a single lowercase letter 
    Parser<char> lower();

    // Parses a single alphanumeric letter 
    Parser<char> alpha_num();

};

#endif /* CPPARSEC_CHAR_H */
