#include <print>
#include <chrono>
#include <string>
#include "cpparsec.h"

using namespace cpparsec;
using namespace std;


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

// Parses a single string
Parser<std::string> string6_(const std::string& str) {
    return CPPARSEC_DEFN(std::string) {
        for (auto [i, c] : str | std::views::enumerate) {
            CPPARSEC_FAIL_IF(i < input.size() && c != input[i], format("Expected string \"{}\", got \"{}\"", str, str.substr(0, i)));
        }
        //CPPARSEC_FAIL_IF(input->substr(0, str.size()) != str);
        // non consuming fail model, test efficiency

        return str;
    };
}


// Parses a single string
Parser<std::string> string7_(const std::string& str) {
    return Parser<std::string>([=](InputStream& input) -> ParseResult<std::string> {
        for (auto c : str) {
            CPPARSEC_SKIP(char_(c) ^ "");
        }

        return str;
        });
}
// Parses a single string
Parser<std::string> string8_(const std::string& str) {
    return Parser<std::string>([=](InputStream& input) -> ParseResult<std::string> {
        for (auto c : str) {
            CPPARSEC_SKIP(char_(c) ^ "" ^ "");
        }

        return str;
        });
}

// Parses a single string
Parser<std::string> string9_(const std::string& str) {
    return Parser<std::string>([=](InputStream& input) -> ParseResult<std::string> {
        for (auto c : str) {
            CPPARSEC_SKIP(char_(c) ^ "" ^ "" ^ "");
        }

        return str;
        });
}

void time_parse(Parser<string> p, string&& msg = "") {
	auto start = std::chrono::high_resolution_clock::now();

	for (int i = 0; i < 4000000; i++) {
        //ParseResult<string> result = p.parse("testtesttesttest");
        ParseResult<string> result2 = p.parse("testtesttesttesz");
	}

	auto end = std::chrono::high_resolution_clock::now();

	println("{} {}\n", duration_cast<std::chrono::milliseconds>(end - start), msg);
}


int main() {
    time_parse(string_("testtesttesttest"), "basic extra error");
}
void test1() {

}
