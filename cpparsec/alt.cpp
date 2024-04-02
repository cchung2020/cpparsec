#include <functional>
#include <optional>
#include <string>
#include <vector>
#include <memory>
#include <print>
#include <ranges>
#include <iostream>
#include <string_view>
#include <expected>

using std::print, std::println, std::cout;

namespace cpparsec2 {
    // Error type for parser failures
    struct ParseError {
        std::string message;
        size_t line;
        size_t column;
    };

    // A parser result can either be successful (carrying the parsed value) or a failure (carrying an error message)
    template<typename T>
    using ParserResult = std::expected<T, ParseError>;

    using InputStream = std::string_view;

    template<typename T>
    class Parser {
    public:
        using ParseFunction = std::function<ParserResult<T>(InputStream&)>;
    private:
        std::shared_ptr<ParseFunction> parser;
    public:
        Parser(ParseFunction parser) :
            parser(std::make_shared<ParseFunction>(parser))
        { }
        // Top level parser execution, parses a string_view
        ParserResult<T> parse(InputStream& input) const {
            return (*parser)(input);
        }
        // Parses self, then another parser
        template<typename U>
        Parser<U> with(Parser<U> other) const {
            return Parser<U>([=](InputStream& input) -> ParserResult<U> {
                auto result = this->parse(input);
                if (result) {
                    return other.parse(input);
                }
                return std::unexpected(result.error());
                });
        }
    };

    // Parsers a single character
    Parser<char> character(char c) {
        return Parser<char>([c](InputStream& input) -> ParserResult<char> {
            if (!input.empty() && input[0] == c) {
                input.remove_prefix(1);
                return c;
            }
            return std::unexpected(ParseError{ "Unexpected character", 0, 0 });
            });
    }

    // Parse zero or more occurrences
    template<typename T>
    Parser<std::vector<T>> many(Parser<T> p) {
        return Parser<std::vector<T>>([=](InputStream& input) -> ParserResult<std::vector<T>> {
            std::vector<T> values;
            InputStream original = input;
            while (true) {
                auto result = p.parse(input);
                if (result) {
                    values.push_back(*result);
                }
                else {
                    if (values.empty()) {
                        return std::unexpected(result.error());
                    }
                    break;
                }
            }
            if (!input.empty()) {
                return std::unexpected(ParseError{ "Unexpected characters after parsed input", 0, original.size() - input.size() });
            }
            return values;
            });
    }

    // Parse occurrence between two parses
    template<typename O, typename C, typename T>
    Parser<T> between(Parser<O> open, Parser<C> close, Parser<T> p) {
        return Parser<T>([=](InputStream& input) -> ParserResult<T> {
            auto openResult = open.parse(input);
            if (!openResult) {
                return std::unexpected(openResult.error());
            }
            auto valueResult = p.parse(input);
            if (!valueResult) {
                return std::unexpected(valueResult.error());
            }
            auto closeResult = close.parse(input);
            if (!closeResult) {
                return std::unexpected(closeResult.error());
            }
            return *valueResult;
            });
    }
}
