#define BOOST_TEST_MODULE cpparsec
#include <boost/test/included/unit_test.hpp>
#include "cpparsec.h"
#include <algorithm>

using namespace cpparsec;
using std::string, std::string_view, std::vector, std::tuple, std::optional;
using std::ranges::all_of;

BOOST_AUTO_TEST_SUITE(test1_suite)

BOOST_AUTO_TEST_CASE(Char_Parser)
{
    string inputStr = "aabab";
    string_view input = inputStr;

    Parser<char> a = char_('a');
    ParserResult<char> result1 = a.parse(input);

    BOOST_REQUIRE(result1.has_value());
    BOOST_CHECK(*result1 == 'a');
    BOOST_CHECK(input == "abab");

    BOOST_CHECK(!char_('A').parse(input).has_value());
    BOOST_CHECK(input == "abab");

    Parser<tuple<char, char>> a_and_b = char_('a') & (char_('b'));
    ParserResult result2 = a_and_b.parse(input);

    BOOST_REQUIRE(result2.has_value());
    BOOST_CHECK(*result2 == tuple('a', 'b'));
    BOOST_CHECK(input == "ab");
    BOOST_CHECK(!eof().parse(input).has_value());

    Parser<char> a_with_b = a.with(char_('b'));
    auto result3 = a_with_b.parse(input);

    BOOST_REQUIRE(result3.has_value());
    BOOST_CHECK(*result3 == 'b');
    BOOST_CHECK(input == "");
    BOOST_CHECK(eof().parse(input).has_value());
}

BOOST_AUTO_TEST_CASE(Between_Parser)
{
    string inputStr = "xyz";
    string_view input = inputStr;

    ParserResult<char> result1 = char_('y').between(char_('x'), char_('z')).parse(input);

    BOOST_REQUIRE(result1.has_value());
    BOOST_CHECK(*result1 == 'y');
    BOOST_CHECK(eof().parse(input).has_value());

    ParserResult<char> result2 = char_('Y').between(char_('x'), char_('z')).parse("xyz");
    BOOST_REQUIRE(!result2.has_value());
}

BOOST_AUTO_TEST_CASE(Many_Parser)
{
    string inputStr = "53242k";
    string_view input = inputStr;

    ParserResult<string> result1 = many(digit()).parse(input);

    BOOST_REQUIRE(result1.has_value());
    BOOST_CHECK(*result1 == "53242");
    BOOST_CHECK(input == "k");

    inputStr = "xyxyxyEND";
    input = inputStr;

    auto xy = char_('x') & char_('y');
    auto is_xy = [](auto xy) { return xy == tuple('x', 'y'); };
    ParserResult<vector<tuple<char, char>>> result2 = many(xy).parse(input);

    BOOST_REQUIRE(result2.has_value());
    BOOST_CHECK(result2->size() == 3);
    BOOST_CHECK(all_of(*result2, is_xy));
    BOOST_CHECK(input == "END");

    inputStr = "xyxyxyxZ";
    input = inputStr;

    ParserResult<vector<tuple<char, char>>> result3 = many(xy).parse(input);

    BOOST_REQUIRE(!result3.has_value());
    BOOST_CHECK(input == "Z");
    // very debatable for now, consumptive failing input behavior is unsettled

    inputStr = "HELLOworld";
    input = inputStr;

    ParserResult<string> result4 = many1(upper()).parse(input);

    BOOST_REQUIRE(result4.has_value());
    BOOST_CHECK(*result4 == "HELLO");
    BOOST_CHECK(input == "world");

    inputStr = "helloWORLD";
    input = inputStr;

    ParserResult<string> result5 = many1(upper()).parse(input);

    BOOST_REQUIRE(!result5.has_value());
}

BOOST_AUTO_TEST_CASE(Try_Or_Parser)
{
    string inputStr = "ab";
    string_view input = inputStr;

    auto a_or_b = char_('a') | char_('b');

    ParserResult<char> result1 = a_or_b.parse(input);

    BOOST_REQUIRE(result1.has_value());
    BOOST_CHECK(*result1 == 'a');
    BOOST_CHECK(input == "b");

    inputStr = "ba";
    input = inputStr;

    ParserResult<char> result2 = a_or_b.parse(input);

    BOOST_REQUIRE(result2.has_value());
    BOOST_CHECK(*result2 == 'b');
    BOOST_CHECK(input == "a");

    auto two = string_("two");
    auto three = string_("three");
    auto two_or_three = two | three;

    ParserResult<string> result3 = two_or_three.parse("two");

    BOOST_REQUIRE(result3.has_value());
    BOOST_CHECK(*result3 == "two");

    ParserResult<string> result4 = two_or_three.parse("three");

    BOOST_REQUIRE(!result4.has_value());

    auto try_two_or_three = try_(two) | three;

    ParserResult<string> result5 = try_two_or_three.parse("two");

    BOOST_REQUIRE(result5.has_value());
    BOOST_CHECK(*result5 == "two");

    ParserResult<string> result6 = try_two_or_three.parse("three");

    BOOST_REQUIRE(result6.has_value());
    BOOST_CHECK(*result6 == "three");
}

BOOST_AUTO_TEST_CASE(Int_Parser)
{
    ParserResult<int> result1 = int_().parse("1");

    BOOST_REQUIRE(result1.has_value());
    BOOST_CHECK(*result1 == 1);

    ParserResult<tuple<int, int>> result2 = (int_() << space() & int_()).parse("25 105");

    BOOST_REQUIRE(result2.has_value());
    BOOST_CHECK(*result2 == tuple(25, 105));

    ParserResult<int> result3 = (int_()).parse("a500");

    BOOST_REQUIRE(!result3.has_value());

    ParserResult<vector<int>> result4 = many1(int_() << (optional_(space()))).parse("1 2 3 4 5");

    BOOST_REQUIRE(result4.has_value());
    BOOST_CHECK(*result4 == vector({ 1, 2, 3, 4, 5 }));

}

BOOST_AUTO_TEST_CASE(Optional_Result_Parser)
{
    Parser<optional<int>> opt_int = optional_result(int_());
    ParserResult<optional<int>> result1 = opt_int.parse("123");

    BOOST_REQUIRE(result1.has_value());
    BOOST_CHECK(*result1 == optional(123));

    ParserResult<optional<int>> result2 = opt_int.parse("X");

    BOOST_REQUIRE(result2.has_value());
    BOOST_CHECK(*result2 == std::nullopt);

    // one more

}
BOOST_AUTO_TEST_SUITE_END()

//BOOST_AUTO_TEST_SUITE(test2_suite)
//
//BOOST_AUTO_TEST_CASE(Test1)
//{
//    BOOST_CHECK(1 < 2);
//}
//
//BOOST_AUTO_TEST_SUITE_END()

