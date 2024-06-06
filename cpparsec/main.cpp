#include <print>
#include <chrono>
#include <string>
#include <utility>
#include "cpparsec.h"
#include "cpparsec_numeric.h"
#include <memory>

using namespace cpparsec;
using namespace std;

//template<typename T>
//concept InputStream = requires(T stream, size_t n) {
//    { stream.empty() } -> std::convertible_to<bool>;
//    { stream.peek() } -> std::same_as<char>;  // Example for a char stream
//    { stream.consume() };
//    { stream.consume(n) };
//};

//template<typename T, InputStream Stream>
//class Parser {
//public:
//    using ParseResult = std::expected<T, std::string>; // Using std::expected for parse results
//    using ParseFunction = std::function<ParseResult(Stream&)>;
//    // Example usage
//    struct StringStream {
//        std::string data;
//        size_t index = 0;

//        bool empty() const { return index >= data.size(); }
//        char peek() const { return data[index]; }
//        void consume() { if (!empty()) index++; }
//    };

//    template<typename StringStream>
//    concept InputStream = requires(StringStream s) {
//        { s.empty() } -> std::convertible_to<bool>;
//        { s.peek() } -> std::same_as<char>;
//        { s.consume() };
//    };

template <std::formattable<char> T>
void printFormatted(const T& value) {
    std::cout << std::format("Formatted output: {}\n", value);
}
struct ErrorContent3
    : std::variant<std::pair<std::string, std::string>, std::pair<char, char>, std::string, std::monostate>
{ };



//// Parses a single string
//Parser<std::string> string2_(const std::string& str) {
//    return Parser<std::string>([=](InputStream& input) -> ParseResult<std::string> {
//        for (auto [i, c] : str | std::views::enumerate) {
//            CPPARSEC_SKIP(char_(c) ^ "");
//        }
//
//        return str;
//        });
//}
//
//
//// Parses a single string
//Parser<std::string> string3_(const std::string& str) {
//    return Parser<std::string>([=](InputStream& input) -> ParseResult<std::string> {
//        for (auto [i, c] : str | std::views::enumerate) {
//            CPPARSEC_SKIP(char_(c));
//        }
//
//        return str;
//        });
//}
//
//// Parses a single string
//Parser<std::string> string4_(const std::string& str) {
//    return Parser<std::string>([=](InputStream& input) -> ParseResult<std::string> {
//        for (auto c : str) {
//            CPPARSEC_SKIP(char_(c));
//        }
//
//        return str;
//        });
//}
//
//// Parses a single string
//Parser<std::string> string5_(const std::string& str) {
//    return Parser<std::string>([=](InputStream& input) -> ParseResult<std::string> {
//        for (auto [i, c] : str | std::views::enumerate) {
//            CPPARSEC_FAIL_IF(i < input.size() && c != input[i], format("Expected string \"{}\", got \"{}\"", str, str.substr(0, i)));
//        }
//        //CPPARSEC_FAIL_IF(input->substr(0, str.size()) != str);
//        // non consuming fail model, test efficiency
//
//        return str;
//        });
//}
//
//// Parses a single string
//Parser<std::string> string6_(const std::string& str) {
//    return CPPARSEC_DEFN(std::string) {
//        for (auto [i, c] : str | std::views::enumerate) {
//            CPPARSEC_FAIL_IF(i < input.size() && c != input[i], format("Expected string \"{}\", got \"{}\"", str, str.substr(0, i)));
//        }
//        //CPPARSEC_FAIL_IF(input->substr(0, str.size()) != str);
//        // non consuming fail model, test efficiency
//
//        return str;
//    };
//}
//
//
//// Parses a single string
//Parser<std::string> string7_(const std::string& str) {
//    return Parser<std::string>([=](InputStream& input) -> ParseResult<std::string> {
//        for (auto c : str) {
//            CPPARSEC_SKIP(char_(c) ^ "");
//        }
//
//        return str;
//        });
//}
//// Parses a single string
//Parser<std::string> string8_(const std::string& str) {
//    return Parser<std::string>([=](InputStream& input) -> ParseResult<std::string> {
//        for (auto c : str) {
//            CPPARSEC_SKIP(char_(c) ^ "" ^ "");
//        }
//
//        return str;
//        });
//}
//
//// Parses a single string
//Parser<std::string> string9_(const std::string& str) {
//    return Parser<std::string>([=](InputStream& input) -> ParseResult<std::string> {
//        for (auto c : str) {
//            CPPARSEC_SKIP(char_(c) ^ "" ^ "" ^ "");
//        }
//
//        return str;
//        });
//}

