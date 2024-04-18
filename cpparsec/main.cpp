#include <print>
#include <chrono>
#include <string>
#include "cpparsec.h"

using namespace cpparsec;
using namespace std;

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
    return CPPARSEC_DEFN(std::string) {
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
        if (!result)
        {
            //result.error()().message_stack();
            //if (i == 0) {
            //    println("{}", result.error()().message_stack());
            //}
        }
    }
    end = chrono::high_resolution_clock::now();
    println("error: {} {}\n", duration_cast<chrono::milliseconds>(end - start), msg);
}

int main() {
    //auto p = string_("exam");

    //auto result = p.parse("exadd");
    //if (result) {
    //    println("{}", *result);
    //}
    //else {
    //    ParseError x = result.error()();
    //    println("{}", result.error()().message_stack());
    //}

    //time_parse(many(between(char_('a'), char_('c'), char_('b'))), 2000000, "abbbbbc", "between ab...c");
    //time_parse(many(between2(char_('a'), char_('c'), char_('b'))), 2000000, "abbbbbc", "between ab...c");
    //time_parse(int_(), 200000, "12345123", "regular int, 12345123");
    //time_parse(int_(), 200000, "a12345123", "regular int, a12345123 (err)");
    //time_parse(int2_(), 200000, "12345123", "fast int, 12345123");
    //time_parse(int2_(), 200000, "a12345123", "fast int, a12345123 (err)");
    //if (auto result = int_().parse("a12345123"); !result) {
    //    println("{} [fast int]\n", result.error()().message_stack());
    //}
    //if (auto result = int2_().parse("a12345123"); !result) {
    //    println("{}", result.error()().message_stack());
    //}
    //time_parse(int_(), 200000, "a12345123", "tttterr");
    //time_parse(int2_(), 200000, "12345123", "2tttt");
    //time_parse(int2_(), 200000, "a12345123", "2tttterr");
    // 
    time_parse(string_("tttttttt"), 2000000, "tttttttt", "tttt");
    time_parse(string2_("tttttttt"), 2000000, "tttttttt", "2tttt");
    time_parse(string_("tttt") & string_("tttt"), 2000000, "tttttttt", "tt tt");
    time_parse(string2_("tttt") & string2_("tttt"), 2000000, "tttttttt", "2tt tt");
    time_parse(string_("tt") & string_("tt") & string_("tt") & string_("tt"), 2000000, "tttttttt", "t t t t");
    time_parse(string2_("tt") & string2_("tt") & string2_("tt") & string2_("tt"), 2000000, "tttttttt", "2t t t t");
    time_parse(char_('t') & char_('t') & char_('t') & char_('t') & char_('t') & char_('t') & char_('t') & char_('t'), 2000000, "tttttttt", "char t t t t");
}

void test1() {

}
