#ifndef CPPARSEC_CORE_H
#define CPPARSEC_CORE_H

#include <print>
#include <iostream>
#include <vector>
#include <expected>
#include <variant>
#include <string>
#include <string_view>
#include <algorithm>
#include <functional>
#include <utility>
#include <ranges>
#include <concepts>

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

#define CPPARSEC_MAKE(...) \
    cpparsec::_ParserFactory<__VA_ARGS__> () = [=](__VA_ARGS__::InputStream& input) -> cpparsec::ParseResult<typename __VA_ARGS__::Item>

#define CPPARSEC_MAKE_METHOD(name, ...) \
    cpparsec::_ParserFactory<__VA_ARGS__> () = [=, name = *this](__VA_ARGS__::InputStream& input) -> cpparsec::ParseResult<typename __VA_ARGS__::Item>

using std::println;

namespace cpparsec {

    // ========================================================================
    // 
    // ======================= HEADERS AND DECLARATIONS =======================
    // 
    // ========================================================================

    // ============================= PARSE ERROR ==============================

    template <std::formattable<char> Atom = char>
    class ParseError {
    public:
        struct ErrorContent
            : std::variant<std::pair<std::string, std::string>, std::pair<Atom, Atom>, std::string, std::monostate>
        { };

        ParseError(ErrorContent&& err) : errors({ err }) { }
        ParseError(const std::string expected, const std::string found) : errors({ ErrorContent{std::pair {expected, found}} }) { }
        ParseError(std::formattable<char> auto expected, std::formattable<char> auto found) : errors({ ErrorContent{std::pair {expected, found}} }) { }
        ParseError(std::string&& msg) : errors({ ErrorContent{msg} }) { }

        // Add error to error container
        ParseError& add_error(ErrorContent&& err);

        // Returns deepest error message as a std::string
        std::string message();

        // Returns shallowest error message as a std::string
        std::string message_top();

        // Returns all error messages as a std::string
        std::string message_stack();

    private:
        std::vector<ErrorContent> errors;
    };

    template<typename T, std::formattable<char> Atom = char>
    using ParseResult = std::expected<T, std::function<ParseError<Atom>()>>;

    // ============================ PARSER CONCEPTS ===========================

    // Parser: you can call .parse(s) on it, and it returns a ParserResult
    template<typename Parser, typename T, typename Input>
    concept ParserType = requires(Parser p, Input& s) {
        { p.parse(s) }; // ->std::same_as<ParseResult<T>>;
    };

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

    // ================================ PARSER ================================

    template<typename T, typename Input = std::string_view>
    class Parser {
    public:
        using Item = T;
        using InputStream = Input;
        using ParseFunction = std::function<ParseResult<T>(InputStream&)>; // ParseResult<T>(*)(InputStream&)
              // function takes InputStream, returns ParserResult<T>

    private:
        ParseFunction parser;

    public:
        // Implementation detail, Parsers take ParseFunctions
        // Use the CPPARSEC_MAKE macro for documented parsers
        Parser(ParseFunction&& parser);

        // Top level parser execution, parses a string
        ParseResult<T> parse(const std::string& input) const;

        // Top level parser execution, parses an InputStream
        // Parser consumes/modifies InputStream
        ParseResult<T> parse(InputStream& input) const;

        // Parses self and other, returns result of other
        template<typename U>
        Parser<U, Input> with(Parser<U, Input> other) const;

        // Parses self and other, returns result of self
        template<typename U>
        Parser<T, Input> skip(Parser<U, Input> other) const;

        // Parses self and other, returns pair of both results
        template<typename U>
        Parser<std::pair<T, U>, Input> pair_with(Parser<U, Input> other) const;

        // Parses occurence satisfying a condition
        Parser<T, Input> satisfy(std::function<bool(T)> cond) const;

        // Never consumes input and always succeeds, returns the value given. 
        template <typename U>
        Parser<U, Input> success(U item);

        // Parse occurence between two parses
        template<typename O, typename C>
        Parser<T, Input> between(Parser<O, Input> open, Parser<C, Input> close) const;

        // | "or" operator parses the left parser, then the right parser if the left one fails without consuming
        Parser<T, Input> or_(const Parser<T, Input>& right) const;

        // Parses p without consuming input on failure
        Parser<T, Input> try_() const;

        // Apply a function to the parse result
        template <typename U>
        Parser<U, Input> transform(std::function<U(T)> func) const;
    };

