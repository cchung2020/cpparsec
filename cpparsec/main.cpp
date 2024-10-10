#include <print>
#include "cpparsec.h"
#include "cpparsec_numeric.h"

using namespace cpparsec;

Parser<std::vector<int>> spaced_ints() {
	return many(int_().skip(spaces()));
}

int main() {
	std::string input = "1 2 3 4 5 6";  // step 1        step 2  
	ParseResult<std::vector<int>> result = spaced_ints().parse(input);
	// step 3
	if (result.has_value()) {
		std::vector<int> nums = result.value();
		for (int n : nums) {
			std::print("{} ", n);
		}
	}
	else {
		println("{}", result.error()().message());
	}
}