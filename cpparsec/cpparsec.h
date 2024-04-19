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

//
#define CPPARSEC_SAVE(var, ...)                    \
    auto&& _##var##_ = (__VA_ARGS__).parse(input); \
    if (!_##var##_.has_value()) {                  \
        return std::unexpected(_##var##_.error()); \
    }                                              \
    auto&& var = _##var##_.value();

#define CPPARSEC_SKIP(p) \
    if (auto&& _cpparsec_skipresult = (p).parse(input); !_cpparsec_skipresult) { \
        return std::unexpected(_cpparsec_skipresult.error());                    \
    }                                                                            \

#define CPPARSEC_FAIL_IF(cond, message) if (cond) { return std::unexpected([=]() { return message; }); }
#define CPPARSEC_FAIL(message) return std::unexpected([=]() { return message; });

#define CPPARSEC_GET_INPUT input
#define CPPARSEC_GET_INPUT_DATA input.data()
#define CPPARSEC_SET_INPUT(new_input) input = new_input;
#define CPPARSEC_PARSERESULT(p) p.parse(input)
#define CPPARSEC_RESULT(name) *name

#define CPPARSEC_DEFN(...) \
    _ParserFactory<__VA_ARGS__>() = [=](InputStream& input) -> ParseResult<__VA_ARGS__>

#define CPPARSEC_DEFN_METHOD(name, ...) \
    _ParserFactory<__VA_ARGS__>() = [=, name = *this](InputStream& input) -> ParseResult<__VA_ARGS__>

using std::println, std::format, std::string, std::string_view;


namespace cpparsec {
    struct ErrorContent : std::variant<std::pair<std::string, std::string>, std::pair<char, char>, std::string> { };

    struct ParseError {
        std::vector<ErrorContent> errors;
        //std::vector<std::optional<std::string>> messages;

        ParseError(ErrorContent&& err)
            : errors({ err })
        {}
        ParseError(const std::string expected, const std::string found)
            : errors({ ErrorContent{std::pair {expected, found}} })
        {}
        ParseError(char expected, char found)
            : errors({ ErrorContent{std::pair{expected, found}} })
        {}
        ParseError(std::string&& msg)
            : errors({ ErrorContent{msg} })
        {}

        ParseError& add_error(ErrorContent&& err) {
            errors.push_back(err);
            return *this;
        }

        std::string message() {
            return format("{}", errors[0]);
        }
        std::string message_stack() {
            std::string msg = format("{}", errors[0]);

            for (size_t i = 1; i < errors.size(); i++) {
                msg += format("\n{}", errors[i]);
            }

            return msg;
        }
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
        Parser(ParseFunction&& parser) :
            parser(std::move(parser))
        { }

        // Top level parser execution, parses a string
        ParseResult<T> parse(const std::string& input) const {
            InputStream view = input;
            return parser(view);
        }

        // Top level parser execution, parses a string_view
        // Parser consumes/modifies string_view
        ParseResult<T> parse(std::string_view& input) const {
            InputStream view = input;
            auto result = parser(view);
            input = view;
            return result;
        }

        //// Executes the parser, modifying InputStream
        //// Implementation only
        //ParseResult<T> parse(InputStream& input) const {
        //    // return (*parser)(input);
        //    return input ? (*parser)(input) : std::nullopt;
        //}

        // Parses self and other, returns result of other
        template<typename U>
        Parser<U> with(Parser<U> other) const {
            return Parser<U>([=, selfParser = *this](InputStream& input) -> ParseResult<U> {
                CPPARSEC_SKIP(selfParser);
                CPPARSEC_SAVE(result, other);

                return result;
                });
        }

        // Parses self and other, returns result of self
        template<typename U>
        Parser<T> skip(Parser<U> other) const {
            return Parser<T>([=, selfParser = *this](InputStream& input) -> ParseResult<T> {
                CPPARSEC_SAVE(result, selfParser);
                CPPARSEC_SKIP(other);

                return result;
                });
        }

        // Parses self and other, returns pair of both results
        template<typename U>
        Parser<std::pair<T, U>> pair_with(Parser<U> other) const {
            return Parser<std::pair<T, U>>([=, selfParser = *this](InputStream& input)->ParseResult<std::pair<T, U>> {
                CPPARSEC_SAVE(a, selfParser);
                CPPARSEC_SAVE(b, other);

                return std::pair(a, b);
            });
        }

        // Parses occurence satisfying a condition
        Parser<T> satisfy(std::function<bool(T)> cond) const {
            return Parser<T>([=, captureParser = parser](InputStream& input) -> ParseResult<T> {
                auto result = captureParser(input);
                CPPARSEC_FAIL_IF(!result || !cond(*result), format("Failed satisfy"));

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
            return Parser<T>([=, captureParser = parser](InputStream& input) -> ParseResult<T> {
                auto starting_input = input;
                if (auto result = captureParser(input)) {
                    return result;
                }
                else if (starting_input.data() != input.data()) {
                    return result;
                }

                return right.parse(input);
                });
        }

        Parser<T> try_() {
            return Parser<T>([=, captureParser = parser](InputStream& input) -> ParseResult<T> {
                auto starting_input = input;
                ParseResult<T> result = captureParser(input);
                if (!result) {
                    input = starting_input; // undo input consumption
                }

                return result;
                });
        }

        // Apply a function to the parse result
        template <typename U>
        auto transform(std::function<U(T)> func) const {
            return Parser<U>([=, captureParser = parser](InputStream& input) -> ParseResult<U> {
                ParseResult<T> result = captureParser(input);
                return result.transform(func);
                });
        }
    };

    template <typename T>
    class _ParserFactory {
    public:
        Parser<T> operator=(Parser<T>::ParseFunction&& parser) {
            return Parser<T>(std::move(parser));
        }
    };

    template <typename C, typename T>
    concept PushBack = requires (C c, T t) {
        { c.push_back(t) } -> std::same_as<void>;
        { c.push_back(std::move(t)) } -> std::same_as<void>;
    };


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

    // Add an error message to the parse result if it fails
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
    template <typename T>
    Parser<T> operator%(Parser<T> p, std::string&& msg) {
        return CPPARSEC_DEFN(T) {
            ParseResult<T> result = CPPARSEC_PARSERESULT(p);
            if (!result.has_value()) {
                result.error() = ParseError(msg);
            }
            return result;
        };
    }


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
                            return std::unexpected(result.error());
                        }
                        //CPPARSEC_FAIL_IF(starting_point != CPPARSEC_GET_INPUT_DATA, result.error());
                        break;
                    }
                }

                return values;
            };
        }
    };

    // Parses any character
    Parser<char> any_char() {
        return Parser<char>([](InputStream& input) -> ParseResult<char> {
            CPPARSEC_FAIL_IF(input.empty(), ParseError("any_char: unexpected end of input"));

            char c = input[0];
            input.remove_prefix(1);
            return c;
            });
    }


    // Concept for a function taking any type and returning a result convertible to bool
    template<typename Pred, typename Arg>
    concept UnaryPredicate = requires(Pred f, Arg a) {
        { f(a) } -> std::convertible_to<bool>;
    };

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

    // Parses a single letter
    Parser<char> letter() {
        return char_satisfy(isalpha, "Expected a letter");
    }

    // Parses a single digit
    Parser<char> digit() {
        return char_satisfy(isdigit, "Expected a digit");
    }

    // Parses a single digit
    Parser<char> digit2() {
        return char_satisfy(isdigit, "");
    }

    // Parses a single space
    Parser<char> space() {
        return char_satisfy(isspace) ^ "Expected a space";
    }

    template <typename T>
    Parser<std::monostate> skip_many(Parser<T>);

    template <typename T>
    Parser<std::monostate> skip_many1(Parser<T>);

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

    // Parses successfully only if the input is empty
    Parser<std::monostate> eof() {
        return CPPARSEC_DEFN(std::monostate) {
            CPPARSEC_FAIL_IF(input.size() > 0, ParseError({ input[0] }, "end of input"));
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

    // A parser which always fails without consuming input
    template <typename T = std::monostate>
    Parser<T> unexpected() {
        return CPPARSEC_DEFN(T) {
            return std::unexpected(ParseError("unexpected!", "unexpected!"));
        };
    }

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

    template<typename T>
    Parser<std::monostate> optional_(Parser<T> p);

    namespace helper {
        template <typename T, typename U, PushBack<T> Container = std::vector<T>>
            requires std::movable<Container>
        Parser<Container> many_till_accumulator(Parser<T> p, Parser<U> end, Container&& init = {}) {
            return CPPARSEC_DEFN(Container) {
                Container values(init);

                while (true) {
                    auto start_point = CPPARSEC_GET_INPUT_DATA;

                    if (CPPARSEC_PARSERESULT(end)) {
                        break; // end parser succeeded, stop accumulating
                    }
                    CPPARSEC_FAIL_IF(start_point != CPPARSEC_GET_INPUT_DATA, ParseError("many_tillfail", "many_tillfail"));

                    if (ParseResult<T> result = CPPARSEC_PARSERESULT(p)) {
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

    // Parses p one or more times until end succeeds, returning the parsed values
    template <typename T, typename U, PushBack<T> Container = std::vector<T>>
    Parser<Container> many1_till(Parser<T> p, Parser<U> end) {
        return CPPARSEC_DEFN(Container) {
            CPPARSEC_SAVE(first, p);
            CPPARSEC_SAVE(values, helper::many_till_accumulator(p, end, { first }));

            return values;
        };
    }

    // Parses p zero or more times until end succeeds, returning the parsed values
    template <typename T, typename U, PushBack<T> Container = std::vector<T>>
    Parser<Container> many_till(Parser<T> p, Parser<U> end) {
        return helper::many_till_accumulator(p, end);
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

    // Parses p zero or more times until end succeeds, returning the parsed values, std::string specialization
    template <typename T, PushBack<char> StringContainer = std::string>
    Parser<StringContainer> many_till(Parser<char> p, Parser<T> end) {
        return helper::many_till_accumulator(p, end, StringContainer());
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
            auto start_point = CPPARSEC_GET_INPUT_DATA;
            ParseResult<T> result = CPPARSEC_PARSERESULT(p);
            CPPARSEC_FAIL_IF(!result && start_point != CPPARSEC_GET_INPUT_DATA, ParseError("optional_result", "optional_result"));

            return (result ? std::optional(result.value()) : std::nullopt);
        };
        // WIP
        //return p.transform([](auto r) { return std::optional(r); }) | std::nullopt;
    }

    // Parse zero or more parses, ignoring the results
    template<typename T>
    Parser<std::monostate> skip_many(Parser<T> p) {
        return CPPARSEC_DEFN(std::monostate) {
            while (true) {
                auto starting_point = CPPARSEC_GET_INPUT_DATA;
                if (!CPPARSEC_PARSERESULT(p)) {
                    CPPARSEC_FAIL_IF(starting_point != CPPARSEC_GET_INPUT_DATA, ParseError("skip_many", "skip_many"));
                    break;
                }
            }
            return std::monostate{}; // Return placeholder for success
        };
    }

    // Parse one or more parses, ignoring the results
    template<typename T>
    Parser<std::monostate> skip_many1(Parser<T> p) {
        return p >> skip_many(p);
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

    // Parse zero or more parses of p separated by sep
    template <typename T, typename U>
    Parser<std::vector<T>> sep_by(Parser<T> p, Parser<U> sep) {
        return sep_by1(p, sep) | success(std::vector<T>());
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

    // Parse zero or more parses of p separated by sep, std::string specialization
    template <typename T>
    Parser<std::string> sep_by(Parser<char> p, Parser<T> sep) {
        return sep_by1(p, sep) | success("");
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

    // Parse occurence between two parses
    // between(open, close, p) is open.with(p).skip(close) is open >> p << close
    template<typename O, typename C, typename T>
    Parser<T> between(Parser<O> open, Parser<C> close, Parser<T> p) {
        //return open >> p << close;
        return CPPARSEC_DEFN(T) {
            CPPARSEC_SKIP(open);
            CPPARSEC_SAVE(middle, p);
            CPPARSEC_SKIP(close);

            return middle;
        };
    }
    template<typename O, typename C, typename T>
    Parser<T> between2(Parser<O> open, Parser<C> close, Parser<T> p) {
        return open >> p << close;
    }

    // return Parser<T>() = [](InputStream& input) -> ParseResult<T> ;
    Parser<std::pair<int, std::string>> cubeParser() {
        return CPPARSEC_DEFN(std::pair<int, std::string>) {
            CPPARSEC_SAVE(cubeNum, int_() << space());
            CPPARSEC_SAVE(cubeColor, string_("red") | string_("green") | string_("blue"));

            return std::pair{ cubeNum, cubeColor };
        };
    }

    // return Parser<T>() = [](InputStream& input) -> ParseResult<T> ;
    Parser<std::tuple<int, std::string>> cubeParser2() {
        return (int_() << space()) & (string_("red") | string_("green") | string_("blue"));
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
            ParseResult<T> result = CPPARSEC_PARSERESULT(p);
            CPPARSEC_SET_INPUT(input_copy);
            CPPARSEC_FAIL_IF(result.has_value(), ParseError("not_followed_by", "not_followed_by"));

            return std::monostate{};
        };
    }

    // Parse one or more left associative applications of op to p, returning the
    // result of the repeated applications. Can be used to parse 1+2+3+4 as ((1+2)+3)+4
    template <typename T>
    Parser<T> chainl1(Parser<T> arg, Parser<std::function<T(T, T)>> op) {
        return CPPARSEC_DEFN(T) {
            CPPARSEC_SAVE(arg1, arg);

            while (true) {
                auto start_point = CPPARSEC_GET_INPUT_DATA;
                ParseResult<std::function<T(T, T)>> f = CPPARSEC_PARSERESULT(op);
                if (!f) {
                    CPPARSEC_FAIL_IF(start_point != CPPARSEC_GET_INPUT_DATA, f.error());
                    break;
                }
                CPPARSEC_SAVE(arg2, arg);

                arg1 = (*f)(arg1, arg2);
            }

            return arg1;
        };
    }
    // Example: chainl(int_(), add_op()).parse("1+2+3+4") -> ((1+2)+3)+4

    // Parse zero or more left associative applications of op to p, returning the
    // result of the repeated applications. If there are zero applications, return backup.
    template <typename T>
    Parser<T> chainl(Parser<T> arg, Parser<std::function<T(T, T)>> op, T backup) {
        return chainl1(arg, op) | success(backup);
    }

    // Parses a sequence of functions, returning on the first successful result.
    template<typename T, std::ranges::input_range Container>
    Parser<T> choice(Container parsers) {
        if (std::ranges::empty(parsers)) {
            return unexpected<T>();
        }

        return std::ranges::fold_left_first(parsers, std::bit_or<>{}).value();
    }

    // Takes a std::function of a parser (not the parser itself) for deferred evaluation
    // Use to avoid infinite cycles in mutual/infinte recursion
    template<typename T>
    Parser<T> lazy(std::function<Parser<T>()> parser_func) {
        return CPPARSEC_DEFN(T) {
            return CPPARSEC_PARSERESULT(parser_func());
        };
    }

    // Takes a function pointer to a parser (not the parser itself) for deferred evaluation
    // Can be used to avoid infinite cycles in mutual recursion
    template<typename T>
    Parser<T> lazy(Parser<T>(*parser_func)()) {
        return CPPARSEC_DEFN(T) {
            return CPPARSEC_PARSERESULT(parser_func());
        };
    }

    //template <typename T, typename Op>
    //Parser<T> chainr1(Parser<T> p, Parser<Op> op, std::function<T(T, Op, T)> func) {
    //    // Helper to build right-associative expression
    //    auto rest = CPPARSEC_DEFN(T) {
    //        return (op >> chainr1(p, op, func)).transform([func](auto&& pair) {
    //            auto [op, b] = pair;
    //            return func(CPPARSEC_RESULT(p), op, b);
    //            }) | success(CPPARSEC_RESULT(p));
    //    };

    //    return p >> rest;
    //} 
    //template <typename T, typename Op>
    //Parser<T> chainr(Parser<T> p, Parser<Op> op, std::function<T(T, Op, T)> func) {
    //    return chainr1(p, op, func) | success(T{}); // Allow empty sequence
    //}

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

    // Parses an int
    Parser<int> int2_() {
        return many1(digit2()).transform<int>(helper::stoi);
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
    // | "or" operator parses the left parser, then the right parser if the left one fails without consuming
    template <typename T>
    Parser<T> operator|(const Parser<T>& left, const Parser<T>& right) {
        return left.or_(right);
    }

    // | "or" operator parses the left parser, then the right parser if the left one fails without consuming
    template <typename T>
    Parser<T> or_(const Parser<T>& left, const Parser<T>& right) {
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
}

template <>
struct std::formatter<cpparsec::ErrorContent> {
    // for debugging only

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
        }, error);
    }
};
