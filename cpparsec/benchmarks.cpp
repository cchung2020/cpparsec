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

Parser<std::vector<int>> spaced_ints() {
    return many(int_().skip(spaces()));
}

Parser<std::vector<int>> spaced_ints2() {
    Parser<int> spaced_int = CPPARSEC_MAKE(Parser<int>) {
        CPPARSEC_SAVE(val, int_());
        CPPARSEC_SKIP(spaces());
        return val;
    };

    return many(spaced_int);
}

void macro_benchmarks() {
    bool _ignore = false;

    ankerl::nanobench::Bench().minEpochIterations(50000).run("spaced_ints_ parser", [&] {
        ParseResult<vector<int>> num = spaced_ints().parse("0 1732 -2783723 1723 -23823 281 +0237 12 2 +23");

        ankerl::nanobench::doNotOptimizeAway(_ignore);
        });
    ankerl::nanobench::Bench().minEpochIterations(50000).run("spaced_ints2 parser", [&] {
        ParseResult<vector<int>> num = spaced_ints2().parse("0 1732 -2783723 1723 -23823 281 +0237 12 2 +23");

        ankerl::nanobench::doNotOptimizeAway(_ignore);
        });
}

void between_benchmarks() {
    bool _ignore = false;
    string input = "x 0 1732 -2783723 1723 -23823 281 +0237 12 2 +23 enxd";
    ankerl::nanobench::Bench().minEpochIterations(20000).run("between spaces/end, spacedint_ parser", [&] {
        ParseResult<vector<int>> num = between(char_('!'), string_("end"), spaced_ints()).parse(input);
        ankerl::nanobench::doNotOptimizeAway(_ignore);
        });
    ankerl::nanobench::Bench().minEpochIterations(20000).run("between spaces/end, spacedint_ parser", [&] {
        ParseResult<vector<int>> num = between(char_('!'), string_("end"), spaced_ints()).parse(input);
        ankerl::nanobench::doNotOptimizeAway(_ignore);
        });
    ankerl::nanobench::Bench().minEpochIterations(20000).run("between spaces/end, spacedint_ parser", [&] {
        ParseResult<vector<int>> num = between(char_('!'), string_("end"), spaced_ints()).parse(input);
        ankerl::nanobench::doNotOptimizeAway(_ignore);
        });

    //ankerl::nanobench::Bench().minEpochIterations(20000).run("between2 spaces/end, spacedint_ parser", [&] {
    //    ParseResult<vector<int>> num = between2(spaces(), string_("end"), spaced_ints()).parse(input);
    //    ankerl::nanobench::doNotOptimizeAway(_ignore);
    //    });
    //ankerl::nanobench::Bench().minEpochIterations(20000).run("between3 spaces/end, spacedint_ parser", [&] {
    //    ParseResult<vector<int>> num = between3(spaces(), string_("end"), spaced_ints()).parse(input);
    //    ankerl::nanobench::doNotOptimizeAway(_ignore);
    //    });

    //println("");
    //ankerl::nanobench::Bench().minEpochIterations(20000).run("between spaces/end, spacedint2_ parser", [&] {
    //    ParseResult<vector<int>> num = between(spaces(), string_("end"), spaced_ints2()).parse(input);
    //    ankerl::nanobench::doNotOptimizeAway(_ignore);
    //    });
    //ankerl::nanobench::Bench().minEpochIterations(20000).run("between2 spaces/end, spacedint2_ parser", [&] {
    //    ParseResult<vector<int>> num = between2(spaces(), string_("end"), spaced_ints2()).parse(input);
    //    ankerl::nanobench::doNotOptimizeAway(_ignore);
    //    });
    //ankerl::nanobench::Bench().minEpochIterations(20000).run("between3 spaces/end, spacedint2_ parser", [&] {
    //    ParseResult<vector<int>> num = between3(spaces(), string_("end"), spaced_ints2()).parse(input);
    //    ankerl::nanobench::doNotOptimizeAway(_ignore);
    //    });
}

int main() {
    between_benchmarks();
}
