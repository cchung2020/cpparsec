#include "cpparsec.h"

namespace cpparsec {
    // ========================================================================
    // 
    // ===================== NON TEMPLATE IMPLEMENTATIONS =====================
    // 
    // ========================================================================

    // =========================== ParseError =================================

    ParseError::ParseError(ErrorContent&& err)
        : errors({ err })
    {}
    ParseError::ParseError(const std::string expected, const std::string found)
        : errors({ ErrorContent{std::pair {expected, found}} })
    {}
    ParseError::ParseError(char expected, char found)
        : errors({ ErrorContent{std::pair{expected, found}} })
    {}
    ParseError::ParseError(std::string&& msg)
        : errors({ ErrorContent{msg} })
    {}

    ParseError& ParseError::add_error(ErrorContent&& err) {
        errors.push_back(err);
        return *this;
    }

    std::string ParseError::message() {
        return std::format("{}", errors.front());
    }

    std::string ParseError::message_top() {
        return std::format("{}", errors.back());
    }

    std::string ParseError::message_stack() {
        std::string msg = std::format("{}", errors[0]);

        for (size_t i = 1; i < errors.size(); i++) {
            msg += std::format("\n{}", errors[i]);
        }

        return msg;
    }

    // ============================= Core Parsers =============================

    // Parses a single character
    Parser<char> char_(char c) {
        return CPPARSEC_DEFN(char) {
            CPPARSEC_FAIL_IF(input.empty(), ParseError("end of input", { c }));
            CPPARSEC_FAIL_IF(input[0] != c, ParseError(input[0], c));

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

                    CPPARSEC_FAIL(ParseError(c, c2).add_error(ErrorContent{ std::pair{ str, str2 } }));
                }
            }

            input.remove_prefix(str.size());
            return str;
        };
    }

    // Parses any character
    Parser<char> any_char() {
        return Parser<char>([](InputStream& input) -> ParseResult<char> {
            CPPARSEC_FAIL_IF(input.empty(), ParseError("any_char: unexpected end of input"));

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

    // ======================= Core Parser Combinators ========================

    // Parse zero or more characters, std::string specialization
    Parser<std::string> many(Parser<char> charP) {
        return helper::many_accumulator(charP, std::string());
    }

    // Parse one or more characters, std::string specialization
    Parser<std::string> many1(Parser<char> charP) {
        return CPPARSEC_DEFN(std::string) {
            CPPARSEC_SAVE(first, charP);
            CPPARSEC_SAVE(values, helper::many_accumulator(charP, std::string({ first })));

            return values;
        };
    }

    // ========================== Character Parsers ===========================

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

// needs to be outside namespace to be seen by fmt
template <>
struct std::formatter<cpparsec::ErrorContent> {
    auto parse(std::format_parse_context& ctx) {
        return ctx.end();
    }

    auto format(const cpparsec::ErrorContent& error, std::format_context& ctx) const {
        return std::visit([&ctx](auto&& err) {
            using T = std::decay_t<decltype(err)>;
            if constexpr (std::is_same_v<T, std::pair<char, char>>) {
                return std::format_to(ctx.out(), "Expected '{}', found '{}'", err.first, err.second);
            }
            else if constexpr (std::is_same_v<T, std::pair<std::string, std::string>>) {
                return std::format_to(ctx.out(), "Expected \"{}\", found \"{}\"", err.first, err.second);
            }
            else if constexpr (std::is_same_v<T, std::string>) {
                return std::format_to(ctx.out(), "{}", err);
            }
            else if constexpr (std::is_same_v<T, std::monostate>) {
                return std::format_to(ctx.out(), "empty error");
            }
            }, error);
    }
};