    // ============================ PARSER FACTORY ============================

    // Implementation detail, should never be invoked manually
    template <typename ParserType>
    struct _ParserFactory {
        ParserType operator=(ParserType::ParseFunction&& parser) {
            return ParserType(std::move(parser));
        }
    };

    // ======================= CORE PARSER COMBINATORS ========================

    // Parses given number of parses
    template<typename T, typename Input>
    Parser<std::vector<T>, Input> count(int n, Parser<T, Input> p);

    // Parses a sequence of functions, returning on the first successful result.
    template<typename T, typename Input>
    Parser<T, Input> choice(std::vector<Parser<T, Input>> parsers);

    // Parse occurence between two parses
    // between(open, close, p) is open.with(p).skip(close) is open >> p << close
    template<typename O, typename C, typename T, typename Input>
    Parser<T, Input> between(Parser<O> open, Parser<C> close, Parser<T, Input> p);

    // between 2
    template<typename O, typename C, typename T, typename Input>
    Parser<T, Input> between3(Parser<O> open, Parser<C> close, Parser<T, Input> p);

    // | "or" parses the left parser, then the right parser if the left one fails without consuming
    template <typename T, typename Input>
    Parser<T, Input> or_(const Parser<T, Input>& left, const Parser<T, Input>& right);

    // Parses a given parser, doesn't consume input on failure
    // Commonly used with | "or" operator
    template <typename T, typename Input>
    Parser<T, Input> try_(Parser<T, Input> p);

    // Parses p without consuming input. If p fails, it will consume input. Wrap p with try_ to avoid this.
    template <typename T, typename Input>
    Parser<T, Input> look_ahead(Parser<T, Input> p);

    // Succeeds only if p fails to parse. Never consumes input.
    template <typename T, typename Input>
    Parser<std::monostate, Input> not_followed_by(Parser<T, Input> p);

    // Parses p if p passes a condition, failing if it doesn't
    template<typename T, typename Input>
    Parser<T, Input> satisfy(const Parser<T, Input>& parser, std::function<bool(T)> cond);

    // Never consumes input and always succeeds, returns the value given. 
    template <typename Input, typename T>
    Parser<T, Input> success(T item);

    // A parser which always fails without consuming input
    template <typename Input, typename T>
    Parser<T, Input> unexpected();

    // Parses successfully only if the input is empty
    template <typename Input, typename T>
    Parser<T, Input> eof();

    // Parses p, ignoring the result
    // Does not improve performance (return types are not lazy)
    template<typename T, typename Input>
    Parser<std::monostate, typename Input> skip(Parser<T, typename Input> p);

    // Parses for an optional p, succeeds if p fails without consuming
    template<typename T, typename Input>
    Parser<std::monostate, Input> optional_(Parser<T, Input> p);

    // Parses for an optional p, succeeds if p fails without consuming
    template<typename T, typename Input>
    Parser<std::optional<T>, Input> optional_result(Parser<T, Input> p);

    // Parse zero or more parses
    template<typename T, PushBack<T> Container, typename Input>
    Parser<Container, Input> many(Parser<T, Input> p);

    // Parse one or more parses
    template<typename T, PushBack<T> Container, typename Input>
    Parser<Container, Input> many1(Parser<T, Input> p);

    // Parses p zero or more times until end succeeds, returning the parsed values
    template <typename T, typename U, PushBack<T> Container, typename Input>
    Parser<Container, Input> many_till(Parser<T, Input> p, Parser<U, Input> end);

    // Parses p one or more times until end succeeds, returning the parsed values
    template <typename T, typename U, PushBack<T> Container, typename Input>
    Parser<Container, Input> many1_till(Parser<T, Input> p, Parser<U, Input> end);

    // Parses zero or more instances of p, ignores results
    template <typename T, typename Input>
    Parser<std::monostate, Input> skip_many(Parser<T, Input> p);

    // Parses one or more instances of p, ignores results
    template <typename T, typename Input>
    Parser<std::monostate, Input> skip_many1(Parser<T, Input> p);

    // Parse zero or more parses of p separated by sep
    template <typename T, typename U, typename Input>
    Parser<std::vector<T>, Input> sep_by(Parser<T, Input> p, Parser<U, Input> sep);

