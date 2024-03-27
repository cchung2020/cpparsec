#ifndef CPPARSEC_H
#define CPPARSEC_H

#include <functional>
#include <optional>
#include <string>
#include <vector>
#include <memory>
#include <print>
#include <ranges>
#include <iostream>
#include <string_view>
#include <cctype>

#define CPPARSEC_RETURN(val) \
    if (input) { return {val}; } return std::nullopt;
#define CPPARSEC_RETURN_NAMEDINPUT(input, val) \
    if (input) { return val; } return std::nullopt;


#define CPPARSEC_SAVE_NAMEDINPUT(var, p, input) \
    auto _##var = p.parse(input); \
    if (!_##var.has_value()) {         \
        return std::nullopt;      \
    }                             \
    auto var = _##var.value();
#define CPPARSEC_SAVE(var, p) \
    CPPARSEC_SAVE_NAMEDINPUT(var, p, input);

#define CPPARSEC_SAVE_PARSER_NAMEDINPUT(var, p, input) \
    auto _##var = p(input); \
    if (!_##var.has_value()) {         \
        return std::nullopt;      \
    }                             \
    auto var = _##var.value();
#define CPPARSEC_SAVE_PARSER(var, p) \
    CPPARSEC_SAVE_PARSER_NAMEDINPUT(var, p, input);

#define CPPARSEC_SKIP_NAMEDINPUT(p, input) \
    if (!p.parse(input)) { return std::nullopt; }
#define CPPARSEC_SKIP(p) \
    CPPARSEC_SKIP_NAMEDINPUT(p, input)

#define CPPARSEC_FAIL_IF(cond) \
    if (cond) { return std::nullopt; }

using std::print, std::println, std::cout;

// #include <string_view>
// #include <expected>

namespace cpparsec {

    // A parser result can either be successful (carrying the parsed value and remaining input)
    // or a failure (optionally carrying an error message).
    template<typename T>
    using ParserResult = std::optional<T>;

    using InputStream = std::optional<std::string_view>;

    template<typename T>
    class Parser {
    public:
        using ParseFunction = std::function<ParserResult<T>(InputStream&)>;

    private:
        std::shared_ptr<ParseFunction> parser;
        ParserResult<T> result;

    public:
        Parser(ParseFunction parser) :
            parser(std::make_shared<ParseFunction>(parser))
        { }

        // Top level parser execution, parses a string
        ParserResult<T> parse(const std::string& input) const {
            InputStream view = { input };
            return (*parser)(view);
        }

        // Top level parser execution, parses a string_view
        // Parser consumes + modifies string_view
        ParserResult<T> parse(std::string_view& input) const {
            InputStream view = { input };
            auto result = (*parser)(view);
            input = *view;
            return result;
        }

        //// Top level parser execution, parses an optional<string>
        //ParserResult<T> parse(const std::optional<std::string>& input) const {
        //    if (!input) {
        //        return std::nullopt;
        //    }

        //    InputStream view = { *input };
        //    return (*parser)(view);
        //}

        // Executes the parser, modifying InputStream
        // Implementation only
        ParserResult<T> parse(InputStream& input) const {
            // return (*parser)(input);
            return input ? (*parser)(input) : std::nullopt;
        }

        // Parses self and other, returns result of other
        template<typename U>
        Parser<U> with(Parser<U> other) const {
            return Parser<U>([=, captureParser = parser](InputStream& input) -> ParserResult<U> {
                if (ParserResult<T> result = (*captureParser)(input)) {
                    return other.parse(input);
                }
                return std::nullopt;
                });
        }

        // Parses self and other, returns result of self
        template<typename U>
        Parser<T> skip(Parser<U> other) const {
            return Parser<T>([=, captureParser = parser](InputStream& input) -> ParserResult<T> {
                ParserResult result = (*captureParser)(input);
                if (result && other.parse(input)) {
                    return result;
                }
                return std::nullopt;
                });
        }

        // Parses self and other, returns pair of both results
        template<typename U>
        Parser<std::pair<T, U>> pair_with(Parser<U> other) const {
            return *this & other;
        }

