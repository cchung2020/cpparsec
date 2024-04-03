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
#include <concepts>
#include <algorithm>
#include <utility>

template <typename C, typename T>
concept PushBack = requires (C c, T t, C c2) {
    { c.push_back(t) } -> std::same_as<void>;
    { c.push_back(std::move(t)) } -> std::same_as<void>;
};

#define CPPARSEC_RETURN(...) \
    return std::optional{ __VA_ARGS__ };
#define CPPARSEC_RETURN_NAMEDINPUT(input, val) \
    if (input) { return val; } return std::nullopt;


#define CPPARSEC_SAVE_NAMEDINPUT(var, p, input) \
    auto _##var = (p).parse(input); \
    if (!_##var.has_value()) {         \
        return std::nullopt;      \
    }                             \
    auto var = _##var.value();
#define CPPARSEC_SAVE(var, ...) \
    CPPARSEC_SAVE_NAMEDINPUT(var, (__VA_ARGS__), input);

#define CPPARSEC_SAVE_PARSER_NAMEDINPUT(var, p, input) \
    auto _##var = p(input);       \
    if (!_##var.has_value()) {    \
        return std::nullopt;      \
    }                             \
    auto var = _##var.value();
#define CPPARSEC_SAVE_PARSER(var, p) \
    CPPARSEC_SAVE_PARSER_NAMEDINPUT(var, p, input);

#define CPPARSEC_SKIP(p) \
    if (!(p).parse(input)) { return std::nullopt; }

#define CPPARSEC_FAIL_IF(cond) \
    if (cond) { return std::nullopt; }

#define CPPARSEC_IF_PARSE_OK(name, p) \
    if (auto name = (p).parse(input))

#define CPPARSEC_SAVE_INPUT_POINT(name) \
    auto name = input->data();

#define CPPARSEC_INPUT_POINT \
    input->data()

#define CPPARSEC_PARSERESULT(p) \
    p.parse(input)

#define CPPARSEC_RESULT(name) *name

