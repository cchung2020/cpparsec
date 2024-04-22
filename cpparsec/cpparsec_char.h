#ifndef CPPARSEC_CHAR_H
#define CPPARSEC_CHAR_H

#include "cpparsec_core.h"

namespace cpparsec {

    // ========================== CHARACTER PARSERS ===========================
    // Parses a single character
    inline Parser<char> char_(char c) {
        return CPPARSEC_DEFN(char) {
            CPPARSEC_FAIL_IF(input.empty(), ParseError("end of input", { c }));
            CPPARSEC_FAIL_IF(input[0] != c, ParseError(input[0], c));

            input.remove_prefix(1);
            return c;
        };
    }

    // Parses any character
    inline Parser<char> any_char() {
        return Parser<char>([](InputStream& input) -> ParseResult<char> {
            CPPARSEC_FAIL_IF(input.empty(), ParseError("any_char: end of input"));

            char c = input[0];
            input.remove_prefix(1);
            return c;
            });
    }

    // Parses a single character that satisfies a constraint
    // Faster than try_(any_char().satisfy(cond))
    inline Parser<char> char_satisfy(UnaryPredicate<char> auto cond, std::string&& err_msg = "<char_satisfy>") {
        return CPPARSEC_DEFN(char) {
            CPPARSEC_FAIL_IF(input.empty(), ParseError(err_msg, "end of input"));
            CPPARSEC_FAIL_IF(!cond(input[0]), ParseError(err_msg, { input[0] }));

            char c = input[0];
            input.remove_prefix(1);
            return c;
        };
    }

    // Parses a single string
    inline Parser<std::string> string_(const std::string& str) {
        return CPPARSEC_DEFN(std::string) {
            CPPARSEC_FAIL_IF(str.size() > input.size(), ParseError("end of input", { str[0] }));

            for (auto [i, c] : str | std::views::enumerate) {
                if (c != input[i]) {
                    char c2 = input[i];
                    auto str2 = std::string(input.substr(0, i + 1));
                    input.remove_prefix(i);

                    CPPARSEC_FAIL(ParseError(c, c2).add_error({ std::pair{ str, str2 } }));
                }
            }

            input.remove_prefix(str.size());
            return str;
        };
    }

    // Parses a single letter
    inline Parser<char> letter() {
        return char_satisfy(isalpha, "<letter>");
    }

    // Parses a single digit
    inline Parser<char> digit() {
        return char_satisfy(isdigit, "<digit>");
    }

    // Parses a single digit
    inline Parser<char> digit2() {
        return char_satisfy(isdigit);
    }

    // Parses a single space
    inline Parser<char> space() {
        return char_satisfy(isspace, "<space>");
    }

    // Skips zero or more spaces
    inline Parser<std::monostate> spaces() {
        return skip_many(space());
    }

    // Skips one or more spaces
    inline Parser<std::monostate> spaces1() {
        return skip_many1(space());
    }

    // Parses a single newline '\n'
    inline Parser<char> newline() {
        return char_('\n');
    }

    // Parses a single uppercase letter 
    inline Parser<char> upper() {
        return char_satisfy(isupper);
    }

    // Parses a single lowercase letter 
    inline Parser<char> lower() {
        return char_satisfy(islower);
    }

    // Parses a single alphanumeric letter 
    inline Parser<char> alpha_num() {
        return char_satisfy(isalnum);
    }

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