        // Parses occurence satisfying a condition
        Parser<T> satisfy(std::function<bool(T)> cond) const {
            return Parser<T>([=, captureParser = parser](InputStream& input) -> ParserResult<T> {
                auto result = (*captureParser)(input);
                if (result && cond(*result)) {
                    return result;
                }
                return std::nullopt;
            });
        }
        // Parse occurence between two parses
        template<typename O, typename C>
        Parser<T> between(Parser<O> open, Parser<C> close) const {
            return open.with(*this).skip(close);
            //return Parser<T>([=](InputStream& input) -> ParserResult<T> {
            //    open.parse(input);
            //    ParserResult<T> middle = p.parse(input);
            //    close.parse(input);

            //    CPPARSEC_RETURN(middle);
            //});
        }

        // | "or" operator parses the left parser, then the right parser if the left one fails without consuming
        template <typename T>
        Parser<T> or_(const Parser<T>& right) const {
            return Parser<T>([=, captureParser = parser](InputStream& input) -> ParserResult<T> {
                auto starting_input = input;
                if (auto result = (*captureParser)(input)) {
                    return result;
                }
                if (starting_input->data() != input->data()) {
                    return std::nullopt;
                }

                return right.parse(input);
                });
        }

        Parser<T> try_() {
            return Parser<T>([=, captureParser = parser](InputStream& input) -> ParserResult<T> {
                auto starting_input = input;
                if (ParserResult result = (*captureParser)(input)) {
                    return result;
                }

                input = starting_input; // undo input consumption
                return std::nullopt;
            });
        }

        // Apply a function to the parse result
        template <typename U>
        Parser<U> transform(std::function<U(T)> func) const {
            return Parser<U>([=, captureParser = parser](InputStream& input) -> ParserResult<U> {
                ParserResult<T> result = (*captureParser)(input);
                return result.transform(func);
            });
        }
    };

    namespace helper {
        template <typename T> 
        std::vector<T>& many_accumulator(std::vector<T>& vec, Parser<T> p) {
            return Parser<std::vector<T>>([=](InputStream& input) -> ParserResult<std::vector<T>> {
                while (true) {
                    auto starting_point = input->data();
                    if (ParserResult<T> result = p.parse(input)) {
                        vec.push_back(*result);
                        continue;
                    }
                    // consumptive fail, stop parsing
                    CPPARSEC_FAIL_IF(starting_point != input->data());
                    break;
                }

                return vec;
                });
        }
    };

    // << "ignore" operator returns the first result of two parsers
    // a << b is a.skip(b) 
    template <typename T, typename U>
    Parser<T> operator<<(const Parser<T> left, const Parser<U> right);

    // >> "sequence" operator returns the second result of two parsers
    // a >> b is a.with(b) 
    template <typename T, typename U>
    Parser<U> operator>>(const Parser<T> left, const Parser<U> right);

    // & "and" operator joins two parsers 
    template <typename T, typename U>
    Parser<std::tuple<T, U>> operator&(const Parser<T> left, const Parser<U> right);

    // & "and" operator joins a parse and multiple parses into a flat tuple
    template<typename T, typename... Ts>
    Parser<std::tuple<T, Ts...>> operator&(const Parser<T>& left, const Parser<std::tuple<Ts...>>& right);

    // & "and" operator joins multiple parses and a parse into a flat tuple
    template<typename T, typename... Ts>
    Parser<std::tuple<T, Ts...>> operator&(const Parser<std::tuple<Ts...>>& left, const Parser<T>& right);

    // & "and" operator joins multiple parses and multiple parses into a flat tuple
    template<typename... Ts, typename... Us>
    Parser<std::tuple<Ts..., Us...>> operator&(const Parser<std::tuple<Ts...>>& left, const Parser<std::tuple<Us...>>& right);

    // Parses a given parser, doesn't consume input on failure
    // Commonly used with | "or" operator
    template <typename T>
    Parser<T> try_(Parser<T> p);

    // Parses an integer32
    Parser<int> int_();

