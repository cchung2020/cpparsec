#include <print>
#include <chrono>
#include <string>
#include "cpparsec.h"

using namespace cpparsec;
using namespace std;

void time_parse(Parser<string> p, string&& msg = "") {
	auto start = std::chrono::high_resolution_clock::now();

	for (int i = 0; i < 100000; i++) {
		ParseResult<string> result = p.parse("test");
		ParseResult<string> result2 = p.parse("tesz");
	}

	auto end = std::chrono::high_resolution_clock::now();

	println("{} {}\n", duration_cast<std::chrono::milliseconds>(end - start), msg);
}

int main() {
	time_parse(string_("test"), "basic extra error");
	time_parse(string2_("test"), "empty extra error");
	time_parse(string3_("test"), "no extra error");
	time_parse(string4_("test"), "no extra error, no std::enumerate");
	time_parse(string5_("test"), "extra error, fail directly instead of ^");
}
