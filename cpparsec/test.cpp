#define BOOST_TEST_MODULE cpparsec

#include <boost/test/included/unit_test.hpp>
#include "cpparsec.h"
#include <algorithm>

using namespace cpparsec;
using std::string, std::string_view, std::vector, std::tuple, std::optional, std::function;
using std::ranges::all_of;

BOOST_AUTO_TEST_SUITE(test1_suite)

BOOST_AUTO_TEST_CASE(Char_Parser)
{
    string inputStr = "aabab";
    string_view input = inputStr;

    Parser<char> a = char_('a');
    ParseResult<char> result1 = a.parse(input);

    BOOST_REQUIRE(result1.has_value());
    BOOST_CHECK(*result1 == 'a');
    BOOST_CHECK(input == "abab");

    BOOST_CHECK(!char_('A').parse(input).has_value());
    BOOST_CHECK(input == "abab");

    Parser<tuple<char, char>> a_and_b = char_('a') & (char_('b'));
    ParseResult result2 = a_and_b.parse(input);

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

BOOST_AUTO_TEST_CASE(String_Parser)
{
    string inputStr = "test string";
    string_view input = inputStr;

    ParseResult<string> result1 = string_("test").parse(input);

    BOOST_REQUIRE(result1.has_value());
    BOOST_CHECK(*result1 == "test");
    BOOST_CHECK(input == " string");

    ParseResult<string> result2 = (space() >> string_("string")).parse(input);

    BOOST_REQUIRE(result2.has_value());
    BOOST_CHECK(*result2 == "string");
    BOOST_CHECK(input == "");

    ParseResult<string> result3 = string_("finished").parse("finishes");

    BOOST_REQUIRE(!result3.has_value());
    // BOOST_CHECK(input == "");
    // very debatable for now, consumptive failing input behavior is unsettled

}

BOOST_AUTO_TEST_CASE(Count_Parser)
{
    string inputStr = "1 2 3 4 5 6 7";
    string_view input = inputStr;

    ParseResult<vector<int>> result1 = count(5, int_() << optional_(space())).parse(input);

    BOOST_REQUIRE(result1.has_value());
    BOOST_CHECK(*result1 == vector({ 1, 2, 3, 4, 5 }));
    BOOST_CHECK(input == "6 7");

    ParseResult<vector<int>> result2 = count(3, int_() << optional_(space())).parse(input);
    
    BOOST_REQUIRE(!result2.has_value());

    inputStr = "xyxyxyxy!";
    input = inputStr;

    auto is_xy = [](auto xy) { return xy == tuple('x', 'y'); };
    ParseResult<vector<tuple<char, char>>> result3 = count(3, char_('x') & char_('y')).parse(input);
    vector<tuple<char, char>> vec = {  };

    BOOST_REQUIRE(result3.has_value());
    BOOST_CHECK(result3->size() == 3);
    BOOST_CHECK(all_of(*result3, is_xy));
    BOOST_CHECK(input == "xy!");
}

BOOST_AUTO_TEST_CASE(Between_Parser)
{
    string inputStr = "xyz";
    string_view input = inputStr;

    ParseResult<char> result1 = char_('y').between(char_('x'), char_('z')).parse(input);

    BOOST_REQUIRE(result1.has_value());
    BOOST_CHECK(*result1 == 'y');
    BOOST_CHECK(eof().parse(input).has_value());

    ParseResult<char> result2 = char_('Y').between(char_('x'), char_('z')).parse("xyz");
    BOOST_REQUIRE(!result2.has_value());

    Parser<string> excl_between_xz = many1(char_('!')).between(char_('x'), char_('z'));
    ParseResult<string> result3 = excl_between_xz.parse("x!!!!!!!z");
    BOOST_REQUIRE(result3.has_value());
    BOOST_REQUIRE(*result3 == "!!!!!!!");
}

BOOST_AUTO_TEST_CASE(Many_Parser)
{
    string inputStr = "53242k";
    string_view input = inputStr;

    ParseResult<string> result1 = many(digit()).parse(input);

    BOOST_REQUIRE(result1.has_value());
    BOOST_CHECK(*result1 == "53242");
    BOOST_CHECK(input == "k");

    inputStr = "xyxyxyEND";
    input = inputStr;

    auto xy = char_('x') & char_('y');
    auto is_xy = [](auto xy) { return xy == tuple('x', 'y'); };
    ParseResult<vector<tuple<char, char>>> result2 = many(xy).parse(input);

    BOOST_REQUIRE(result2.has_value());
    BOOST_CHECK(result2->size() == 3);
    BOOST_CHECK(all_of(*result2, is_xy));
    BOOST_CHECK(input == "END");

    inputStr = "xyxyxyxZ";
    input = inputStr;

    ParseResult<vector<tuple<char, char>>> result3 = many(xy).parse(input);

    BOOST_REQUIRE(!result3.has_value());
    // BOOST_CHECK(input == "Z");
    // very debatable for now, consumptive failing input behavior is unsettled

    inputStr = "HELLOworld";
    input = inputStr;

    ParseResult<string> result4 = many1(upper()).parse(input);

    BOOST_REQUIRE(result4.has_value());
    BOOST_CHECK(*result4 == "HELLO");
    BOOST_CHECK(input == "world");

    inputStr = "helloWORLD";
    input = inputStr;

    ParseResult<string> result5 = many1(upper()).parse(input);

    BOOST_REQUIRE(!result5.has_value());

    // ParseResult<std::u16string> result6 = many1<char, std::u16string>(upper()).parse(input);
}

BOOST_AUTO_TEST_CASE(TryOr_Parser)
{
    string inputStr = "ab";
    string_view input = inputStr;

    auto a_or_b = char_('a') | char_('b');

    ParseResult<char> result1 = a_or_b.parse(input);

    BOOST_REQUIRE(result1.has_value());
    BOOST_CHECK(*result1 == 'a');
    BOOST_CHECK(input == "b");

    inputStr = "ba";
    input = inputStr;

    ParseResult<char> result2 = a_or_b.parse(input);

    BOOST_REQUIRE(result2.has_value());
    BOOST_CHECK(*result2 == 'b');
    BOOST_CHECK(input == "a");

    auto two = string_("two");
    auto three = string_("three");
    auto two_or_three = two | three;

    ParseResult<string> result3 = two_or_three.parse("two");

    BOOST_REQUIRE(result3.has_value());
    BOOST_CHECK(*result3 == "two");

    ParseResult<string> result4 = two_or_three.parse("three");

    BOOST_REQUIRE(!result4.has_value());

    auto try_two_or_three = try_(two) | three;

    ParseResult<string> result5 = try_two_or_three.parse("two");

    BOOST_REQUIRE(result5.has_value());
    BOOST_CHECK(*result5 == "two");

    ParseResult<string> result6 = try_two_or_three.parse("three");

    BOOST_REQUIRE(result6.has_value());
    BOOST_CHECK(*result6 == "three");
}

BOOST_AUTO_TEST_CASE(Int_Parser)
{
    ParseResult<int> result1 = int_().parse("1");

    BOOST_REQUIRE(result1.has_value());
    BOOST_CHECK(*result1 == 1);

    ParseResult<tuple<int, int>> result2 = (int_() << space() & int_()).parse("25 105");

    BOOST_REQUIRE(result2.has_value());
    BOOST_CHECK(*result2 == tuple(25, 105));

    ParseResult<int> result3 = (int_()).parse("a500");

    BOOST_REQUIRE(!result3.has_value());

    ParseResult<vector<int>> result4 = many1(int_() << (optional_(space()))).parse("1 2 3 4 5");

    BOOST_REQUIRE(result4.has_value());
    BOOST_CHECK(*result4 == vector({ 1, 2, 3, 4, 5 }));

}

BOOST_AUTO_TEST_CASE(Optional_Result_Parser)
{
    Parser<optional<int>> opt_int = optional_result(int_());
    ParseResult<optional<int>> result1 = opt_int.parse("123");

    BOOST_REQUIRE(result1.has_value());
    BOOST_CHECK(*result1 == optional(123));

    ParseResult<optional<int>> result2 = opt_int.parse("X");

    BOOST_REQUIRE(result2.has_value());
    BOOST_CHECK(*result2 == std::nullopt);

    // one more

}

BOOST_AUTO_TEST_CASE(SepBy_Parser)
{
    Parser<vector<int>> spaced_ints = sep_by(int_(), space());
    ParseResult<vector<int>> result1 = spaced_ints.parse("1 2 3 4 5");

    BOOST_REQUIRE(result1.has_value());
    BOOST_CHECK(*result1 == vector({ 1, 2, 3, 4, 5 }));

    string inputStr = "1 2 3 4x";
    string_view input = inputStr;
    ParseResult<vector<int>> result2 = spaced_ints.parse(input);

    BOOST_REQUIRE(result2.has_value());
    BOOST_CHECK(*result2 == vector({ 1, 2, 3, 4}));
    BOOST_CHECK(input == "x");

    inputStr = "!1 2 3 4 5";
    input = inputStr;

    ParseResult<vector<int>> result3 = spaced_ints.parse(input);

    BOOST_REQUIRE(result3.has_value());
    BOOST_CHECK(input == inputStr);

    Parser<vector<int>> spaced_ints1 = sep_by1(int_(), space());
    ParseResult<vector<int>> result4 = spaced_ints1.parse(input);

    BOOST_REQUIRE(!result4.has_value());

    Parser<string> char_sepby_spaced_int = sep_by1(any_char(), optional_(space()) >> int_());

    inputStr = "a 1b2c 3y 123z";
    input = inputStr;
    ParseResult<string> result5 = char_sepby_spaced_int.parse(input);

    BOOST_REQUIRE(result5.has_value());
    BOOST_CHECK(*result5 == std::string({ 'a', 'b', 'c', 'y', 'z' }));
}

BOOST_AUTO_TEST_CASE(ManyTill_Parser) {
    string inputStr = "1 2 3 4 5!...";
    string_view input = inputStr;

    Parser<vector<int>> nums_till_excl = many_till(int_() << optional_(space()), char_('!'));

    ParseResult<vector<int>> result1 = nums_till_excl.parse(input);

    BOOST_REQUIRE(result1.has_value());
    BOOST_CHECK(*result1 == vector({1, 2, 3, 4, 5}));
    BOOST_CHECK(input == "...");

    inputStr = "!nothing";
    input = inputStr;

    ParseResult<vector<int>> result2 = nums_till_excl.parse(input);

    BOOST_REQUIRE(result2.has_value());
    BOOST_CHECK(result2->size() == 0);
    BOOST_CHECK(input == "nothing");

    inputStr = "nothing";
    input = inputStr;

    ParseResult<vector<int>> result3 = nums_till_excl.parse(input);

    BOOST_REQUIRE(!result3.has_value());

    Parser<vector<int>> nums_till_excl1 = many1_till(int_() << optional_(space()), char_('!'));

    inputStr = "!nothing";
    input = inputStr;

    ParseResult<vector<int>> result4 = nums_till_excl1.parse(input);

    BOOST_REQUIRE(!result4.has_value());

    inputStr = "1 2 3 4 5!...";
    input = inputStr;

    ParseResult<vector<int>> result5 = nums_till_excl1.parse(input);

    BOOST_REQUIRE(result5.has_value());
    BOOST_CHECK(*result5 == vector({ 1, 2, 3, 4, 5 }));
    BOOST_CHECK(input == "...");

    inputStr = "/*inside comment*/!";
    input = inputStr;

    Parser<string> simple_comment = 
        string_("/*") >> many1_till(any_char(), try_(string_("*/")));

    ParseResult<string> result6 = simple_comment.parse(input);

    BOOST_REQUIRE(result6.has_value());
    BOOST_CHECK(*result6 == "inside comment");
    BOOST_CHECK(input == "!");

}

BOOST_AUTO_TEST_CASE(LookAhead_NotFollowedBy_Parser) {
    auto partialParseNum = [](const string& word, int num) {
        return try_(
            look_ahead(string_(word)) >> char_(word[0]) >> success(num)
        );
    };

    Parser<int> wordToNum =
        partialParseNum("one", 1)
        | partialParseNum("two", 2)
        | partialParseNum("three", 3)
        | partialParseNum("four", 4)
        | partialParseNum("five", 5)
        | partialParseNum("six", 6)
        | partialParseNum("seven", 7)
        | partialParseNum("eight", 8)
        | partialParseNum("nine", 9);

    function charToInt = [](char c) { return c - '0'; };

    Parser<int> number = digit().transform(charToInt) | wordToNum;

    Parser<int> numberBetweenLetters = number.between(
        (many(not_followed_by(number) >> letter())),
        (many(not_followed_by(number) >> letter())));

    string inputStr = "x5KZ4threeXtwone0Y";
    string_view input = inputStr;

    ParseResult<vector<int>> result1 = many1(numberBetweenLetters).parse(input);

    BOOST_REQUIRE(result1.has_value());
    BOOST_CHECK(*result1 == vector({ 5, 4, 3, 2, 1, 0 }));
    BOOST_CHECK(input == "");
}

BOOST_AUTO_TEST_CASE(ChainL_Parser) {
    auto add_op = success(function<int(int, int)>([](int a, int b) { return a + b; }));
    auto mul_op = success(function<int(int, int)>([](int a, int b) { return a * b; }));

    auto spaced = [](auto p) { return p.between(spaces(), spaces()); };

    function<Parser<int>()> expr, term, factor;

    factor = [&]() { return spaced(int_() | lazy(expr).between(char_('('), char_(')'))); };
    term = [&]() { return chainl1(factor(), char_('*') >> mul_op); };
    expr = [&]() { return chainl1(term(), char_('+') >> add_op); };

    ParseResult<int> result1 = expr().parse("2+3*4");

    BOOST_REQUIRE(result1);
    BOOST_CHECK(*result1 == 14);

    ParseResult<int> result2 = expr().parse("2+3*4+5");

    BOOST_REQUIRE(result2);
    BOOST_CHECK(*result2 == 19);

    ParseResult<int> result3 = expr().parse("(2)+(3*((4)))+5");

    BOOST_REQUIRE(result3);
    BOOST_CHECK(*result3 == 19);

    ParseResult<int> result4 = expr().parse("(2+3*(4+5))");

    BOOST_REQUIRE(result4);
    BOOST_CHECK(*result4 == 29);

    ParseResult<int> result5 = expr().parse("(2+3*");

    BOOST_REQUIRE(!result5);

    ParseResult<int> result6 = expr().parse("1+");

    BOOST_REQUIRE(!result6);

    string inputStr = " ( 2 ) + ( 3 * ( ( 4 ) ) ) + 5 end";
    string_view input = inputStr;

    ParseResult<int> result7 = expr().parse(input);

    BOOST_REQUIRE(result7);
    BOOST_CHECK(*result7 == 19);
    BOOST_CHECK(input == "end");
}

BOOST_AUTO_TEST_CASE(Choice_Parser) {

    Parser<int> t_skeleton_nums = choice<int>(vector({
        try_(string_("two") >> spaces() >> success(2)),
        try_(string_("three") >> spaces() >> success(3)),
        try_(string_("ten") >> spaces() >> success(10)),
    }));

    string inputStr = "two threeten two  tenEND";
    string_view input = inputStr;

    ParseResult<vector<int>> result1 = many1(t_skeleton_nums).parse(input);

    BOOST_REQUIRE(result1.has_value());
    BOOST_CHECK(*result1 == vector({ 2, 3, 10, 2, 10 }));
    BOOST_CHECK(input == "END");
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