    // Parses any character
    Parser<char> any_char();

    
    // Parse zero or more characters, std::string specialization
    Parser<std::string> many1(Parser<char> charP);

    // Parse one or more characters, std::string specialization
    Parser<std::string> many(Parser<char> charP);

    template<typename T>
    Parser<T> satisfy(const Parser<T>& parser, std::function<bool(T)> cond) {
        return parser.satisfy(cond);
    }

    // Parses a single string
    Parser<std::string> string_(const std::string& str) {
        return Parser<std::string>([=](InputStream& input) -> ParserResult<std::string> {
            if (input->substr(0, str.size()) == str) {
                return { str };
            }

            return std::nullopt;
            });
    }

    // Parses any character
    Parser<char> any_char() {
        return Parser<char>([](InputStream& input) -> ParserResult<char> {
            if (!input->empty()) {
                char c = (*input)[0];
                input->remove_prefix(1);
                return { c };
            }

            return std::nullopt;
            });
    }

    // Parses a single character
    Parser<char> char_(char c) {
        return Parser<char>([=](InputStream& input) -> ParserResult<char> {
            if (!input->empty() && (*input)[0] == c) {
                input->remove_prefix(1);
                return { c };
            }

            return std::nullopt;
            });

        //return any_char().satisfy([=](auto c2) { return c == c2; });

    }
    // Parses a single letter
    Parser<char> letter() {
        return try_(any_char().satisfy(isalpha));
    }

    // Parses a single digit
    Parser<char> digit() {
        return try_(any_char().satisfy(isdigit));
    }

    // Parses a single space
    Parser<char> space() {
        return try_(any_char().satisfy(isspace));
    }

    // Parses zero or more spaces
    Parser<std::string> spaces() {
        return many(space());
    }

    // Parses one or more spaces
    Parser<std::string> spaces1() {
        return many1(space());
    }

    // Parses a single newline '\n'
    Parser<char> newline() {
        return char_('\n');
    }

    // Parses a single uppercase letter 
    Parser<char> upper() {
        return try_(any_char().satisfy(isupper));
    }

    // Parses a single lowercase letter 
    Parser<char> lower() {
        return try_(any_char().satisfy(islower));
    }

    // Parses a single alphanumeric letter 
    Parser<char> alpha_num() {
        return try_(any_char().satisfy(isalnum));
    }

    namespace helper {
        //template <typename T>
        //void many_accumulator(std::vector<T>& vec, Parser<T> p, InputStream& input) {
        //    while (true) {
        //        auto starting_point = input->data();
        //        if (ParserResult result = p.parse(input)) {
        //            values.push_back(*result);
        //            continue;
        //        }
        //        if (starting_point != input->data()) {
        //            return std::nullopt; // consumptive fail, stop parsing
        //        }
        //        break;
        //    }
        //}
    }


    // Parse zero or more parses
    template<typename T>
    Parser<std::vector<T>> many(Parser<T> p) {
        return Parser<std::vector<T>>([=](InputStream& input) -> ParserResult<std::vector<T>> {
            std::vector<T> values;
            while (true) {
                auto starting_point = input->data();
                if (ParserResult result = p.parse(input)) {
                    values.push_back(*result);
                    continue;
                }
                // consumptive fail, stop parsing
                CPPARSEC_FAIL_IF(starting_point != input->data());
                break;
            }

            return ( values );
            });
    }

    // Parse one or more parses
    template<typename T>
    Parser<std::vector<T>> many1(Parser<T> p) {
        return Parser<std::vector<T>>([=](InputStream& input) -> ParserResult<std::vector<T>> {
            std::vector<T> values;
            CPPARSEC_SAVE(result1, p);
            values.push_back(result1);

            while (true) {
                auto starting_point = input->data();
                if (auto result = p.parse(input)) {
                    values.push_back(*result);
                    continue;
                }
                // consumptive fail, stop parsing
                CPPARSEC_FAIL_IF(starting_point != input->data());
                break;
            }
            return values;
            });
    }

