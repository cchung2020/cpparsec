#include <print>
#include <vector>
#include <numeric>
#include "cpparsec.h"
#include "cpparsec_numeric.h"

using namespace cpparsec;

// building a parser which reads ints and skips spaces after the first int
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