    // Parse one or more parses of p separated by sep
    template <typename T, typename U, typename Input>
    Parser<std::vector<T>, Input> sep_by1(Parser<T, Input> p, Parser<U, Input> sep);

    // Parse zero or more parses of p separated by and ending with sep
    template <typename T, typename U, typename Input>
    Parser<std::vector<T>, Input> end_by(Parser<T, Input> p, Parser<U, Input> sep);

    // Parse one or more parses of p separated by and ending with sep
    template <typename T, typename U, typename Input>
    Parser<std::vector<T>, Input> end_by1(Parser<T, Input> p, Parser<U, Input> sep);

    // Parse one or more left associative applications of op to p, returning the
    // result of the repeated applications. Can be used to parse 1+2+3+4 as ((1+2)+3)+4
    template <typename T, typename Input>
    Parser<T, Input> chainl1(Parser<T, Input> arg, Parser<std::function<T(T, T)>, Input> op);

    // Parse zero or more left associative applications of op to p, returning the
    // result of the repeated applications. If there are zero applications, return backup.
    template <typename T, typename Input>
    Parser<T, Input> chainl(Parser<T, Input> arg, Parser<std::function<T(T, T)>, Input> op, T backup);

    // Takes a std::function of a parser (not the parser itself) for deferred evaluation
    // Can be used to avoid infinite cycles in mutual recursion
    template<typename T, typename Input>
    Parser<T, Input> lazy(std::function<Parser<T, Input>()> parser_func);

    // Takes a function pointer to a parser (not the parser itself) for deferred evaluation
    // Can be used to avoid infinite cycles in mutual recursion
    template<typename T, typename Input>
    Parser<T, Input> lazy(Parser<T, Input>(*parser_func)());

    // =========================== OPERATORS ==================================

    // << "ignore" operator returns the first result of two parsers
    // a << b is a.skip(b) 
    template <typename T, typename U, typename Input>
    Parser<T, Input> operator<<(const Parser<T, Input> left, const Parser<U, Input> right);

    // >> "sequence" operator returns the second result of two parsers
    // a >> b is a.with(b) 
    template <typename T, typename U, typename Input>
    Parser<U, Input> operator>>(const Parser<T, Input> left, const Parser<U, Input> right);

    // | "or" operator parses the left parser, then the right parser if the left one fails without consuming
    template <typename T, typename Input>
    Parser<T, Input> operator|(const Parser<T, Input>& left, const Parser<T, Input>& right);

    // & "and" operator joins two parses
    template <typename T, typename U, typename Input>
    Parser<std::tuple<T, U>, Input> operator&(const Parser<T, Input> left, const Parser<U, Input> right);

    // & "and" operator joins a parse and multiple parses
    template<typename T, typename... Ts, typename Input>
    Parser<std::tuple<T, Ts...>, Input> operator&(const Parser<T, Input>& left, const Parser<std::tuple<Ts...>, Input>& right);

    // & "and" operator joins multiple parses and a parse 
    template<typename T, typename... Ts, typename Input>
    Parser<std::tuple<T, Ts...>, Input> operator&(const Parser<std::tuple<Ts...>, Input>& left, const Parser<T, Input>& right);

    // & "and" operator joins multiple parses and multiple parses
    template<typename... Ts, typename... Us, typename Input>
    Parser<std::tuple<Ts..., Us...>, Input> operator&(const Parser<std::tuple<Ts...>, Input>& left, const Parser<std::tuple<Us...>, Input>& right);

    // Add an error message to the parse result if it fails
    // Designed for debugging purposes, is slow
    template <typename T, typename Input>
    Parser<T, Input> operator^(Parser<T, Input>&& p, std::string&& msg);

    // Replace an error message from the parse result if it fails
    // Designed for debugging purposes, is slow
    template <typename T, typename Input>
    Parser<T, Input> operator%(Parser<T, Input> p, std::string&& msg);

    // ========================================================================
    // 
    // ======================= TEMPLATE IMPLEMENTATIONS =======================
    // 
    // ========================================================================

    // ============================= PARSE ERROR ==============================

    // Add error to error container
    template <std::formattable<char> Atom>
    ParseError<Atom>& ParseError<Atom>::add_error(ErrorContent&& err) {
        errors.push_back(err);
        return *this;
    }

    // Returns deepest error message as a std::string
    template <std::formattable<char> Atom>
    std::string ParseError<Atom>::message() {
        return std::format("{}", errors.front());
    }