    // Parse zero or more characters, std::string specialization
    Parser<std::string> many(Parser<char> charP) {
        return Parser<std::string>([=](InputStream& input) -> ParserResult<std::string> {
            std::string str;
            while (true) {
                auto starting_point = input->data();
                if (ParserResult<char> result = charP.parse(input)) {
                    str.push_back(*result);
                    continue;
                }
                // consumptive fail, stop parsing
                CPPARSEC_FAIL_IF(starting_point != input->data());
                break;
            }
            return str;
            });
    }

    // Parse one or more characters, std::string specialization
    Parser<std::string> many1(Parser<char> charP) {
        return Parser<std::string>([=](InputStream& input) -> ParserResult<std::string> {
            std::string str;
            CPPARSEC_SAVE(c1, charP);
            str.push_back(c1);

            while (true) {
                auto starting_point = input->data();
                if (auto c2 = charP.parse(input)) {
                    str.push_back(*c2);
                    continue;
                }
                // consumptive fail, stop parsing
                CPPARSEC_FAIL_IF(starting_point != input->data());
                break;
            }
            return str;
            })
            ;
    }

    // Parse zero or more parses, ignoring the results
    template<typename T>
    Parser<std::monostate> skipMany(Parser<T> p) {
        return Parser<std::monostate>([p](InputStream& input) -> ParserResult<std::monostate> {
            while (true) {
                auto starting_point = input->data(); // Keep track of input to detect no consumption
                if (!p.parse(input)) {
                    CPPARSEC_FAIL_IF(starting_point != input->data());
                    break;
                }
            }
            return std::monostate{}; // Return placeholder for success
            });
    }

    // Parse one or more parses, ignoring the results
    template<typename T>
    Parser<std::monostate> skipMany1(Parser<T> p) {
        return p.with(skipMany(p));
    }


    // Parses given number of parses
    template<typename T>
    Parser<std::vector<T>> count(int n, Parser<T> p) {
        return Parser<std::vector<T>>([=](InputStream& input) -> ParserResult<std::vector<T>> {
            std::vector<T> vec(n);

            for (int i = 0; i < n; i++) {
                auto starting_point = input->data();
                if (ParserResult<T> result = p.parse(input)) {
                    vec[i] = (*result);
                    continue;
                }
                // consumptive fail, stop parsing
                CPPARSEC_FAIL_IF(starting_point != input->data());
                break;
            }

            return vec;
        });
    }

    // Parse occurence between two parses
    // between(open, close, p) is open.with(p).skip(close) is open >> p << close
    template<typename O, typename C, typename T>
    Parser<T> between(Parser<O> open, Parser<C> close, Parser<T> p) {
        return open >> p << close;
        //return Parser<T>([=](InputStream& input) -> ParserResult<T> {
        //    CPPARSEC_SKIP(open);
        //    CPPARSEC_SAVE(middle, p);
        //    CPPARSEC_SKIP(close);

        //    CPPARSEC_RETURN(middle);
        //});
    }

    Parser<std::pair<int, std::string>> cubeParser() {
        return Parser<std::pair<int, std::string>>([](InputStream& input) -> ParserResult<std::pair<int, std::string>> {
            CPPARSEC_SAVE(cubeNum, int_());
            CPPARSEC_SKIP(space());
            CPPARSEC_SAVE(cubeColor, string_("red"));

            return std::pair{ cubeNum, cubeColor };
        });
    }

