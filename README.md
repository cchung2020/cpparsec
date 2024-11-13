# What is CPParsec?

CPParsec is my senior honors project at the University of Massachusetts Lowell. 

It is an experimental toy C++23 monadic parser-combinator library heavily inspired by [Haskell's Parsec library](https://en.wikipedia.org/wiki/Parsec_(parser)). 

Many of the design decisions are deliberately probing what is possible using the latest features, and are not necessarily the best tool for certain problems. 

# How do I install/compile it?

The project is header only. `#include cpparsec.h` for the base `char`/`std::string` specialized parsers.

It's a header file which simply includes cpparsec_core.h and cpparsec_char.h.

1. cpparsec_core.h: headers for parser combinators 
2. cpparsec_char.h: headers for char/std::string specializations

# How do I use it?

Three steps: 
1. Initialize a parser
2. Parse something with the parser
3. Do something with the result

See this small complete example program:
```C++
#include <print>
#include <vector>
#include <numeric>
#include "cpparsec.h"
#include "cpparsec_numeric.h"

using namespace cpparsec;

// makes a parser which reads multiple ints, ignoring spaces after the first int
Parser<std::vector<int>> spaced_ints() {
	return many(int_().skip(spaces()));
}

int main() {
	std::string input = "1 2 3 4 5 6";
	ParseResult<std::vector<int>> result = spaced_ints().parse(input);
	//                                     ^step 1       ^step 2  
	// step 3
	if (result.has_value()) {
		std::vector<int> nums = result.value();
		int input_sum = std::accumulate(nums.begin(), nums.end(), 0);

		for (int n : nums) {
			std::print("{} ", n);
		}
		std::println("sums to {} ", input_sum);
	}
	else {
		std::println("{}", result.error().message());
	}
}

```
Output: `1 2 3 4 5 6 sums to 21`

See [examples folder](https://github.com/cchung2020/cpparsec/tree/master/cpparsec/examples) and [tests](https://github.com/cchung2020/cpparsec/tree/master/cpparsec/tests) for more usage.

# To do:
* Make error handling more efficient (!)
* Make the library work on other compilers, only works on MSVC currently (!)
* Investigate lowering compile times (!) but I suspect it's impossible without a total rewrite
* Investigate gap between debug and release build, currently up to 150x difference (!)
* Add more benchmarking tests (maybe GitHub actions-supported?)
* Add more tests
* Add more examples

This project is licensed under the terms of the MIT license.
