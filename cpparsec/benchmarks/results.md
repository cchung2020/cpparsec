Benchmark setup:
```cpp
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

int main() {
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
        ParseResult<string> str = string_("dsavg3@#()HRJNDI").parse("dsavg3@#()HRJNDI");
        ankerl::nanobench::doNotOptimizeAway(_ignore);
        });

    ankerl::nanobench::Bench().minEpochIterations(100000).run("inefficient_string parser", [&] {
        ParseResult<string> str = inefficient_string("dsavg3@#()HRJNDI").parse("dsavg3@#()HRJNDI");
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
```

Results (release):

|               ns/op |                op/s |    err% |     total | benchmark
|--------------------:|--------------------:|--------:|----------:|:----------
|               26.39 |       37,896,311.98 |    6.8% |      0.32 | :wavy_dash: `char parser` (Unstable with ~1,085,435.4 iters. Increase `minEpochIterations` to e.g. 10854354)
|               38.51 |       25,968,243.35 |    2.7% |      0.47 | `char parser error reporting`
|            1,802.08 |          554,913.73 |    5.6% |      2.11 | :wavy_dash: `string parser` (Unstable with ~108,543.5 iters. Increase `minEpochIterations` to e.g. 1085435)
|              700.67 |        1,427,202.03 |    4.2% |      0.87 | `inefficient_string parser`
|              704.56 |        1,419,317.86 |    7.1% |      0.88 | :wavy_dash: `integer parser` (Unstable with ~108,543.5 iters. Increase `minEpochIterations` to e.g. 1085435)
|              373.28 |        2,678,959.29 |    6.4% |      0.46 | :wavy_dash: `integer parser error reporting` (Unstable with ~108,543.5 iters. Increase `minEpochIterations` to e.g. 1085435)
|            4,713.92 |          212,137.58 |    2.9% |      1.12 | `CSV string parser`

Confusing results on inefficient_string vs string_, the former runs far slower on my regular timed applications but on the benchmark it's faster for some reason

Results (debug):
|               ns/op |                op/s |    err% |     total | benchmark
|--------------------:|--------------------:|--------:|----------:|:----------
|            2,065.97 |          484,033.68 |    5.7% |     24.29 | :wavy_dash: `char parser` (Unstable with ~1,085,435.4 iters. Increase `minEpochIterations` to e.g. 10854354)
|            2,129.95 |          469,495.35 |    0.6% |     25.55 | `char parser error reporting`
|           13,954.68 |           71,660.53 |    0.6% |     16.68 | `string parser`
|           18,515.63 |           54,008.42 |    0.7% |     22.22 | `inefficient_string parser`
|           35,577.45 |           28,107.69 |    1.2% |     42.46 | `integer parser`
|           21,217.79 |           47,130.27 |    1.1% |     25.90 | `integer parser error reporting`
|          201,239.91 |            4,969.19 |    2.0% |     48.50 | `CSV string parser`

