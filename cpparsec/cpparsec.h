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

using std::print, std::println, std::cout;

// #include <string_view>
// #include <expected>

namespace cpparsec {

// A parser result can either be successful (carrying the parsed value and remaining input)
// or a failure (optionally carrying an error message).
template<typename T>
using ParserResult = std::optional<T>;

// using InputStream = std::optional<std::string_view>;

template<typename T>
class Parser {
public:
    using ParseFunction = std::function<ParserResult<T>(std::string_view&)>;

private:
    std::shared_ptr<ParseFunction> parser;
    bool valid;
    ParserResult<T> result;

public:
    Parser(ParseFunction parser) :
        parser(std::make_shared<ParseFunction>(parser)),
        valid(true) 
    { }

    // Executes the parser, modifying InputStream(implementation only)
    ParserResult<T> parse(std::string_view& input) const {
        return (*parser)(input);
    }

    // Top level parser execution, parses a string
    ParserResult<T> parse(const std::string& input) const {
        std::string_view view = input;
        return (*parser)(view);
    }

    //ParserResult<T> parse(ParseFunction p, std::string_view& input) const {
    //    if (valid) {
    //        ParseResult result = (*parser)(view));
    //        valid = result != std::nullopt;
    //        return result;
    //    }
    //    return std::nullopt;
    //}

    // with combinator
    template<typename U>
    Parser<U> with(Parser<U> other) const {
        return Parser<U>([=, captureParser = parser](std::string_view& input) -> ParserResult<U> {
            if (ParserResult result = this->parse(input)) {
                return { other.parse(input) };
            }
            return std::nullopt;
            });
    }

    // Static method to create a parser for a single character
    static Parser<char> character(char c) {
        return Parser<char>([c](std::string_view& input) -> ParserResult<char> {
            if (!input.empty() && input[0] == c) {
                input = input.substr(1);
                return { c };
            }
            return std::nullopt;
            });
    }


};

// Parses an integer32
Parser<int> integer();

Parser<char> character(char c) {
    return Parser<char>::character(c);
}

// Parse zero or more occurrences
template<typename T>
Parser<std::vector<T>> many(Parser<T> p) {
    return Parser<std::vector<T>>([=](std::string_view& input) -> ParserResult<std::vector<T>> {
        std::vector<T> values;
        while (ParserResult<T> result = p.parse(input)) {
            values.push_back(*result);
        }
        return { values };
        });
}

// Parse occurence between two parses
template<typename O, typename C, typename T>
Parser<T> between(Parser<O> open, Parser<C> close, Parser<T> p) {
    return Parser<T>([open, close, p](std::string_view& input) -> ParserResult<T> {
        ParserResult<O> first = open.parse(input);
        ParserResult<T> middle = p.parse(input);
        ParserResult<C> last = close.parse(input);

        if (first && last) {
            return middle;
        }

        return std::nullopt;
        });
}

// Parse occurence between two parses - short circuit abuse
template<typename O, typename C, typename T>
Parser<T> between2(Parser<O> open, Parser<C> close, Parser<T> p) {
    return Parser<T>([open, close, p](std::string_view& input) -> ParserResult<T> {
        ParserResult<T> middle;

        if (open.parse(input)
            && (middle = p.parse(input))
            && close.parse(input)) {
            return middle;
        }

        return std::nullopt; // inside the class
        });
}

Parser<std::pair<int, std::string>> cubeParser() {
    return Parser<std::pair<int, std::string>>([](std::string_view& input) {
        ParserResult<int> cubeNum = integer().parse(input);
        ParserResult<int> cubeNum2 = integer().parse(input);

        //if (cubeNum && cubeNum2) {
        //    return { {*cubeNum, ""} };
        //}

        return std::nullopt; // inside the class
        });
}

// Parse occurence between two parses - short circuit abuse
template<typename O, typename C, typename T>
Parser<T> between3(Parser<O> open, Parser<C> close, Parser<T> p) {
    return Parser<T>([open, close, p](std::string_view& input) -> ParserResult<T> {
        ParserResult<T> result; // inside the class
        bool valid; // inside the class
        ParserResult<T> middle;

        if (open.parse(input)
            && (middle = p.parse(input))
            && close.parse(input)) {
            result = middle; // assigning to inside the class
        }

        return result; // inside the class
        });
}

};

/*
    ============ NUMERIC PROCESSORS ============
*/
using namespace cpparsec;
// Parse occurence between two parses - short circuit abuse

Parser<int> integer() {
    return Parser<int>([](std::string_view& input) -> ParserResult<int> {
        ParserResult<int> result = many(character('1')).parse(input)
            .transform([](std::vector<char> digits) {
                std::string numStr(digits.begin(), digits.end());
                return std::stoi(numStr);
            });

        return result;
    });
}

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