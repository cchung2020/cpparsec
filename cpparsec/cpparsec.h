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

#define CPPARSEC_CHECK(result) \
    if (!result) { return std::nullopt; }
#define CPPARSEC_RETURN(val) \
    if (input) { return val; } return std::nullopt;
#define CPPARSEC_RETURN_NAMED_INPUT(input, val) \
    if (input) { return val; } return std::nullopt;
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
    Parser<T> between(Parser<O> open, Parser<C> close) {
        return open.with(*this).skip(close);
        //return Parser<T>([=](InputStream& input) -> ParserResult<T> {
        //    open.parse(input);
        //    ParserResult<T> middle = p.parse(input);
        //    close.parse(input);

        //    CPPARSEC_RETURN(middle);
        //});
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


// << "ignore" operator returns the first result of two parsers
// a << b is a.skip(b) 
template <typename T, typename U>
Parser<T> operator<<(const Parser<T> left, const Parser<U> right) {
    return left.skip(right);
}

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

// Parses an integer32
Parser<int> int_();

// Parsers any character
Parser<char> any_char();


template<typename T>
Parser<T> satisfy(const Parser<T>& parser, std::function<bool(T)> cond) {
    return parser.satisfy(cond);
}


// Parsers a single string
Parser<std::string> string_(const std::string& str) {
    return Parser<std::string>([=](InputStream& input) -> ParserResult<std::string> {
        if (input->substr(0, str.size()) == str) {
            return { str };
        }

        return std::nullopt;
        });
}

// Parsers any character
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

// Parsers a single character
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
// Parsers a single letter
Parser<char> letter() {
    return any_char().satisfy(isalpha);
}

// Parsers a single digit
Parser<char> digit() {
    return any_char().satisfy(isdigit);
}

// Parsers a single space
Parser<char> space() {
    return any_char().satisfy(isspace);
}

// Parse zero or more occurrences
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
            if (starting_point != input->data()) {
                return std::nullopt; // consumptive fail, stop parsing
            }
            break;
        }

        CPPARSEC_RETURN({ values });
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
            if (starting_point != input->data()) {
                return std::nullopt; // consumptive fail, stop parsing
            }
            break;
        }

        CPPARSEC_RETURN({ str });
    });
}

// Parse occurence between two parses
// between(open, close, p) is open.with(p).skip(close) is open >> p << close
template<typename O, typename C, typename T>
Parser<T> between(Parser<O> open, Parser<C> close, Parser<T> p) {
    return open >> p << close;
    //return Parser<T>([=](InputStream& input) -> ParserResult<T> {
    //    open.parse(input);
    //    ParserResult<T> middle = p.parse(input);
    //    close.parse(input);

    //    CPPARSEC_RETURN(middle);
    //});
}

Parser<std::string> open_space_close() {
    return Parser<std::string>([](InputStream& input) -> ParserResult<std::string> {
        ParserResult<char> result = char_('c').parse(input);
        ParserResult<std::string> value = many(char_(' ')).parse(input);
        char_(*result).parse(input);

        CPPARSEC_RETURN(*value);
        });
}