#define CPPARSEC_DEFN(...) \
    _ParserFactory<__VA_ARGS__>() = [=](InputStream& input) -> ParserResult<__VA_ARGS__>

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

        // Executes the parser, modifying InputStream
        // Implementation only
        ParserResult<T> parse(InputStream& input) const {
            // return (*parser)(input);
            return input ? (*parser)(input) : std::nullopt;
        }

        // Parses self and other, returns result of other
        template<typename U>
        Parser<U> with(Parser<U> other) const {
            return Parser<U>([=, selfParser = *this](InputStream& input) -> ParserResult<U> {
                CPPARSEC_SKIP(selfParser);
                CPPARSEC_SAVE(result, other)

                return result;
                });
        }

        // Parses self and other, returns result of self
        template<typename U>
        Parser<T> skip(Parser<U> other) const {
            return Parser<T>([=, selfParser = *this](InputStream& input) -> ParserResult<T> {
                CPPARSEC_SAVE(result, selfParser)
                CPPARSEC_SKIP(other);

                return result;
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
                CPPARSEC_FAIL_IF(!result || !cond(*result));
                
                return result;
            });
        }
        // Parse occurence between two parses
        template<typename O, typename C>
        Parser<T> between(Parser<O> open, Parser<C> close) const {
            return open.with(*this).skip(close);
        }

        // | "or" operator parses the left parser, then the right parser if the left one fails without consuming
        template <typename T>
        Parser<T> or_(const Parser<T>& right) const {
            return Parser<T>([=, captureParser = parser](InputStream& input) -> ParserResult<T> {
                auto starting_input = input;
                if (auto result = (*captureParser)(input)) {
                    return result;
                }
                CPPARSEC_FAIL_IF(starting_input->data() != input->data())

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
                CPPARSEC_FAIL_IF(true);
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

    template <typename T>
    class _ParserFactory {
    public:
        Parser<T> operator=(Parser<T>::ParseFunction parser) {
            return Parser<T>(parser);
        }
    };

    namespace helper {
        template <typename T, typename Container = std::vector<T>>
            requires PushBack<Container, T> && std::movable<Container> 
        Parser<Container> many_accumulator(Parser<T> p, Container&& init = {}) {
            return CPPARSEC_DEFN(Container) {
                Container values(init);
                while (true) {
                    auto starting_point = CPPARSEC_INPUT_POINT;
                    if (ParserResult<T> result = p.parse(input)) {
                        values.push_back(*result);
                        continue;
                    }
                    // consumptive fail, stop parsing
                    CPPARSEC_FAIL_IF(starting_point != CPPARSEC_INPUT_POINT);
                    break;
                }

                return values;
            };
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
            for (auto c : str) {
                CPPARSEC_SAVE(c2, any_char());
                CPPARSEC_FAIL_IF(c != c2);
            }
            //CPPARSEC_FAIL_IF(input->substr(0, str.size()) != str);
            // non consuming fail model

            return str;
            });
    }

    // Parses any character
    Parser<char> any_char() {
        return Parser<char>([](InputStream& input) -> ParserResult<char> {
            CPPARSEC_FAIL_IF(input->empty());

            char c = (*input)[0];
            input->remove_prefix(1);
            return c;
            });
    }

    // Parses a single character
    Parser<char> char_(char c) {
        return CPPARSEC_DEFN(char) {
            CPPARSEC_FAIL_IF(input->empty() || (*input)[0] != c);

            input->remove_prefix(1);
            return c;
        };

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

    // Parses successfully only if the input is empty
    Parser<std::monostate> eof() {
        return CPPARSEC_DEFN(std::monostate) {
            CPPARSEC_FAIL_IF(input->size() > 0);
            return std::monostate{};
        };
    }


    // Never consumes input and always succeeds, returns the value given. 
    template <typename T>
    Parser<T> success(T item) {
        return CPPARSEC_DEFN(T) {
            return item;
        };
    }

    // Parse zero or more parses
    template<typename T>
    Parser<std::vector<T>> many(Parser<T> p) {
        return helper::many_accumulator<T>(p);
    }

    // Parse one or more parses
    template<typename T>
    Parser<std::vector<T>> many1(Parser<T> p) {
        return CPPARSEC_DEFN(std::vector<T>) {
            CPPARSEC_SAVE(first, p);
            CPPARSEC_SAVE(values, helper::many_accumulator(p, { first }));

            return values;
        };
    }

    // Parse zero or more characters, std::string specialization
    Parser<std::string> many(Parser<char> charP) {
        return helper::many_accumulator(charP, std::string());
    }

    // Parse one or more characters, std::string specialization
    Parser<std::string> many1(Parser<char> charP) {
        return CPPARSEC_DEFN(std::string) {
            CPPARSEC_SAVE(first, charP);
            CPPARSEC_SAVE(values, helper::many_accumulator(charP, std::string(1, first)));

            return values;
        };
    }

    // Parses p, ignoring the result
    template<typename T>
    Parser<std::monostate> skip(Parser<T> p) {
        return p >> success(std::monostate{});
    }

    // Parses for an optional p, succeeds if p fails without consuming
    template<typename T>
    Parser<std::monostate> optional_(Parser<T> p) {
        return skip(p) | success(std::monostate{});
    }

    // Parses for an optional p, succeeds if p fails without consuming
    template<typename T>
    Parser<std::optional<T>> optional_result(Parser<T> p) {
        return CPPARSEC_DEFN(std::optional<T>) {
            CPPARSEC_SAVE_INPUT_POINT(starting_point);
            ParserResult<T> result = CPPARSEC_PARSERESULT(p);
            CPPARSEC_FAIL_IF(!result && starting_point != CPPARSEC_INPUT_POINT);

            return std::optional(result ? result : std::nullopt);
        };
        // WIP
        //return p.transform([](auto r) { return std::optional(r); }) | std::nullopt;
    }

    // Parse zero or more parses, ignoring the results
    template<typename T>
    Parser<std::monostate> skipMany(Parser<T> p) {
        return CPPARSEC_DEFN(std::monostate) {
            while (true) {
                CPPARSEC_SAVE_INPUT_POINT(starting_point);
                if (!CPPARSEC_PARSERESULT(p)) {
                    CPPARSEC_FAIL_IF(starting_point != CPPARSEC_INPUT_POINT);
                    break;
                }
            }
            return std::monostate{}; // Return placeholder for success
        };
    }

    // Parse one or more parses, ignoring the results
    template<typename T>
    Parser<std::monostate> skipMany1(Parser<T> p) {
        return p >> skipMany(p);
    }

    // Parse one or more parses of p separated by sep
    template <typename T, typename U>
    Parser<std::vector<T>> sepBy1(Parser<T> p, Parser<U> sep) {
        return CPPARSEC_DEFN(std::vector<T>) {
            CPPARSEC_SAVE(first, p);
            CPPARSEC_SAVE(values, helper::many_accumulator(sep >> p, { first }));

            return values;
        };
    }

    // Parse zero or more parses of p separated by sep
    template <typename T, typename U>
    Parser<std::vector<T>> sepBy(Parser<T> p, Parser<U> sep) {
        return sepBy1(p, sep) | success(std::vector<T>());
    }

    // Parse one or more parses of p separated by sep, std::string specialization
    template <typename T>
    Parser<std::string> sepBy1(Parser<char> p, Parser<T> sep) {
        return CPPARSEC_DEFN(std::string) {
            CPPARSEC_SAVE(first, p);
            CPPARSEC_SAVE(values, helper::many_accumulator(sep >> p, std::string(1, first)));

            return values;
        };
    }

    // Parse zero or more parses of p separated by sep, std::string specialization
    template <typename T>
    Parser<std::string> sepBy(Parser<char> p, Parser<T> sep) {
        return sepBy1(p, sep) | success("");
    }

    // Parse zero or more parses of p separated by and ending with sep
    template <typename T, typename U>
    Parser<std::vector<T>> endBy(Parser<T> p, Parser<U> sep) {
        return many(p << sep);
    }

    // Parse one or more parses of p separated by and ending with sep
    template <typename T, typename U>
    Parser<std::vector<T>> endBy1(Parser<T> p, Parser<U> sep) {
        return many1(p << sep);
    }

    // Parse zero or more parses of p separated by and ending with sep, std::string specialization
    template <typename T>
    Parser<std::string> endBy(Parser<char> p, Parser<T> sep) {
        return many(p << sep);
    }

    // Parse one or more parses of p separated by and ending with sep, std::string specialization
    template <typename T>
    Parser<std::string> endBy1(Parser<char> p, Parser<T> sep) {
        return many1(p << sep);
    }

    // Parses given number of parses
    template<typename T>
    Parser<std::vector<T>> count(int n, Parser<T> p) {
        return CPPARSEC_DEFN(std::vector<T>) {
            std::vector<T> vec(n);

            for (int i = 0; i < n; i++) {
                auto starting_point = CPPARSEC_INPUT_POINT;
                if (ParserResult<T> result = CPPARSEC_PARSERESULT(p)) {
                    vec[i] = (*result);
                    continue;
                }
                // consumptive fail, stop parsing
                CPPARSEC_FAIL_IF(starting_point != CPPARSEC_INPUT_POINT);
                break;
            }

            return vec;
        };
    }

    // Parse occurence between two parses
    // between(open, close, p) is open.with(p).skip(close) is open >> p << close
    template<typename O, typename C, typename T>
    Parser<T> between(Parser<O> open, Parser<C> close, Parser<T> p) {
        //return open >> p << close;
        //DEFN(T)
        return CPPARSEC_DEFN(T) {
            CPPARSEC_SKIP(open);
            CPPARSEC_SAVE(middle, p);
            CPPARSEC_SKIP(close);
            CPPARSEC_RETURN(middle);
        };
    }

    // return Parser<T>() = [](InputStream& input) -> ParserResult<T> ;
    Parser<std::pair<int, std::string>> cubeParser() {
        return CPPARSEC_DEFN(std::pair<int, std::string>) {
            CPPARSEC_SAVE(cubeNum, int_() << space());
            CPPARSEC_SAVE(cubeColor, string_("red"));

            return std::pair{ cubeNum, cubeColor };
        };
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
        return many1(digit()).transform<int>(helper::stoi);
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

///*
//    ============ TEXT PROCESSORS ============
//*/

//
///*
//    ============ PARSER COMBINATORS ============
//*/

///*
//    ============ TEXT PROCESSORS DEFINITIONS ============
//*/

///*
//    ============ PARSER COMBINATORS DEFINITIONS ============
//*/

#endif /* CPPARSEC_H */
