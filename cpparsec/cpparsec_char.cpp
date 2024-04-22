#include "cpparsec_char.h"
#include <cctype>

namespace cpparsec {

    // ========================== Character Parsers ===========================

    // Parses a single character
    Parser<char> char_(char c) {
        return CPPARSEC_DEFN(char) {
            CPPARSEC_FAIL_IF(input.empty(), ParseError("end of input", { c }));
            CPPARSEC_FAIL_IF(input[0] != c, ParseError(input[0], c));

            input.remove_prefix(1);
            return c;
        };
    }

    // Parses any character
    Parser<char> any_char() {
        return Parser<char>([](InputStream& input) -> ParseResult<char> {
            CPPARSEC_FAIL_IF(input.empty(), ParseError("any_char: end of input"));

            char c = input[0];
            input.remove_prefix(1);
            return c;
            });
    }

    // Parses a single character that satisfies a constraint
    // Faster than try_(any_char().satisfy(cond))
    Parser<char> char_satisfy(UnaryPredicate<char> auto cond, std::string&& err_msg = "<char_satisfy>") {
        return CPPARSEC_DEFN(char) {
            CPPARSEC_FAIL_IF(input.empty(), ParseError(err_msg, "end of input"));
            CPPARSEC_FAIL_IF(!cond(input[0]), ParseError(err_msg, { input[0] }));

            char c = input[0];
            input.remove_prefix(1);
            return c;
        };
    }

    // Parses a single string
    Parser<std::string> string_(const std::string& str) {
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
    Parser<char> letter() {
        return char_satisfy(isalpha, "<letter>");
    }

    // Parses a single digit
    Parser<char> digit() {
        return char_satisfy(isdigit, "<digit>");
    }

    // Parses a single digit
    Parser<char> digit2() {
        return char_satisfy(isdigit);
    }

    // Parses a single space
    Parser<char> space() {
        return char_satisfy(isspace, "<space>");
    }

    // Skips zero or more spaces
    Parser<std::monostate> spaces() {
        return skip_many(space());
    }

    // Skips one or more spaces
    Parser<std::monostate> spaces1() {
        return skip_many1(space());
    }

    // Parses a single newline '\n'
    Parser<char> newline() {
        return char_('\n');
    }

    // Parses a single uppercase letter 
    Parser<char> upper() {
        return char_satisfy(isupper);
    }

    // Parses a single lowercase letter 
    Parser<char> lower() {
        return char_satisfy(islower);
    }

    // Parses a single alphanumeric letter 
    Parser<char> alpha_num() {
        return char_satisfy(isalnum);
    }
};
