#define ANKERL_NANOBENCH_IMPLEMENT

#include <vector>
#include <string>
#include "Benchmarks/nanobench.h"
#include "cpparsec.h"
#include "cpparsec_numeric.h"

using std::string, std::vector;
using namespace cpparsec;

inline Parser<std::string> inefficient_string(const std::string& str) {
    return CPPARSEC_MAKE(Parser<std::string>) {
        for (auto c : str) {
            CPPARSEC_SKIP(char_(c));
        }

        return str;
    };
}

Parser<vector<string>> string_csv() {
    auto nonCommaChar = [](char c) { return (c != ','); };
    return sep_by1(many(char_satisfy(nonCommaChar)), char_(','));
}

void benchmark1() {
    bool _ignore = false;

    ankerl::nanobench::Bench().minEpochIterations(1000000).run("char parser", [&] {
        ParseResult<char> num = char_('x').parse("x");
        ankerl::nanobench::doNotOptimizeAway(_ignore);
        });

    ankerl::nanobench::Bench().minEpochIterations(1000000).run("char parser error reporting", [&] {
        ParseResult<char> num = char_('x').parse("y");
        ankerl::nanobench::doNotOptimizeAway(_ignore);
        });

    ankerl::nanobench::Bench().minEpochIterations(100000).run("string parser", [&] {
        ParseResult<string> str = string_("longstringtesterjontester").parse("longstringtesterjontester");
        ankerl::nanobench::doNotOptimizeAway(_ignore);
        });

    ankerl::nanobench::Bench().minEpochIterations(100000).run("inefficient_string parser", [&] {
        ParseResult<string> str = inefficient_string("longstringtesterjontester").parse("longstringtesterjontester");
        ankerl::nanobench::doNotOptimizeAway(_ignore);
        });

    ankerl::nanobench::Bench().minEpochIterations(100000).run("integer parser", [&] {
        ParseResult<int> num = int_().parse("23554567");
        ankerl::nanobench::doNotOptimizeAway(_ignore);
        });

    ankerl::nanobench::Bench().minEpochIterations(100000).run("integer parser error reporting", [&] {
        ParseResult<int> num = int_().parse("X");
        ankerl::nanobench::doNotOptimizeAway(_ignore);
        });

    string str_csv_input = "a, bc, def, ghij, jklmnop, qrestuvwxyz, dsiadisandiosndioni, daiondidsajhio dhsiofsdhuihrfsdfhdsifhniosdafoisadfni";

    ankerl::nanobench::Bench().minEpochIterations(20000).run("CSV string parser", [&] {
        ParseResult<vector<string>> strs = string_csv().parse(str_csv_input);

        ankerl::nanobench::doNotOptimizeAway(_ignore);
        });
}

void int_benchmarks();

void between_benchmarks() {
    bool _ignore = false;

    ankerl::nanobench::Bench().minEpochIterations(100000).run("between spaces, int_ parser", [&] {
        ParseResult<int> num = between(space(), space(), int_()).parse(" 123 ");
        ankerl::nanobench::doNotOptimizeAway(_ignore);
        });
    ankerl::nanobench::Bench().minEpochIterations(100000).run("between spaces2, int_ parser", [&] {
        ParseResult<int> num = between2(space(), space(), int_()).parse(" 123 ");
        ankerl::nanobench::doNotOptimizeAway(_ignore);
        });
    ankerl::nanobench::Bench().minEpochIterations(100000).run("between spaces3, int_ parser", [&] {
        ParseResult<int> num = between3(space(), space(), int_()).parse(" 123 ");
        ankerl::nanobench::doNotOptimizeAway(_ignore);
        });
}

int main() {
    between_benchmarks();
}