    // Returns shallowest error message as a std::string
    template <std::formattable<char> Atom>
    std::string ParseError<Atom>::message_top() {
        return std::format("{}", errors.back());
    }

    // Returns all error messages as a std::string
    template <std::formattable<char> Atom>
    std::string ParseError<Atom>::message_stack() {
        std::string msg = std::format("{}", errors[0]);

        for (size_t i = 1; i < errors.size(); i++) {
            msg += std::format("\n{}", errors[i]);
        }

        return msg;
    }

    // ================================ Parser ================================

    // Implementation detail, Parsers take ParseFunctions
    // Use the CPPARSEC_MAKE macro for documented parsers
    template <typename T, typename Input>
    Parser<T, Input>::Parser(ParseFunction&& parser) :
        parser(std::move(parser))
    { }

    // Top level parser execution, parses a string
    template <typename T, typename Input>
    ParseResult<T> Parser<T, Input>::parse(const std::string& input) const {
        InputStream view = input;
        return parser(view);
    }

    // Top level parser execution, parses a string_view
    // Parser consumes/modifies string_view
    template <typename T, typename Input>
    ParseResult<T> Parser<T, Input>::parse(Parser<T, Input>::InputStream& input) const {
        InputStream view = input;
        auto result = parser(view);
        input = view;
        return result;
    }

    // Parses self and other, returns result of other
    template<typename T, typename Input>
    template<typename U>
    Parser<U, Input> Parser<T, Input>::with(Parser<U, Input> other) const {
        return CPPARSEC_MAKE_METHOD(thisParser, Parser<U, Input>) {
            CPPARSEC_SKIP(thisParser);
            CPPARSEC_SAVE(result, other);

            return result;
        };
    }

    // Parses self and other, returns result of self
    template<typename T, typename Input>
    template<typename U>
    Parser<T, Input> Parser<T, Input>::skip(Parser<U, Input> other) const {
        return CPPARSEC_MAKE_METHOD(thisParser, Parser<T, Input>) {
            CPPARSEC_SAVE(result, thisParser);
            CPPARSEC_SKIP(other);

            return result;
        };
    }


    // Parses self and other, returns pair of both results
    template<typename T, typename Input>
    template<typename U>
    Parser<std::pair<T, U>, Input> Parser<T, Input>::pair_with(Parser<U, Input> other) const {
        return CPPARSEC_MAKE_METHOD(thisParser, Parser<std::pair<T, U>, Input>) {
            CPPARSEC_SAVE(a, thisParser);
            CPPARSEC_SAVE(b, other);

            return std::pair(a, b);
        };
    }

    // Parses occurence satisfying a condition
    template<typename T, typename Input>
    Parser<T, Input> Parser<T, Input>::satisfy(std::function<bool(T)> cond) const {
        return CPPARSEC_MAKE_METHOD(thisParser, Parser<T, Input>) {
            ParseResult<T> result = thisParser.parse(input);
            CPPARSEC_FAIL_IF(!result || !cond(*result), result.error());

            return result;
        };
    }

    // Never consumes input and always succeeds, returns the value given. 
    template<typename T, typename Input>
    template <typename U>
    Parser<U, Input> Parser<T, Input>::success(U item) {
        return CPPARSEC_MAKE_METHOD(thisParser, Parser<U, Input>) {
            CPPARSEC_SKIP(thisParser);
            return item;
        };
    }

    // Parse occurence between two parses
    template<typename T, typename Input>
    template<typename O, typename C>
    Parser<T, Input> Parser<T, Input>::between(Parser<O, Input> open, Parser<C, Input> close) const {
        return open.with(*this).skip(close);
    }