    // Parses a given parser, doesn't consume input on failure
    // Commonly used with | "or" operator
    template <typename T>
    Parser<T> try_(Parser<T> p) {
        return p.try_();
    }

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
        return many(digit()).transform<int>(helper::stoi);
    }

    // << "ignore" operator returns the first result of two parsers
    // a << b is a.skip(b) 
    template <typename T, typename U>
    Parser<T> operator<<(const Parser<T> left, const Parser<U> right) {
        return left.skip(right);
    }

    // >> "sequence" operator returns the second result of two parsers
    // a >> b is a.with(b) 
    template <typename T, typename U>
    Parser<U> operator>>(const Parser<T> left, const Parser<U> right) {
        return left.with(right);
    }

    // & "and" operator joins two parses
    template <typename T, typename U>
    Parser<std::tuple<T, U>> operator&(const Parser<T> left, const Parser<U> right) {
        return Parser<std::tuple<T, U>>([=](InputStream& input) -> ParserResult<std::tuple<T, U>> {
            CPPARSEC_SAVE(a, left);
            CPPARSEC_SAVE(b, right);

            return { std::make_tuple(a, b) };
        });
    }

    // | "or" operator parses the left parser, then the right parser if the left one fails without consuming
    template <typename T>
    Parser<T> operator|(const Parser<T>& left, const Parser<T>& right) {
        return left.or_(right);
    }

    // & "and" operator joins a parse and multiple parses
    template<typename T, typename... Ts>
    Parser<std::tuple<T, Ts...>> operator&(const Parser<T>& left, const Parser<std::tuple<Ts...>>& right) {
        return Parser<std::tuple<T, Ts...>>([=](InputStream& input) -> ParserResult<std::tuple<T, Ts...>> {
            CPPARSEC_SAVE(a, left);
            CPPARSEC_SAVE(bs, right);

            return { std::tuple_cat(std::make_tuple(a), bs) };
        });
    }

    // & "and" operator joins multiple parses and a parse 
    template<typename T, typename... Ts>
    Parser<std::tuple<T, Ts...>> operator&(const Parser<std::tuple<Ts...>>& left, const Parser<T>& right) {
        return Parser<std::tuple<T, Ts...>>([=](InputStream& input) -> ParserResult<std::tuple<T, Ts...>> {
            CPPARSEC_SAVE(as, left);
            CPPARSEC_SAVE(b, right);

            return { std::tuple_cat(as, std::make_tuple(b)) };
        });
    }

    // & "and" operator joins multiple parses and multiple parses
    template<typename... Ts, typename... Us>
    Parser<std::tuple<Ts..., Us...>> operator&(const Parser<std::tuple<Ts...>>& left, const Parser<std::tuple<Us...>>& right) {
        return Parser<std::tuple<Ts..., Us...>>([=](InputStream& input) -> ParserResult<std::tuple<Ts..., Us...>> {
            CPPARSEC_SAVE(as, left);
            CPPARSEC_SAVE(bs, right);

            return { std::tuple_cat(as,bs) };
        });
    }

};

/*
    ============ NUMERIC PROCESSORS ============
*/
using namespace cpparsec;
// Parse occurence between two parses - short circuit abuse

//// Parser<T> is a function that takes a string
//// and returns a ParserResult<T>
//template<typename T>
//using Parser = std::function<ParserResult<T> (const std::string&)>;
//
///*
//    ============ TEXT PROCESSORS ============
//*/
//
//// Parse a character
//Parser<char> character(char c);
//
//// Parse a string
//Parser<std::string> stringChar(const std::string& str);
//
//
///*
//    ============ PARSER COMBINATORS ============
//*/
//
//
//// Parse zero or more occurrences
//template<typename T> 
//Parser<std::vector<T>> many(Parser<T> p);
//
//// Parse one or more occurrences
//template<typename T> 
//Parser<std::vector<T>> many1(Parser<T> p);
//
//// Parse occurence between two parses
//template<typename O, typename C, typename T>
//Parser<T> between(Parser<O> open, Parser<C> close, Parser<T> p);
//
///*
//    ============ TEXT PROCESSORS DEFINITIONS ============
//*/
//
//// Parse a character
//Parser<char> character(char c) {
//    return [c](const std::string& input) -> ParserResult<char> {
//        if (!input.empty() && input[0] == c) {
//            return { {c, input.substr(1)} };
//        }
//        return std::nullopt; // Parsing failure
//    };
//}
//
//// Parse a string
//Parser<std::string> parseString(const std::string& str) {
//    return [str](const std::string& input) -> ParserResult<std::string> {
//        if (input.substr(0, str.size()) == str) {
//            return { {str, input.substr(str.size())} };
//        }
//        return std::nullopt; // Parsing failure
//    };
//}
//
///*
//    ============ PARSER COMBINATORS DEFINITIONS ============
//*/

#endif /* CPPARSEC_H */
