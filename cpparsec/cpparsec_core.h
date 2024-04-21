#ifndef CPPARSEC_CORE_H
#define CPPARSEC_CORE_H

#include <functional>
#include <optional>
#include <expected>
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
#include <variant>
#include <utility>

// Initialize a variable with a parse result value
// Automatically returns if the parser fails
#define CPPARSEC_SAVE(var, ...)                               \
    auto&& _##var##_ = (__VA_ARGS__).parse(input);            \
    if (!_##var##_.has_value()) {                             \
        return std::unexpected(std::move(_##var##_.error())); \
    }                                                         \
    auto&& var = _##var##_.value();

// Parse a parser without assigning the result value
// Automatically returns if the parser fails
#define CPPARSEC_SKIP(p) \
    if (auto&& _cpparsec_skipresult = (p).parse(input); !_cpparsec_skipresult) { \
        return std::unexpected(std::move(_cpparsec_skipresult.error()));         \
    }                                                                            \

// Automatically returns ParseError if the condition is true 
#define CPPARSEC_FAIL_IF(cond, message) if (cond) { return std::unexpected([=]() { return message; }); }

// Automatically returns ParseError
#define CPPARSEC_FAIL(message) return std::unexpected([=]() { return message; });

#define CPPARSEC_GET_INPUT input
#define CPPARSEC_GET_INPUT_DATA input.data()
#define CPPARSEC_SET_INPUT(new_input) input = new_input;

#define CPPARSEC_DEFN(...) \
    cpparsec::_ParserFactory<__VA_ARGS__>() = [=](cpparsec::InputStream& input) -> cpparsec::ParseResult<__VA_ARGS__>

#define CPPARSEC_DEFN_METHOD(name, ...) \
    cpparsec::_ParserFactory<__VA_ARGS__>() = [=, name = *this](cpparsec::InputStream& input) -> cpparsec::ParseResult<__VA_ARGS__>

using std::println, std::format, std::string, std::string_view;

namespace cpparsec {
    // ========================================================================
    // 
    // ======================= HEADERS AND DECLARATIONS =======================
    // 
    // ========================================================================

    struct ErrorContent
        : std::variant<std::pair<std::string, std::string>, std::pair<char, char>, std::string, std::monostate>
    { };

    class ParseError {
    private:
        std::vector<ErrorContent> errors;

    public:
        ParseError(ErrorContent&& err);
        ParseError(const std::string expected, const std::string found);
        ParseError(char expected, char found);
        ParseError(std::string&& msg);

        // Add error to error container
        ParseError& add_error(ErrorContent&& err);

        // Returns deepest error message as a std::string
        std::string message();

        // Returns shallowest error message as a std::string
        std::string message_top();

        // Returns all error messages as a std::string
        std::string message_stack();
    };

    template<typename T>
    using ParseResult = std::expected<T, std::function<ParseError()>>;
    using InputStream = std::string_view;

    template<typename T>
    class Parser {
    public:
        using ParseFunction = std::function<ParseResult<T>(InputStream&)>; // ParseResult<T>(*)(InputStream&)

    private:
        ParseFunction parser;

    public:
        // Implementation detail, Parsers take ParseFunctions
        // Use the CPPARSEC_DEFN macro for documented parsers
        Parser(ParseFunction&& parser);

        // Top level parser execution, parses a string
        ParseResult<T> parse(const std::string& input) const;

        // Top level parser execution, parses a string_view
        // Parser consumes/modifies string_view
        ParseResult<T> parse(std::string_view& input) const;

        // Parses self and other, returns result of other
        template<typename U>
        Parser<U> with(Parser<U> other) const;

        // Parses self and other, returns result of self
        template<typename U>
        Parser<T> skip(Parser<U> other) const;

        // Parses self and other, returns pair of both results
        template<typename U>
        Parser<std::pair<T, U>> pair_with(Parser<U> other) const;

        // Parses occurence satisfying a condition
        Parser<T> satisfy(std::function<bool(T)> cond) const;

        // Parse occurence between two parses
        template<typename O, typename C>
        Parser<T> between(Parser<O> open, Parser<C> close) const;

        // | "or" operator parses the left parser, then the right parser if the left one fails without consuming
        Parser<T> or_(const Parser<T>& right) const;

        // Parses p without consuming input on failure
        Parser<T> try_() const;

        // Apply a function to the parse result
        template <typename U>
        Parser<U> transform(std::function<U(T)> func) const;
    };

    // ============================ PARSER FACTORY ============================

    // Implementation detail, should never be invoked manually
    template <typename T>
    struct _ParserFactory {
        Parser<T> operator=(Parser<T>::ParseFunction&& parser) {
            return Parser<T>(std::move(parser));
        }
    };

    // =============================== CONCEPTS ===============================

    // Concept for a container that can call push_back
    template <typename C, typename T>
    concept PushBack = requires (C c, T t) {
        { c.push_back(t) } -> std::same_as<void>;
        { c.push_back(std::move(t)) } -> std::same_as<void>;
    };

    // Concept for a unary predicate function
    template<typename Pred, typename Arg>
    concept UnaryPredicate = requires(Pred f, Arg a) {
        { f(a) } -> std::convertible_to<bool>;
    };

    // ======================= CORE PARSER COMBINATORS ========================

    // Parses given number of parses
    template<typename T>
    Parser<std::vector<T>> count(int n, Parser<T> p);

    // Parses a sequence of functions, returning on the first successful result.
    template<typename T, std::ranges::input_range Container>
    Parser<T> choice(Container parsers);

    // Parse occurence between two parses
    // between(open, close, p) is open.with(p).skip(close) is open >> p << close
    template<typename O, typename C, typename T>
    Parser<T> between(Parser<O> open, Parser<C> close, Parser<T> p);

    // between 2
    template<typename O, typename C, typename T>
    Parser<T> between2(Parser<O> open, Parser<C> close, Parser<T> p);

    // | "or" parses the left parser, then the right parser if the left one fails without consuming
    template <typename T>
    Parser<T> or_(const Parser<T>& left, const Parser<T>& right);

    // Parses a given parser, doesn't consume input on failure
    // Commonly used with | "or" operator
    template <typename T>
    Parser<T> try_(Parser<T> p);

    // Parses p without consuming input. If p fails, it will consume input. Wrap p with try_ to avoid this.
    template <typename T>
    Parser<T> look_ahead(Parser<T> p);

    // Succeeds only if p fails to parse. Never consumes input.
    template <typename T>
    Parser<std::monostate> not_followed_by(Parser<T> p);

    // Parses p if p passes a condition, failing if it doesn't
    template<typename T>
    Parser<T> satisfy(const Parser<T>& parser, std::function<bool(T)> cond);

    // Never consumes input and always succeeds, returns the value given. 
    template <typename T>
    Parser<T> success(T item);

    // A parser which always fails without consuming input
    template <typename T>
    Parser<T> unexpected();

    // Parses successfully only if the input is empty
    template <typename T>
    Parser<std::monostate> eof();

    // Parses p, ignoring the result
    // Does not improve performance (return types are not lazy)
    template<typename T>
    Parser<std::monostate> skip(Parser<T> p);

    // Parses for an optional p, succeeds if p fails without consuming
    template<typename T>
    Parser<std::monostate> optional_(Parser<T> p);

    // Parses for an optional p, succeeds if p fails without consuming
    template<typename T>
    Parser<std::optional<T>> optional_result(Parser<T> p);

    // Parse zero or more parses
    template<typename T, PushBack<T> Container>
    Parser<Container> many(Parser<T> p);

    // Parse one or more parses
    template<typename T, PushBack<T> Container>
    Parser<Container> many1(Parser<T> p);

    // Parse zero or more characters, std::string specialization
    Parser<std::string> many(Parser<char> charP);

    // Parse one or more characters, std::string specialization
    Parser<std::string> many1(Parser<char> charP);

    // Parses p zero or more times until end succeeds, returning the parsed values
    template <typename T, typename U, PushBack<T> Container>
    Parser<Container> many_till(Parser<T> p, Parser<U> end);

    // Parses p one or more times until end succeeds, returning the parsed values
    template <typename T, typename U, PushBack<T> Container>
    Parser<Container> many1_till(Parser<T> p, Parser<U> end);

    // Parses p zero or more times until end succeeds, returning the parsed values, std::string specialization
    template <typename T, PushBack<char> StringContainer>
    Parser<StringContainer> many_till(Parser<char> p, Parser<T> end);

    // Parses p one or more times until end succeeds, returning the parsed values, std::string specialization
    template <typename T, PushBack<char> StringContainer>
    Parser<StringContainer> many1_till(Parser<char> p, Parser<T> end);

    // Parses zero or more instances of p, ignores results
    template <typename T>
    Parser<std::monostate> skip_many(Parser<T> p);

    // Parses one or more instances of p, ignores results
    template <typename T>
    Parser<std::monostate> skip_many1(Parser<T> p);

    // Parse zero or more parses of p separated by sep
    template <typename T, typename U>
    Parser<std::vector<T>> sep_by(Parser<T> p, Parser<U> sep);

    // Parse one or more parses of p separated by sep
    template <typename T, typename U>
    Parser<std::vector<T>> sep_by1(Parser<T> p, Parser<U> sep);

    // Parse zero or more parses of p separated by sep, std::string specialization
    template <typename T>
    Parser<std::string> sep_by(Parser<char> p, Parser<T> sep);

    // Parse one or more parses of p separated by sep, std::string specialization
    template <typename T>
    Parser<std::string> sep_by1(Parser<char> p, Parser<T> sep);

    // Parse zero or more parses of p separated by and ending with sep
    template <typename T, typename U>
    Parser<std::vector<T>> end_by(Parser<T> p, Parser<U> sep);

    // Parse one or more parses of p separated by and ending with sep
    template <typename T, typename U>
    Parser<std::vector<T>> end_by1(Parser<T> p, Parser<U> sep);

    // Parse zero or more parses of p separated by and ending with sep, std::string specialization
    template <typename T>
    Parser<std::string> end_by(Parser<char> p, Parser<T> sep);

    // Parse one or more parses of p separated by and ending with sep, std::string specialization
    template <typename T>
    Parser<std::string> end_by1(Parser<char> p, Parser<T> sep);

    // Parse one or more left associative applications of op to p, returning the
    // result of the repeated applications. Can be used to parse 1+2+3+4 as ((1+2)+3)+4
    template <typename T>
    Parser<T> chainl1(Parser<T> arg, Parser<std::function<T(T, T)>> op);

    // Parse zero or more left associative applications of op to p, returning the
    // result of the repeated applications. If there are zero applications, return backup.
    template <typename T>
    Parser<T> chainl(Parser<T> arg, Parser<std::function<T(T, T)>> op, T backup);

    // Takes a std::function of a parser (not the parser itself) for deferred evaluation
    // Can be used to avoid infinite cycles in mutual recursion
    template<typename T>
    Parser<T> lazy(std::function<Parser<T>()> parser_func);

    // Takes a function pointer to a parser (not the parser itself) for deferred evaluation
    // Can be used to avoid infinite cycles in mutual recursion
    template<typename T>
    Parser<T> lazy(Parser<T>(*parser_func)());

    // =========================== OPERATORS ==================================

    // << "ignore" operator returns the first result of two parsers
    // a << b is a.skip(b) 
    template <typename T, typename U>
    Parser<T> operator<<(const Parser<T> left, const Parser<U> right);

    // >> "sequence" operator returns the second result of two parsers
    // a >> b is a.with(b) 
    template <typename T, typename U>
    Parser<U> operator>>(const Parser<T> left, const Parser<U> right);

    // | "or" operator parses the left parser, then the right parser if the left one fails without consuming
    template <typename T>
    Parser<T> operator|(const Parser<T>& left, const Parser<T>& right);

    // & "and" operator joins two parses
    template <typename T, typename U>
    Parser<std::tuple<T, U>> operator&(const Parser<T> left, const Parser<U> right);

    // & "and" operator joins a parse and multiple parses
    template<typename T, typename... Ts>
    Parser<std::tuple<T, Ts...>> operator&(const Parser<T>& left, const Parser<std::tuple<Ts...>>& right);

    // & "and" operator joins multiple parses and a parse 
    template<typename T, typename... Ts>
    Parser<std::tuple<T, Ts...>> operator&(const Parser<std::tuple<Ts...>>& left, const Parser<T>& right);

    // & "and" operator joins multiple parses and multiple parses
    template<typename... Ts, typename... Us>
    Parser<std::tuple<Ts..., Us...>> operator&(const Parser<std::tuple<Ts...>>& left, const Parser<std::tuple<Us...>>& right);

    // Add an error message to the parse result if it fails
    // Designed for debugging purposes, is slow
    template <typename T>
    Parser<T> operator^(Parser<T>&& p, std::string&& msg);

    // Replace an error message from the parse result if it fails
    // Designed for debugging purposes, is slow
    template <typename T>
    Parser<T> operator%(Parser<T> p, std::string&& msg);


    // ========================================================================
    // 
    // ======================= TEMPLATE IMPLEMENTATIONS =======================
    // 
    // ========================================================================


    // ================================ Parser ================================

    // Implementation detail, Parsers take ParseFunctions
    // Use the CPPARSEC_DEFN macro for documented parsers
    template <typename T>
    Parser<T>::Parser(ParseFunction&& parser) :
        parser(std::move(parser))
    { }

    // Top level parser execution, parses a string
    template <typename T>
    ParseResult<T> Parser<T>::parse(const std::string& input) const {
        InputStream view = input;
        return parser(view);
    }

    // Top level parser execution, parses a string_view
    // Parser consumes/modifies string_view
    template <typename T>
    ParseResult<T> Parser<T>::parse(std::string_view& input) const {
        InputStream view = input;
        auto result = parser(view);
        input = view;
        return result;
    }

    // Parses self and other, returns result of other
    template<typename T>
    template<typename U>
    Parser<U> Parser<T>::with(Parser<U> other) const {
        return CPPARSEC_DEFN_METHOD(thisParser, U) {
            CPPARSEC_SKIP(thisParser);
            CPPARSEC_SAVE(result, other);

            return result;
        };
    }

    // Parses self and other, returns result of self
    template<typename T>
    template<typename U>
    Parser<T> Parser<T>::skip(Parser<U> other) const {
        return CPPARSEC_DEFN_METHOD(thisParser, T) {
            CPPARSEC_SAVE(result, thisParser);
            CPPARSEC_SKIP(other);

            return result;
        };
    }

    // Parses self and other, returns pair of both results
    template<typename T>
    template<typename U>
    Parser<std::pair<T, U>> Parser<T>::pair_with(Parser<U> other) const {
        return CPPARSEC_DEFN_METHOD(thisParser, std::pair<T, U>) {
            CPPARSEC_SAVE(a, thisParser);
            CPPARSEC_SAVE(b, other);

            return std::pair(a, b);
        };
    }

    // Parses occurence satisfying a condition
    template<typename T>
    Parser<T> Parser<T>::satisfy(std::function<bool(T)> cond) const {
        return CPPARSEC_DEFN_METHOD(thisParser, T) {
            ParseResult<T> result = thisParser(input);
            CPPARSEC_FAIL_IF(!result || !cond(*result), format("Failed satisfy"));

            return result;
        };
    }

    // Parse occurence between two parses
    template<typename T>
    template<typename O, typename C>
    Parser<T> Parser<T>::between(Parser<O> open, Parser<C> close) const {
        return open.with(*this).skip(close);
    }

    // | "or" operator parses the left parser, then the right parser if the left one fails without consuming
    template <typename T>
    Parser<T> Parser<T>::or_(const Parser<T>& right) const {
        return CPPARSEC_DEFN_METHOD(thisParser, T) {
            auto starting_input = input;
            if (ParseResult<T> result = thisParser.parse(input)) {
                return result;
            }
            else if (starting_input.data() != input.data()) {
                return result;
            }

            return right.parse(input);
        };
    }

    // Parses p without consuming input on failure
    template<typename T>
    Parser<T> Parser<T>::try_() const {
        return CPPARSEC_DEFN_METHOD(thisParser, T) {
            auto starting_input = input;
            ParseResult<T> result = thisParser.parse(input);
            if (!result) {
                input = starting_input; // undo input consumption
            }

            return result;
        };
    }

    // Apply a function to the parse result
    template<typename T>
    template <typename U>
    Parser<U> Parser<T>::transform(std::function<U(T)> func) const {
        return CPPARSEC_DEFN_METHOD(thisParser, U) {
            ParseResult<T> result = thisParser.parse(input);
            return result.transform(func);
        };
    }

    // ======================= Core Parser Combinators ========================

    // Parses given number of parses
    template<typename T>
    Parser<std::vector<T>> count(int n, Parser<T> p) {
        return CPPARSEC_DEFN(std::vector<T>) {
            std::vector<T> vec(n);

            for (int i = 0; i < n; i++) {
                CPPARSEC_SAVE(val, p);
                vec[i] = val;
            }

            return vec;
        };
    }

    // Parses a sequence of functions, returning on the first successful result.
    template<typename T, std::ranges::input_range Container>
    Parser<T> choice(Container parsers) {
        if (std::ranges::empty(parsers)) {
            return unexpected<T>();
        }

        return std::ranges::fold_left_first(parsers, std::bit_or<>{}).value();
    }

    // Parse occurence between two parses
    // between(open, close, p) is open.with(p).skip(close) is open >> p << close
    template<typename O, typename C, typename T>
    Parser<T> between(Parser<O> open, Parser<C> close, Parser<T> p) {
        return open >> p << close;
    }

    // between 2
    template<typename O, typename C, typename T>
    Parser<T> between2(Parser<O> open, Parser<C> close, Parser<T> p) {
        //return open >> p << close;
        return CPPARSEC_DEFN(T) {
            CPPARSEC_SKIP(open);
            CPPARSEC_SAVE(middle, p);
            CPPARSEC_SKIP(close);

            return middle;
        };
    }

    // | "or" parses the left parser, then the right parser if the left one fails without consuming
    template <typename T>
    Parser<T> or_(const Parser<T>& left, const Parser<T>& right) {
        return left.or_(right);
    }

    // Parses a given parser, doesn't consume input on failure
    // Commonly used with | "or" operator
    template <typename T>
    Parser<T> try_(Parser<T> p) {
        return p.try_();
    }

    // Parses p without consuming input. If p fails, it will consume input. Wrap p with try_ to avoid this.
    template <typename T>
    Parser<T> look_ahead(Parser<T> p) {
        return CPPARSEC_DEFN(T) {
            auto input_copy = CPPARSEC_GET_INPUT;
            CPPARSEC_SAVE(value, p);
            CPPARSEC_SET_INPUT(input_copy);

            return value;
        };
    }

    // Succeeds only if p fails to parse. Never consumes input.
    template <typename T>
    Parser<std::monostate> not_followed_by(Parser<T> p) {
        return CPPARSEC_DEFN(std::monostate) {
            auto input_copy = CPPARSEC_GET_INPUT;
            ParseResult<T> result = p.parse(input);
            CPPARSEC_SET_INPUT(input_copy);
            CPPARSEC_FAIL_IF(result.has_value(), ParseError("not_followed_by", "not_followed_by"));

            return std::monostate{};
        };
    }

    // Parses p if p passes a condition, failing if it doesn't
    template<typename T>
    Parser<T> satisfy(const Parser<T>& parser, std::function<bool(T)> cond) {
        return CPPARSEC_DEFN_METHOD(thisParser, T) {
            ParseResult<T> result = thisParser(input);
            CPPARSEC_FAIL_IF(!result || !cond(*result), format("Failed satisfy"));

            return result;
        };
    }

    // Never consumes input and always succeeds, returns the value given. 
    template <typename T>
    Parser<T> success(T item) {
        return CPPARSEC_DEFN(T) {
            return item;
        };
    }

    // A parser which always fails without consuming input
    template <typename T = std::monostate>
    Parser<T> unexpected() {
        return CPPARSEC_DEFN(T) {
            CPPARSEC_FAIL(ParseError("unexpected"));
        };
    }

    // Parses successfully only if the input is empty
    template <typename T = std::monostate>
    Parser<std::monostate> eof() {
        return CPPARSEC_DEFN(T) {
            CPPARSEC_FAIL_IF(input.size() > 0, ParseError({ input[0] }, "end of input"));
            return T{};
        };
    }

    // Parses p, ignoring the result
    // Does not improve performance (return types are not lazy)
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
            auto start_point = CPPARSEC_GET_INPUT_DATA;
            ParseResult<T> result = p.parse(input);
            CPPARSEC_FAIL_IF(!result && start_point != CPPARSEC_GET_INPUT_DATA, ParseError("optional_result", "optional_result"));

            return (result ? std::optional(result.value()) : std::nullopt);
        };
        // WIP
        //return p.transform([](auto r) { return std::optional(r); }) | std::nullopt;
    }

    namespace helper {
        template <typename T, PushBack<T> Container = std::vector<T>>
            requires std::movable<Container>
        Parser<Container> many_accumulator(Parser<T> p, Container&& init = {}) {
            return CPPARSEC_DEFN(Container) {
                Container values(init);

                while (true) {
                    auto starting_point = CPPARSEC_GET_INPUT_DATA;
                    if (ParseResult<T> result = p.parse(input)) {
                        values.push_back(*result);
                        continue;
                    }
                    else {
                        // consumptive fail, stop parsing
                        if (starting_point != input.data()) {
                            return std::unexpected(std::move(result.error()));
                        }
                        break;
                    }
                }

                return values;
            };
        }
    };

    // Parse zero or more parses
    template<typename T, PushBack<T> Container = std::vector<T>>
    Parser<Container> many(Parser<T> p) {
        return helper::many_accumulator(p);
    }

    // Parse one or more parses
    template<typename T, PushBack<T> Container = std::vector<T>>
    Parser<Container> many1(Parser<T> p) {
        return CPPARSEC_DEFN(std::vector<T>) {
            CPPARSEC_SAVE(first, p);
            CPPARSEC_SAVE(values, helper::many_accumulator(p, { first }));

            return values;
        };
    }

    namespace helper {
        template <typename T, typename U, PushBack<T> Container = std::vector<T>>
            requires std::movable<Container>
        Parser<Container> many_till_accumulator(Parser<T> p, Parser<U> end, Container&& init = {}) {
            return CPPARSEC_DEFN(Container) {
                Container values(init);

                while (true) {
                    auto start_point = CPPARSEC_GET_INPUT_DATA;

                    if (end.parse(input)) {
                        break; // end parser succeeded, stop accumulating
                    }
                    CPPARSEC_FAIL_IF(start_point != CPPARSEC_GET_INPUT_DATA, ParseError("many_tillfail", "many_tillfail"));

                    if (ParseResult<T> result = p.parse(input)) {
                        values.push_back(*result);
                        continue;
                    }

                    // neither end nor p parsed successfully, fail
                    CPPARSEC_FAIL(ParseError("many_tillfail", "many_tillfail"));
                }

                return values;
            };
        }
    };

    // Parses p zero or more times until end succeeds, returning the parsed values
    template <typename T, typename U, PushBack<T> Container = std::vector<T>>
    Parser<Container> many_till(Parser<T> p, Parser<U> end) {
        return helper::many_till_accumulator(p, end);
    }

    // Parses p one or more times until end succeeds, returning the parsed values
    template <typename T, typename U, PushBack<T> Container = std::vector<T>>
    Parser<Container> many1_till(Parser<T> p, Parser<U> end) {
        return CPPARSEC_DEFN(Container) {
            CPPARSEC_SAVE(first, p);
            CPPARSEC_SAVE(values, helper::many_till_accumulator(p, end, { first }));

            return values;
        };
    }

    // Parses p zero or more times until end succeeds, returning the parsed values, std::string specialization
    template <typename T, PushBack<char> StringContainer = std::string>
    Parser<StringContainer> many_till(Parser<char> p, Parser<T> end) {
        return helper::many_till_accumulator(p, end, StringContainer());
    }

    // Parses p one or more times until end succeeds, returning the parsed values, std::string specialization
    template <typename T, PushBack<char> StringContainer = std::string>
    Parser<StringContainer> many1_till(Parser<char> p, Parser<T> end) {
        return CPPARSEC_DEFN(StringContainer) {
            CPPARSEC_SAVE(first, p);
            CPPARSEC_SAVE(values, helper::many_till_accumulator(p, end, StringContainer({ first })));

            return values;
        };
    }

    // Parses zero or more instances of p, ignores results
    template <typename T>
    Parser<std::monostate> skip_many(Parser<T> p) {
        return CPPARSEC_DEFN(std::monostate) {
            while (true) {
                auto starting_point = CPPARSEC_GET_INPUT_DATA;
                if (!p.parse(input)) {
                    CPPARSEC_FAIL_IF(starting_point != CPPARSEC_GET_INPUT_DATA, ParseError("skip_many", "skip_many"));
                    break;
                }
            }
            return std::monostate{}; // Return placeholder for success
        };
    }

    // Parses one or more instances of p, ignores results
    template <typename T>
    Parser<std::monostate> skip_many1(Parser<T> p) {
        return p >> skip_many(p);
    }

    // Parse zero or more parses of p separated by sep
    template <typename T, typename U>
    Parser<std::vector<T>> sep_by(Parser<T> p, Parser<U> sep) {
        return sep_by1(p, sep) | success(std::vector<T>());
    }

    // Parse one or more parses of p separated by sep
    template <typename T, typename U>
    Parser<std::vector<T>> sep_by1(Parser<T> p, Parser<U> sep) {
        return CPPARSEC_DEFN(std::vector<T>) {
            CPPARSEC_SAVE(first, p);
            CPPARSEC_SAVE(values, helper::many_accumulator(sep >> p, { first }));

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
            CPPARSEC_SAVE(values, helper::many_accumulator(sep >> p, std::string(1, first)));

            return values;
        };
    }

    // Parse zero or more parses of p separated by and ending with sep
    template <typename T, typename U>
    Parser<std::vector<T>> end_by(Parser<T> p, Parser<U> sep) {
        return many(p << sep);
    }

    // Parse one or more parses of p separated by and ending with sep
    template <typename T, typename U>
    Parser<std::vector<T>> end_by1(Parser<T> p, Parser<U> sep) {
        return many1(p << sep);
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

    // Parse one or more left associative applications of op to p, returning the
    // result of the repeated applications. Can be used to parse 1+2+3+4 as ((1+2)+3)+4
    template <typename T>
    Parser<T> chainl1(Parser<T> arg, Parser<std::function<T(T, T)>> op) {
        return CPPARSEC_DEFN(T) {
            CPPARSEC_SAVE(arg1, arg);

            while (true) {
                auto start_point = CPPARSEC_GET_INPUT_DATA;
                ParseResult<std::function<T(T, T)>> f = op.parse(input);
                if (!f) {
                    CPPARSEC_FAIL_IF(start_point != CPPARSEC_GET_INPUT_DATA, f.error()());
                    break;
                }
                CPPARSEC_SAVE(arg2, arg);

                arg1 = (*f)(arg1, arg2);
            }

            return arg1;
        };
    }

    // Parse zero or more left associative applications of op to p, returning the
    // result of the repeated applications. If there are zero applications, return backup.
    template <typename T>
    Parser<T> chainl(Parser<T> arg, Parser<std::function<T(T, T)>> op, T backup) {
        return chainl1(arg, op) | success(backup);
    }

    // Takes a std::function of a parser (not the parser itself) for deferred evaluation
    // Can be used to avoid infinite cycles in mutual recursion
    template<typename T>
    Parser<T> lazy(std::function<Parser<T>()> parser_func) {
        return CPPARSEC_DEFN(T) {
            return parser_func().parse(input);
        };
    }

    // Takes a function pointer to a parser (not the parser itself) for deferred evaluation
    // Can be used to avoid infinite cycles in mutual recursion
    template<typename T>
    Parser<T> lazy(Parser<T>(*parser_func)()) {
        return CPPARSEC_DEFN(T) {
            return parser_func().parse(input);
        };
    }

    // =========================== Operators ==================================

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

    // | "or" operator parses the left parser, then the right parser if the left one fails without consuming
    template <typename T>
    Parser<T> operator|(const Parser<T>& left, const Parser<T>& right) {
        return left.or_(right);
    }

    // & "and" operator joins two parses
    template <typename T, typename U>
    Parser<std::tuple<T, U>> operator&(const Parser<T> left, const Parser<U> right) {
        return CPPARSEC_DEFN(std::tuple<T, U>) {
            CPPARSEC_SAVE(a, left);
            CPPARSEC_SAVE(b, right);

            return std::make_tuple(a, b);
        };
    }

    // & "and" operator joins a parse and multiple parses
    template<typename T, typename... Ts>
    Parser<std::tuple<T, Ts...>> operator&(const Parser<T>& left, const Parser<std::tuple<Ts...>>& right) {
        return CPPARSEC_DEFN(std::tuple<T, Ts...>) {
            CPPARSEC_SAVE(a, left);
            CPPARSEC_SAVE(bs, right);

            return std::tuple_cat(std::make_tuple(a), bs);
        };
    }

    // & "and" operator joins multiple parses and a parse 
    template<typename T, typename... Ts>
    Parser<std::tuple<T, Ts...>> operator&(const Parser<std::tuple<Ts...>>& left, const Parser<T>& right) {
        return CPPARSEC_DEFN(std::tuple<T, Ts...>) {
            CPPARSEC_SAVE(as, left);
            CPPARSEC_SAVE(b, right);

            return std::tuple_cat(as, std::make_tuple(b));
        };
    }

    // & "and" operator joins multiple parses and multiple parses
    template<typename... Ts, typename... Us>
    Parser<std::tuple<Ts..., Us...>> operator&(const Parser<std::tuple<Ts...>>& left, const Parser<std::tuple<Us...>>& right) {
        return CPPARSEC_DEFN(std::tuple<Ts..., Us...>) {
            CPPARSEC_SAVE(as, left);
            CPPARSEC_SAVE(bs, right);

            return std::tuple_cat(as, bs);
        };
    }

    // Add an error message to the parse result if it fails
    // Designed for debugging, poor performance
    template <typename T>
    Parser<T> operator^(Parser<T>&& p, std::string&& msg) {
        //return p;
        return CPPARSEC_DEFN(T) {
            ParseResult<T> result = p.parse(input);
            if (!result) {
                ParseError err = result.error()();
                err.add_error({ msg });
                CPPARSEC_FAIL(err);
            }
            return result;
        };
    }

    // Replace an error message from the parse result if it fails
    // Designed for debugging, poor performance
    template <typename T>
    Parser<T> operator%(Parser<T> p, std::string&& msg) {
        return CPPARSEC_DEFN(T) {
            ParseResult<T> result = p.parse(input);
            if (!result.has_value()) {
                result.error() = ParseError(msg);
            }
            return result;
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

#endif /* CPPARSEC_CORE_H */