    // | "or" operator parses the left parser, then the right parser if the left one fails without consuming
    template <typename T, typename Input>
    Parser<T, Input> Parser<T, Input>::or_(const Parser<T, Input>& right) const {
        return CPPARSEC_MAKE_METHOD(thisParser, Parser<T, Input>) {
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
    template<typename T, typename Input>
    Parser<T, Input> Parser<T, Input>::try_() const {
        return CPPARSEC_MAKE_METHOD(thisParser, Parser<T, Input>) {
            auto starting_input = input;
            ParseResult<T> result = thisParser.parse(input);
            if (!result) {
                input = starting_input; // undo input consumption
            }

            return result;
        };
    }

    // Apply a function to the parse result
    template<typename T, typename Input>
    template <typename U>
    Parser<U, Input> Parser<T, Input>::transform(std::function<U(T)> func) const {
        return CPPARSEC_MAKE_METHOD(thisParser, Parser<U, Input>) {
            ParseResult<T> result = thisParser.parse(input);
            return result.transform(func);
        };
    }

    // ======================= Core Parser Combinators ========================

    // Parses given number of parses
    template<typename T, typename Input>
    Parser<std::vector<T>, Input> count(int n, Parser<T, Input> p) {
        return CPPARSEC_MAKE(Parser<std::vector<T>, Input>) {
            std::vector<T> vec(n);

            for (int i = 0; i < n; i++) {
                CPPARSEC_SAVE(val, p);
                vec[i] = val;
            }

            return vec;
        };
    }

    // Parses a sequence of functions, returning on the first successful result.
    template<typename T, typename Input>
    Parser<T, Input> choice(std::vector<Parser<T, Input>> parsers) {
        if (std::ranges::empty(parsers)) {
            return unexpected<Input, T>();
        }

        return std::ranges::fold_left_first(parsers, std::bit_or<>{}).value();
    }

    // Parse occurence between two parses
    template<typename O, typename C, typename T, typename Input>
    Parser<T, Input> between(Parser<O> open, Parser<C> close, Parser<T, Input> p) {
        return open >> p << close;
    }

    template<typename O, typename C, typename T, typename Input>
    Parser<T, Input> between2(Parser<O> open, Parser<C> close, Parser<T, Input> p) {
        return open.with(p).skip(close);
    }

    template<typename O, typename C, typename T, typename Input>
    Parser<T, Input> between3(Parser<O> open, Parser<C> close, Parser<T, Input> p) {
        return cpparsec::_ParserFactory<Parser<T, Input>>() = [=](Parser<T, Input>::InputStream& input)->cpparsec::ParseResult<typename Parser<T, Input>::Item> {
            if (auto&& _cpparsec_skipresult = (open).parse(input); !_cpparsec_skipresult) {
                return std::unexpected(std::move(_cpparsec_skipresult.error()));
            };
            auto&& _middle_ = (p).parse(input); if (!_middle_.has_value()) {
                return std::unexpected(std::move(_middle_.error()));
            } 
            auto&& middle = _middle_.value();
            if (auto&& _cpparsec_skipresult = (close).parse(input); !_cpparsec_skipresult) {
                return std::unexpected(std::move(_cpparsec_skipresult.error()));
            };

            return middle;
        };
    }

    // | "or" parses the left parser, then the right parser if the left one fails without consuming
    template <typename T, typename Input>
    Parser<T, Input> or_(const Parser<T, Input>& left, const Parser<T, Input>& right) {
        return left.or_(right);
    }

    // Parses a given parser, doesn't consume input on failure
    // Commonly used with | "or" operator
    template <typename T, typename Input>
    Parser<T, Input> try_(Parser<T, Input> p) {
        return p.try_();
    }

    // Parses p without consuming input. If p fails, it will consume input. Wrap p with try_ to avoid this.
    template <typename T, typename Input>
    Parser<T, Input> look_ahead(Parser<T, Input> p) {
        return CPPARSEC_MAKE(Parser<T, Input>) {
            auto input_copy = input;
            CPPARSEC_SAVE(value, p);
            input = input_copy;

            return value;
        };
    }

    // Succeeds only if p fails to parse. Never consumes input.
    template <typename T, typename Input>
    Parser<std::monostate, Input> not_followed_by(Parser<T, Input> p) {
        return CPPARSEC_MAKE(Parser<std::monostate, Input>) {
            auto input_copy = input;
            ParseResult<T> result = p.parse(input);
            input = input_copy;
            CPPARSEC_FAIL_IF(result.has_value(), ParseError("not_followed_by", "not_followed_by"));

            return std::monostate{};
        };
    }

    // Parses p if p passes a condition, failing if it doesn't
    template<typename T>
    Parser<T> satisfy(const Parser<T>& parser, std::function<bool(T)> cond) {
        return CPPARSEC_MAKE_METHOD(thisParser, T) {
            ParseResult<T> result = thisParser(input);
            CPPARSEC_FAIL_IF(!result || !cond(*result), std::format("Failed satisfy"));

            return result;
        };
    }

    // Never consumes input and always succeeds, returns the value given. 
    // Input type must be specified for custom Input types
    template <typename Input = std::string_view, typename T>
    Parser<T, Input> success(T item) {
        return CPPARSEC_MAKE(Parser<T, Input>) {
            return item;
        };
    }

    // A parser which always fails without consuming input
    // Input type must be specified for custom Input types
    template <typename Input = std::string_view, typename T = std::monostate>
    Parser<T, Input> unexpected() {
        return CPPARSEC_MAKE(Parser<T, Input>) {
            CPPARSEC_FAIL(ParseError("unexpected"));
        };
    }

    // Parses successfully only if the input is empty
    // Input type must be specified for custom Input types
    template <typename Input = std::string_view, typename T = std::monostate>
    Parser<T, Input> eof() {
        return CPPARSEC_MAKE(Parser<T, Input>) {
            CPPARSEC_FAIL_IF(input.size() > 0, ParseError(std::format("{}", *input.begin()), "end of input"));
            return T{};
        };
    }

    //// Parses p, ignoring the result
    //// Does not improve performance (return types are not lazy)
    //template<typename T>
    //Parser<std::monostate> skip(Parser<T> p) {
    //    return p >> success(std::monostate{});
    //}

    // Parses p, ignoring the result
    // Does not improve performance (return types are not lazy)
    template<typename T, typename Input>
    Parser<std::monostate, Input> skip(Parser<T, Input> p) {
        return p.success(std::monostate{});
    }

    //// Parses p, ignoring the result
    //// Does not improve performance (return types are not lazy)
    //template <typename T, typename Input>
    //ParserType<std::monostate, Input> auto skip(ParserType<T, Input> auto p) {
    //    return p >> success(std::monostate{});
    //}


    // Parses for an optional p, succeeds if p fails without consuming
    template<typename T, typename Input>
    Parser<std::monostate, Input> optional_(Parser<T, Input> p) {
        return p.success(std::monostate{}) | success<Input>(std::monostate{});
    }

    // Parses for an optional p, succeeds if p fails without consuming
    template<typename T, typename Input>
    Parser<std::optional<T>, Input> optional_result(Parser<T, Input> p) {
        return CPPARSEC_MAKE(Parser<std::optional<T>, Input>) {
            auto start_point = input.data();
            ParseResult<T> result = p.parse(input);
            CPPARSEC_FAIL_IF(!result && start_point != input.data(), result.error()());

            return (result ? std::optional(result.value()) : std::nullopt);
        };
        // WIP
        //return p.transform([](auto r) { return std::optional(r); }) | std::nullopt;
    }

    namespace detail {
        template <typename T, PushBack<T> Container = std::vector<T>, typename Input>
            requires std::movable<Container>
        Parser<Container, Input> many_accumulator(Parser<T, Input> p, Container&& init = {}) {
            return CPPARSEC_MAKE(Parser<Container, Input>) {
                Container values(init);

                while (true) {
                    auto starting_point = input.data();
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
    template<typename T, PushBack<T> Container = std::vector<T>, typename Input>
    Parser<Container, Input> many(Parser<T, Input> p) {
        return detail::many_accumulator<T, Container>(p);
    }

    // Parse one or more parses
    template<typename T, PushBack<T> Container = std::vector<T>, typename Input>
    Parser<Container, Input> many1(Parser<T, Input> p) {
        return CPPARSEC_MAKE(Parser<Container, Input>) {
            CPPARSEC_SAVE(first, p);
            CPPARSEC_SAVE(values, detail::many_accumulator<T, Container>(p, { first }));

            return values;
        };
    }

    namespace detail {
        template <typename T, typename U, PushBack<T> Container = std::vector<T>, typename Input>
            requires std::movable<Container>
        Parser<Container, Input> many_till_accumulator(Parser<T, Input> p, Parser<U, Input> end, Container&& init = {}) {
            return CPPARSEC_MAKE(Parser<Container, Input>) {
                Container values(init);

                while (true) {
                    auto start_point = input.data();

                    if (end.parse(input)) {
                        break; // end parser succeeded, stop accumulating
                    }
                    CPPARSEC_FAIL_IF(start_point != input.data(), ParseError("many_tillfail", "many_tillfail"));

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
    template <typename T, typename U, PushBack<T> Container = std::vector<T>, typename Input>
    Parser<Container, Input> many_till(Parser<T, Input> p, Parser<U, Input> end) {
        return detail::many_till_accumulator<T, U, Container, Input>(p, end);
    }

    // Parses p one or more times until end succeeds, returning the parsed values
    template <typename T, typename U, PushBack<T> Container = std::vector<T>, typename Input>
    Parser<Container, Input> many1_till(Parser<T, Input> p, Parser<U, Input> end) {
        return CPPARSEC_MAKE(Parser<Container, Input>) {
            CPPARSEC_SAVE(first, p);
            CPPARSEC_SAVE(values, detail::many_till_accumulator<T, U, Container, Input>(p, end, { first }));

            return values;
        };
    }


    //template<typename T, typename Input, ParserType<T, Input> P>
    //ParserType<T, Input> auto count(int n, P p) {
    //    return cpparsec::_ParserFactory<std::vector<T>>() = [=](Parser<std::vector<T>>::InputStream& input)->cpparsec::ParseResult<std::vector<T>> {
    //        std::vector<T> vec;
    //        for (int i = 0; i < n; i++) {
    //            CPPARSEC_SAVE(val, p);
    //            vec.push_back(std::move(val));
    //        }
    //        return vec;
    //    };
    //}

    // Parses zero or more instances of p, ignores results
    template <typename T, typename Input>
    Parser<std::monostate, Input> skip_many(Parser<T, Input> p) {
        return CPPARSEC_MAKE(Parser<std::monostate, Input>) {
            while (true) {
                auto starting_point = input.data();
                if (!p.parse(input)) {
                    CPPARSEC_FAIL_IF(starting_point != input.data(), ParseError("skip_many", "skip_many"));
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
    template <typename T, typename U, typename Input>
    Parser<std::vector<T>, Input> sep_by(Parser<T, Input> p, Parser<U, Input> sep) {
        return sep_by1(p, sep) | success<Input>(std::vector<T>());
    }

    // Parse one or more parses of p separated by sep
    template <typename T, typename U, typename Input>
    Parser<std::vector<T>, Input> sep_by1(Parser<T, Input> p, Parser<U, Input> sep) {
        return CPPARSEC_MAKE(Parser<std::vector<T>, Input>) {
            CPPARSEC_SAVE(first, p);
            CPPARSEC_SAVE(values, detail::many_accumulator(sep >> p, { first }));

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

    // Parse one or more left associative applications of op to p, returning the
    // result of the repeated applications. Can be used to parse 1+2+3+4 as ((1+2)+3)+4
    template <typename T, typename Input>
    Parser<T, Input> chainl1(Parser<T, Input> arg, Parser<std::function<T(T, T)>, Input> op) {
        return CPPARSEC_MAKE(Parser<T, Input>) {
            CPPARSEC_SAVE(arg1, arg);

            while (true) {
                auto start_point = input.data();
                ParseResult<std::function<T(T, T)>> f = op.parse(input);
                if (!f) {
                    CPPARSEC_FAIL_IF(start_point != input.data(), f.error()());
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
    template <typename T, typename Input>
    Parser<T, Input> chainl(Parser<T, Input> arg, Parser<std::function<T(T, T)>, Input> op, T backup) {
        return chainl1(arg, op) | success(backup);
    }

    // Takes a std::function of a parser (not the parser itself) for deferred evaluation
    // Can be used to avoid infinite cycles in mutual recursion
    template<typename T, typename Input>
    Parser<T, Input> lazy(std::function<Parser<T, Input>()> parser_func) {
        return CPPARSEC_MAKE(Parser<T, Input>) {
            return parser_func().parse(input);
        };
    }

    // Takes a function pointer to a parser (not the parser itself) for deferred evaluation
    // Can be used to avoid infinite cycles in mutual recursion
    template<typename T, typename Input>
    Parser<T, Input> lazy(Parser<T, Input>(*parser_func)()) {
        return CPPARSEC_MAKE(Parser<T, Input>) {
            return parser_func().parse(input);
        };
    }

    // =========================== Operators ==================================

    // << "ignore" operator returns the first result of two parsers
    // a << b is a.skip(b) 
    template <typename T, typename U, typename Input>
    Parser<T, Input> operator<<(const Parser<T, Input> left, const Parser<U, Input> right) {
        return left.skip(right);
    }

    // >> "sequence" operator returns the second result of two parsers
    // a >> b is a.with(b) 
    template <typename T, typename U, typename Input>
    Parser<U, Input> operator>>(const Parser<T, Input> left, const Parser<U, Input> right) {
        return left.with(right);
    }

    // | "or" operator parses the left parser, then the right parser if the left one fails without consuming
    template <typename T, typename Input>
    Parser<T, Input> operator|(const Parser<T, Input>& left, const Parser<T, Input>& right) {
        return left.or_(right);
    }

    // & "and" operator joins two parses
    template <typename T, typename U, typename Input>
    Parser<std::tuple<T, U>, Input> operator&(const Parser<T, Input> left, const Parser<U, Input> right) {
        return CPPARSEC_MAKE(Parser<std::tuple<T, U>, Input>) {
            CPPARSEC_SAVE(a, left);
            CPPARSEC_SAVE(b, right);

            return std::make_tuple(a, b);
        };
    }

    // & "and" operator joins a parse and multiple parses
    template<typename T, typename... Ts, typename Input>
    Parser<std::tuple<T, Ts...>, Input> operator&(const Parser<T, Input>& left, const Parser<std::tuple<Ts...>, Input>& right) {
        return CPPARSEC_MAKE(Parser<std::tuple<T, Ts...>, Input>) {
            CPPARSEC_SAVE(a, left);
            CPPARSEC_SAVE(bs, right);

            return std::tuple_cat(std::make_tuple(a), bs);
        };
    }

    // & "and" operator joins multiple parses and a parse 
    template<typename T, typename... Ts, typename Input>
    Parser<std::tuple<T, Ts...>, Input> operator&(const Parser<std::tuple<Ts...>, Input>& left, const Parser<T, Input>& right) {
        return CPPARSEC_MAKE(Parser<std::tuple<T, Ts...>, Input>) {
            CPPARSEC_SAVE(as, left);
            CPPARSEC_SAVE(b, right);

            return std::tuple_cat(as, std::make_tuple(b));
        };
    }

    // & "and" operator joins multiple parses and multiple parses
    template<typename... Ts, typename... Us, typename Input>
    Parser<std::tuple<Ts..., Us...>, Input> operator&(const Parser<std::tuple<Ts...>, Input>& left, const Parser<std::tuple<Us...>, Input>& right) {
        return CPPARSEC_MAKE(Parser<std::tuple<Ts..., Us...>, Input>) {
            CPPARSEC_SAVE(as, left);
            CPPARSEC_SAVE(bs, right);

            return std::tuple_cat(as, bs);
        };
    }

    // Add an error message to the parse result if it fails
    // Designed for debugging, poor performance
    template <typename T, typename Input>
    Parser<T, Input> operator^(Parser<T, Input>&& p, std::string&& msg) {
        //return p;
        return CPPARSEC_MAKE(Parser<T, Input>) {
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
    template <typename T, typename Input>
    Parser<T, Input> operator%(Parser<T, Input> p, std::string&& msg) {
        return CPPARSEC_MAKE(Parser<T, Input>) {
            ParseResult<T> result = p.parse(input);
            if (!result.has_value()) {
                CPPARSEC_FAIL(ParseError({ msg }));
            }
            return result;
        };
    }

};

// needs to be outside namespace to be seen by fmt
template <>
struct std::formatter<typename cpparsec::ParseError<>::ErrorContent> {
    auto parse(std::format_parse_context& ctx) {
        return ctx.end();
    }

    template <std::formattable<char> T = char>
    auto format(const typename cpparsec::ParseError<T>::ErrorContent& error, std::format_context& ctx) const {
        return std::visit([&ctx](auto&& err) {
            using V = std::decay_t<decltype(err)>;
            if constexpr (std::is_same_v<V, std::pair<T, T>>) {
                return std::format_to(ctx.out(), "Expected '{}', found '{}'", err.first, err.second);
            }
            else if constexpr (std::is_same_v<V, std::pair<std::string, std::string>>) {
                return std::format_to(ctx.out(), "Expected \"{}\", found \"{}\"", err.first, err.second);
            }
            else if constexpr (std::is_same_v<V, std::string>) {
                return std::format_to(ctx.out(), "{}", err);
            }
            else if constexpr (std::is_same_v<V, std::monostate>) {
                return std::format_to(ctx.out(), "empty error");
            }
        }, error);
    }
};

#endif /* CPPARSEC_CORE_H */