// Parses a single string
Parser<std::string> string2_(const std::string& str) {
    return CPPARSEC_MAKE(Parser<std::string>) {
        CPPARSEC_FAIL_IF(str.size() > input.size(), ParseError("end of input", { str[0] }));

        for (auto [i, c] : str | std::views::enumerate) {
            if (c != input[i]) {
                char c2 = input[i];
                auto str2 = std::string(input.substr(0, i + 1));
                input.remove_prefix(1);

                CPPARSEC_FAIL(ParseError(c, c2));
            }
            input.remove_prefix(1);
        }

        return str;
    };
}


template <typename T>
void time_parse(Parser<T> p, int cases, string&& test, string&& msg = "") {
    auto start = chrono::high_resolution_clock::now();

    for (int i = 0; i < cases; i++) {
        ParseResult<T> result = p.parse(test);
        //if (!result)
        //println("{}", result.error()().message_stack());
    }

    auto end = chrono::high_resolution_clock::now();

    println("okay:  {} {}", duration_cast<chrono::milliseconds>(end - start), msg);

    start = chrono::high_resolution_clock::now();

    test.back() = 0;
    for (int i = 0; i < cases; i++) {
        ParseResult<T> result = p.parse(test);
    }
    end = chrono::high_resolution_clock::now();
    println("error: {} {}\n", duration_cast<chrono::milliseconds>(end - start), msg);
}

Parser<string> name() {
    return CPPARSEC_MAKE(Parser<string>) {
        CPPARSEC_SAVE(firstLetter, upper());
        CPPARSEC_SAVE(restOfName, many(letter()));

        return string({ firstLetter }) + restOfName;
    };
}

//Parser<string> name() {
//    return CPPARSEC_MAKE(Parser<string>) {
//        char
//    }
//}

//Parser<std::pair<int, std::string>> cube2();

inline Parser<std::string> inefficient_string(const std::string& str) {
    return CPPARSEC_MAKE(Parser<std::string>) {
        for (auto c : str) {
            CPPARSEC_SKIP(char_(c));
        }

        return str;
    };
}

cpparsec::Parser<int> parse_integer() {
    using namespace cpparsec;

    // Parser for a sequence of digits
    auto digits_parser = many1(digit());

    // Transform the parsed sequence of digits (std::string) to an integer

    auto integer_parser = 
        digits_parser.transform<int>([](std::string digits) -> int {
            return std::stoi(digits);
        });

    
    return integer_parser;
}

template <std::integral T>
T add(T a, T b) {
    return a + b;
}


vector<int>* fn() {
    int var;
    return new vector<int>({1,2,3,4});
}

int main() {
    vector<int>* vec2 = fn();

    delete vec2;

    //printFormatted(2);
    //auto x = (cube2() % "cubeParser failed").parse("3 red");
    //if (x) {
    //    auto [num, color] = *x;
    //    println("{} {}", num, color);
    //}
    //else {
    //    println("{}", x.error()().message_stack());
    //}

    //time_parse(string_("a1").or_(string_("a2")).or_(string_("a3")).or_(string_("a4")), 2000000, "a4", "tttt");
    //time_parse(string_("a1").or_(string_("a2")).or_(string_("a4")).or_(string_("a3")), 2000000, "a4", "tttt");
    //time_parse(string_("a1").or_(string_("a4")).or_(string_("a2")).or_(string_("a4")), 2000000, "a4", "tttt");
    //time_parse(string_("a4").or_(string_("a1")).or_(string_("a3")).or_(string_("a4")), 2000000, "a4", "tttt");
    //time_parse(char_('1').or_(char_('2')).or_(char_('3')).or_(char_('4')), 2000000, "4", "4444");
    //time_parse(char_('1').or_(char_('2')).or_(char_('4')).or_(char_('3')), 2000000, "4", "4444");
    //time_parse(char_('1').or_(char_('4')).or_(char_('2')).or_(char_('4')), 2000000, "4", "4444");
    //time_parse(char_('4').or_(char_('1')).or_(char_('3')).or_(char_('4')), 2000000, "4", "4444");

    time_parse(string_("testtesttest"), 2000000, "testtesttest", "str");
    time_parse(inefficient_string("testtesttest"), 2000000, "testtesttest", "badstr");

    //ParseResult<vector<string>> x = many1(string_("test")).parse("testtest");

    //time_parse(int_(), 2000000, "12345123", "regular int, 12345123");
    //time_parse(int_(), 2000000, "a12345123", "regular int, a12345123 (err)");

    //time_parse(string_("tttttwo").or_(string_("ttttthree")), 2000000, "ttttthree", "tttt");
    //time_parse(string_("tttttwo").or_(string_("ttttthree")), 2000000, "tttttwo", "tttt");
    //time_parse(string_("tttt") & string_("tttt"), 2000000, "tttttttt", "tt tt");
    //time_parse(string_("tttttttt"), 2000000, "tttttttt", "tttt");
    //time_parse(string_("tttt") & string_("tttt"), 2000000, "tttttttt", "tt tt");
}

void test1() {

}
