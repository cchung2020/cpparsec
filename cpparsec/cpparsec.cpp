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
