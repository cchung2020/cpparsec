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

    // ======================== STRING SPECIALIZATIONS ========================

    // Parse zero or more characters, std::string specialization
    template <PushBack<char> StringContainer>
    Parser<StringContainer> many(Parser<char> charP);

    // Parse one or more characters, std::string specialization
    template <PushBack<char> StringContainer>
    Parser<StringContainer> many1(Parser<char> charP);

    // Parses p zero or more times until end succeeds, returning the parsed values, std::string specialization
    template <typename T, PushBack<char> StringContainer>
    Parser<StringContainer> many_till(Parser<char> p, Parser<T> end);

    // Parses p one or more times until end succeeds, returning the parsed values, std::string specialization
    template <typename T, PushBack<char> StringContainer>
    Parser<StringContainer> many1_till(Parser<char> p, Parser<T> end);

    // Parse zero or more parses of p separated by sep, std::string specialization
    template <typename T>
    Parser<std::string> sep_by(Parser<char> p, Parser<T> sep);

    // Parse one or more parses of p separated by sep, std::string specialization
    template <typename T>
    Parser<std::string> sep_by1(Parser<char> p, Parser<T> sep);

    // Parse zero or more parses of p separated by and ending with sep, std::string specialization
    template <typename T>
    Parser<std::string> end_by(Parser<char> p, Parser<T> sep);

    // Parse one or more parses of p separated by and ending with sep, std::string specialization
    template <typename T>
    Parser<std::string> end_by1(Parser<char> p, Parser<T> sep);

    // ========================================================================
    // 
    // ======================= TEMPLATE IMPLEMENTATIONS =======================
    // 
    // ========================================================================


    // ======================== String Specializations ========================

    // Parse zero or more characters, std::string specialization
    template <PushBack<char> StringContainer = std::string>
    Parser<StringContainer> many(Parser<char> charP) {
        return detail::many_accumulator(charP, StringContainer());
    }

    // Parse one or more characters, std::string specialization
    template <PushBack<char> StringContainer = std::string>
    Parser<StringContainer> many1(Parser<char> charP) {
        return CPPARSEC_DEFN(StringContainer) {
            CPPARSEC_SAVE(first, charP);
            CPPARSEC_SAVE(values, detail::many_accumulator(charP, StringContainer({ first })));

            return values;
        };
    }

    // Parses p zero or more times until end succeeds, returning the parsed values, std::string specialization
    template <typename T, PushBack<char> StringContainer = std::string>
    Parser<StringContainer> many_till(Parser<char> p, Parser<T> end) {
        return detail::many_till_accumulator(p, end, StringContainer());
    }

    // Parses p one or more times until end succeeds, returning the parsed values, std::string specialization
    template <typename T, PushBack<char> StringContainer = std::string>
    Parser<StringContainer> many1_till(Parser<char> p, Parser<T> end) {
        return CPPARSEC_DEFN(StringContainer) {
            CPPARSEC_SAVE(first, p);
            CPPARSEC_SAVE(values, detail::many_till_accumulator(p, end, StringContainer({ first })));

            return values;
        };
    }

    // Parse zero or more parses of p separated by sep, std::string specialization
    template <typename T>
    Parser<std::string> sep_by(Parser<char> p, Parser<T> sep) {
        return sep_by1(p, sep) | success("");
    }

    // Parse one or more parses of p separated by sep, std::string specialization
    template <typename T>
    Parser<std::string> sep_by1(Parser<char> p, Parser<T> sep) {
        return CPPARSEC_DEFN(std::string) {
            CPPARSEC_SAVE(first, p);
            CPPARSEC_SAVE(values, detail::many_accumulator(sep >> p, std::string(1, first)));

            return values;
        };
    }

    // Parse zero or more parses of p separated by and ending with sep, std::string specialization
    template <typename T>
    Parser<std::string> end_by(Parser<char> p, Parser<T> sep) {
        return many(p << sep);
    }

    // Parse one or more parses of p separated by and ending with sep, std::string specialization
    template <typename T>
    Parser<std::string> end_by1(Parser<char> p, Parser<T> sep) {
        return many1(p << sep);
    }
};

#endif /* CPPARSEC_CHAR_H */