Parser<std::pair<int, std::string>> cubeParser() {
    return Parser<std::pair<int, std::string>>([](InputStream& input)
            -> ParserResult<std::pair<int, std::string>> {
        auto cubeNum = int_().parse(input);
        auto cubeColor = string_("red").parse(input);

        if (cubeNum) {
            return { {*cubeNum, *cubeColor } };
        }

        return std::nullopt; // inside the class
        });
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


//// << "ignore" operator returns the first result of two parsers
//// a << b is a.skip(b) 
//template <typename T, typename U>
//Parser<T> operator<<(const Parser<T> left, const Parser<U> right) {
//    return left.skip(right);
//}

// >> "sequence" operator returns the second result of two parsers
// a >> b is a.with(b) 
template <typename T, typename U>
Parser<U> operator>>(const Parser<T> left, const Parser<U> right) {
    return left.with(right);
}

// & "and" operator joins two parses
template <typename T, typename U>
Parser<std::tuple<T, U>> operator&(const Parser<T> left, const Parser<U> right) {
    return Parser<std::tuple<T, U>>([=](InputStream& input)
        -> ParserResult<std::tuple<T, U>> {
        auto a = left.parse(input);
        auto b = right.parse(input);

        if (a && b) {
            return { std::make_tuple(*a, *b) };
        }

        return std::nullopt;
    });
}

// & "and" operator joins a parse and multiple parses
template<typename T, typename... Ts>
Parser<std::tuple<T, Ts...>> operator&(const Parser<T>& left, const Parser<std::tuple<Ts...>>& right) {
    return Parser<std::tuple<T, Ts...>>([=](InputStream& input) -> ParserResult<std::tuple<T, Ts...>> {
        auto a = left.parse(input);
        auto bs = right.parse(input);

        if (a && bs) {
            return { std::tuple_cat(std::make_tuple(*a), *bs) };
        }

        return std::nullopt;
    });
}

// & "and" operator joins multiple parses and a parse 
template<typename T, typename... Ts>
Parser<std::tuple<T, Ts...>> operator&(const Parser<std::tuple<Ts...>>& left, const Parser<T>& right) {
    return Parser<std::tuple<T, Ts...>>([=](InputStream& input) -> ParserResult<std::tuple<T, Ts...>> {
        auto as = left.parse(input);
        auto b = right.parse(input);

        if (as && b) {
            return { std::tuple_cat(*as, std::make_tuple(*b)) };
        }

        return std::nullopt;
    });
}

// & "and" operator joins multiple parses and multiple parses
template<typename... Ts, typename... Us>
Parser<std::tuple<Ts..., Us...>> operator&(const Parser<std::tuple<Ts...>>& left, const Parser<std::tuple<Us...>>& right) {
    return Parser<std::tuple<Ts..., Us...>>([=](InputStream& input) -> ParserResult<std::tuple<Ts..., Us...>> {
        auto as = left.parse(input);
        auto bs = right.parse(input);

        if (as && bs) {
            return { std::tuple_cat(*as,*bs) };
        }

        return std::nullopt;
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
//
//// Parse zero or more occurrences
//template<typename T>
//Parser<std::vector<T>> many(Parser<T> p) {
//    return [p](const std::string& input) -> ParserResult<std::vector<T>> {
//        std::vector<T> values;
//        std::string remaining = input;
//        while (true) {
//            auto result = p(remaining);
//            if (!result) break;
//            values.push_back(result->first);
//            remaining = result->second;
//        }
//        return { {values, remaining} };
//        };
//}
//
//// Parse one or more occurrences
//template<typename T>
//Parser<std::vector<T>> many1(Parser<T> p) {
//    return [p](const std::string& input) -> ParserResult<std::vector<T>> {
//        std::vector<T> values;
//        std::string remaining = input;
//
//        ParserResult<T> result = p(remaining);
//        if (!result) {
//            return std::nullopt;
//        }
//        
//        while (result) {
//            values.push_back(result->first);
//            remaining = result->second;
//            result = p(remaining);
//        }
//
//        return { {values, remaining} };
//    };
//}
//
//// Parse occurence between two parses
//template<typename O, typename C, typename T>
//Parser<T> between(Parser<O> open, Parser<C> close, Parser<T> p) {
//    return [open, close, p](const std::string& input) -> ParserResult<T> {
//        std::string remaining = input;
//
//        //T value;
//        //ParserResult<T> result = open(remaining)
//        //    .transform([&remaining, p](ParserResult<O> _o) {
//        //        remaining = _o->second;
//        //        return p(remaining);
//        //    })
//        //    .transform([&remaining, &value, close](ParserResult<T> _p) {
//        //        value = _p->first;
//        //        remaining = _p->second;
//        //        return close(remaining);
//        //    })
//        //    .transform([&remaining, &value](ParserResult<C> _c) {
//        //        remaining = _c->second;
//        //        return { {value, remaining} };
//        //    });
//        // 
//        //return result;
//
//        ParserResult<O> result = open(remaining);
//        if (!result) return std::nullopt;
//        remaining = result->second;
//        
//        ParserResult<T> result2 = p(remaining);
//        if (!result2) return std::nullopt;
//        T betweenValue = result2->first;
//        remaining = result2->second;
//        
//        ParserResult<C> result3 = close(remaining);
//        if (!result3) return std::nullopt;
//        remaining = result3->second;
//        
//        return { {betweenValue, remaining} };
//    };

#endif /* CPPARSEC_H */
