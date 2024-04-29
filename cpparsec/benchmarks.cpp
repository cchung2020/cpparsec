#define ANKERL_NANOBENCH_IMPLEMENT

#include <vector>
#include <string>
#include "Benchmarks/nanobench.h"
#include "cpparsec.h"
#include "cpparsec_numeric.h"

using std::string, std::vector;
using namespace cpparsec;

Parser<vector<string>> string_csv() {
    auto nonCommaChar = [](char c) { return (c != ','); };
    return sep_by1(many(char_satisfy(nonCommaChar)), char_(','));
}

int main() {
    bool _ignore = false;

    string str_csv_input = "a, bc, def, ghij, jklmnop, qrestuvwxyz, dsiadisandiosndioni, daiondidsajhio dhsiofsdhuihrfsdfhdsifhniosdafoisadfni";

    ankerl::nanobench::Bench().run("string parser", [&] {
        ParseResult<string> num = string_("dsavg3@#()HRJNDI").parse("dsavg3@#()HRJNDI");
        ankerl::nanobench::doNotOptimizeAway(_ignore);
        });
    
    ankerl::nanobench::Bench().run("integer parser", [&] {
        ParseResult<int> num = int_().parse("23554567");
        ankerl::nanobench::doNotOptimizeAway(_ignore);
    });

    ankerl::nanobench::Bench().run("integer parser error reporting", [&] {
        ParseResult<int> num = int_().parse("X");
        ankerl::nanobench::doNotOptimizeAway(_ignore);
    });

    ankerl::nanobench::Bench().run("CSV string parser", [&] {
        ParseResult<vector<string>> strs = string_csv().parse(str_csv_input);

        ankerl::nanobench::doNotOptimizeAway(_ignore);
    });
}
