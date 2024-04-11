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
#include <utility>

#define CPPARSEC_SAVE(var, ...)                    \
    auto _##var##_ = (__VA_ARGS__).parse(input);   \
    if (!_##var##_.has_value()) {                  \
        return std::unexpected(_##var##_.error()); \
    }                                              \
    auto var = _##var##_.value();

#define CPPARSEC_SKIP(p) \
    if (auto _cpparsec_skipresult = (p).parse(input); !_cpparsec_skipresult) { \
        return std::unexpected(_cpparsec_skipresult.error());                  \
    }                                                                          \

#define CPPARSEC_FAIL_IF(cond, message) if (cond) { return std::unexpected(message); }

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
    template<typename T>
    using ParseResult = std::expected<T, std::string>;

    using InputStream = std::string_view;

    template<typename T>
    class Parser {
    public:
        using ParseFunction = std::function<ParseResult<T>(InputStream&)>;

    private:
        ParseFunction parser;

    public:
        Parser(ParseFunction parser) :
            parser(parser)
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
                CPPARSEC_SAVE(result, other)

                    return result;
                });
        }

        // Parses self and other, returns result of self
        template<typename U>
        Parser<T> skip(Parser<U> other) const {
            return Parser<T>([=, selfParser = *this](InputStream& input) -> ParseResult<T> {
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
            return Parser<T>([=, captureParser = parser](InputStream& input) -> ParseResult<T> {
                auto result = captureParser(input);
                CPPARSEC_FAIL_IF(!result || !cond(*result), format("Failed satisfy"));

                return result;
                });
        }
        // Parse occurence between two parses
        template<typename O, typename C>
        Parser<T> between(Parser<O> open, Parser<C> close) const {
            return open.with(*this).skip(close) ^ "Between failed";
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
                if (ParseResult<T> result = captureParser(input)) {
                    return result;
                }

                input = starting_input; // undo input consumption
                CPPARSEC_FAIL_IF(true, "try_ ok");
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
        Parser<T> operator=(Parser<T>::ParseFunction parser) {
            return Parser<T>(parser);
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
    Parser<T> operator^(Parser<T> p, std::string&& msg) {
        return CPPARSEC_DEFN(T) {
            ParseResult<T> result = CPPARSEC_PARSERESULT(p);
            if (!result.has_value()) {
                return std::unexpected(format("{}. {}", result.error(), msg));
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
                return std::unexpected(msg);
            }
            return result;
        };
    }


    // Parses a single character
    Parser<char> char_(char c) {
        return CPPARSEC_DEFN(char) {
            CPPARSEC_FAIL_IF(input.empty(), format("Unexpected end of input, expected '{}'", c));
            CPPARSEC_FAIL_IF(input[0] != c, format("Unexpected '{}' instead of '{}'", input[0], c));

            input.remove_prefix(1);
            return c;
        };
    }

    // Parses a single string
    Parser<std::string> string_(const std::string& str) {
        return Parser<std::string>([=](InputStream& input) -> ParseResult<std::string> {
            for (auto [i, c] : str | std::views::enumerate) {
                CPPARSEC_SKIP(char_(c) ^ format("Expected string \"{}\", got \"{}\"", str, str.substr(0,i)));
            }
            //CPPARSEC_FAIL_IF(input->substr(0, str.size()) != str);
            // non consuming fail model, test efficiency

            return str;
            });
    }

    // Parses a single string
    Parser<std::string> string2_(const std::string& str) {
        return Parser<std::string>([=](InputStream& input) -> ParseResult<std::string> {
            for (auto [i, c] : str | std::views::enumerate) {
                CPPARSEC_SKIP(char_(c) ^ "");
            }

            return str;
            });
    }


    // Parses a single string
    Parser<std::string> string3_(const std::string& str) {
        return Parser<std::string>([=](InputStream& input) -> ParseResult<std::string> {
            for (auto [i, c] : str | std::views::enumerate) {
                CPPARSEC_SKIP(char_(c));
            }

            return str;
            });
    }

    // Parses a single string
    Parser<std::string> string4_(const std::string& str) {
        return Parser<std::string>([=](InputStream& input) -> ParseResult<std::string> {
            for (auto c : str) {
                CPPARSEC_SKIP(char_(c));
            }

            return str;
            });
    }

    // Parses a single string
    Parser<std::string> string5_(const std::string& str) {
        return Parser<std::string>([=](InputStream& input) -> ParseResult<std::string> {
            for (auto [i, c] : str | std::views::enumerate) {
                CPPARSEC_FAIL_IF(i < input.size() && c != input[i], format("Expected string \"{}\", got \"{}\"", str, str.substr(0, i)));
            }
            //CPPARSEC_FAIL_IF(input->substr(0, str.size()) != str);
            // non consuming fail model, test efficiency

            return str;
            });
    }

    namespace helper {
        template <typename T, PushBack<T> Container = std::vector<T>>
            requires std::movable<Container>
        Parser<Container> many_accumulator(Parser<T> p, Container&& init = {}) {
            return CPPARSEC_DEFN(Container) {
                Container values(init);

                while (true) {
                    auto starting_point = CPPARSEC_GET_INPUT_DATA;
                    if (ParseResult<T> result = CPPARSEC_PARSERESULT(p)) {
                        values.push_back(*result);
                        continue;
                    }
                    // consumptive fail, stop parsing
                    CPPARSEC_FAIL_IF(starting_point != CPPARSEC_GET_INPUT_DATA, "many_accumulator failed");
                    break;
                }

                return values;
            };
        }
    };

    // Parses any character
    Parser<char> any_char() {
        return Parser<char>([](InputStream& input) -> ParseResult<char> {
            CPPARSEC_FAIL_IF(input.empty(), "Unexpected end of input, expected any_char");

            char c = input[0];
            input.remove_prefix(1);
            return c;
            });
    }

    // Parses a single character that satisfies a constraint
    // Faster than try_(any_char().satisfy(cond))
    Parser<char> char_satisfy(std::function<bool(char)> cond) {
        return CPPARSEC_DEFN(char) {
            CPPARSEC_FAIL_IF(input.empty(), "Unexpected end of input, failed satisfy");
            CPPARSEC_FAIL_IF(!cond(input[0]), std::format("Unexpected '{}' failed satisfy", input[0]));

            char c = input[0];
            input.remove_prefix(1);
            return c;
        };
    }

    // Parses a single letter
    Parser<char> letter() {
        return char_satisfy(isalpha) ^ "Expected a letter";
    }

    // Parses a single digit
    Parser<char> digit() {
        return char_satisfy(isdigit) ^ "Expected a digit";
    }

    // Parses a single space
    Parser<char> space() {
        return char_satisfy(isspace) ^ "Expected a space";
    }

    template <typename T>
    Parser<std::monostate> skip_many(Parser<T>);

    template <typename T>
    Parser<std::monostate> skip_many1(Parser<T>);

    // Parses zero or more spaces
    Parser<std::monostate> spaces() {
        return skip_many(space());
    }

    // Parses one or more spaces
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
            CPPARSEC_FAIL_IF(input.size() > 0, format("Expected end of input, found {}", input[0]));
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
            return std::unexpected("unexpected!");
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
            CPPARSEC_SAVE(values, helper::many_accumulator(charP, std::string(1, first)));

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
                    CPPARSEC_FAIL_IF(start_point != CPPARSEC_GET_INPUT_DATA, "many_tillfail");

                    if (ParseResult<T> result = CPPARSEC_PARSERESULT(p)) {
                        values.push_back(*result);
                        continue;
                    }

                    // neither end nor p parsed successfully, fail
                    CPPARSEC_FAIL_IF(true, "many_tillfail");
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
            CPPARSEC_FAIL_IF(!result && start_point != CPPARSEC_GET_INPUT_DATA, "optional_result_fail");

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
                    CPPARSEC_FAIL_IF(starting_point != CPPARSEC_GET_INPUT_DATA, "skip_many failed");
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
        //DEFN(T)
        return CPPARSEC_DEFN(T) {
            CPPARSEC_SKIP(open);
            CPPARSEC_SAVE(middle, p);
            CPPARSEC_SKIP(close);

            return middle;
        } ^ "Between fail";
    }

    // return Parser<T>() = [](InputStream& input) -> ParseResult<T> ;
    Parser<std::pair<int, std::string>> cubeParser() {
        return CPPARSEC_DEFN(std::pair<int, std::string>) {
            CPPARSEC_SAVE(cubeNum, int_() << space());
            CPPARSEC_SAVE(cubeColor, string_("red") | string_("green") | string_("blue"));

            return std::pair{ cubeNum, cubeColor };
        };
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
            CPPARSEC_FAIL_IF(result.has_value(), "not followed by failed");

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
                    CPPARSEC_FAIL_IF(start_point != CPPARSEC_GET_INPUT_DATA, "chainl1 failed");
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
        return Parser<std::tuple<T, U>>([=](InputStream& input) -> ParseResult<std::tuple<T, U>> {
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

    // | "or" operator parses the left parser, then the right parser if the left one fails without consuming
    template <typename T>
    Parser<T> or_(const Parser<T>& left, const Parser<T>& right) {
        return left.or_(right);
    }

    // & "and" operator joins a parse and multiple parses
    template<typename T, typename... Ts>
    Parser<std::tuple<T, Ts...>> operator&(const Parser<T>& left, const Parser<std::tuple<Ts...>>& right) {
        return Parser<std::tuple<T, Ts...>>([=](InputStream& input) -> ParseResult<std::tuple<T, Ts...>> {
            CPPARSEC_SAVE(a, left);
            CPPARSEC_SAVE(bs, right);

            return { std::tuple_cat(std::make_tuple(a), bs) };
            });
    }

    // & "and" operator joins multiple parses and a parse 
    template<typename T, typename... Ts>
    Parser<std::tuple<T, Ts...>> operator&(const Parser<std::tuple<Ts...>>& left, const Parser<T>& right) {
        return Parser<std::tuple<T, Ts...>>([=](InputStream& input) -> ParseResult<std::tuple<T, Ts...>> {
            CPPARSEC_SAVE(as, left);
            CPPARSEC_SAVE(b, right);

            return { std::tuple_cat(as, std::make_tuple(b)) };
            });
    }

    // & "and" operator joins multiple parses and multiple parses
    template<typename... Ts, typename... Us>
    Parser<std::tuple<Ts..., Us...>> operator&(const Parser<std::tuple<Ts...>>& left, const Parser<std::tuple<Us...>>& right) {
        return Parser<std::tuple<Ts..., Us...>>([=](InputStream& input) -> ParseResult<std::tuple<Ts..., Us...>> {
            CPPARSEC_SAVE(as, left);
            CPPARSEC_SAVE(bs, right);

            return { std::tuple_cat(as,bs) };
            });
    }
}
