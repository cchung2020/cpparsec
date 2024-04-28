#define BOOST_TEST_MODULE cpparsec_alt_char

#include <algorithm>
#include <list>
#include <boost/test/included/unit_test.hpp>
#include <boost/cregex.hpp>
#include "../cpparsec_core.h"
#include "../cpparsec_char_alt_example.h"

using std::string, std::vector, std::tuple, std::optional, std::function;
using std::ranges::all_of;

using namespace cpparsec_example;

// --------------------------- Character Parsers ---------------------------
BOOST_AUTO_TEST_SUITE(Character_Parsers)

BOOST_AUTO_TEST_CASE(Char_Parser_Success)
{

    string inputStr = "aabab";
    CustomStrView input = inputStr;

    CharParser<char> a = char_('a');
    ParseResult<char> result = a.parse(input);

    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK(*result == 'a');
    BOOST_CHECK(input == "abab");
}

BOOST_AUTO_TEST_CASE(Char_Parser_Failure)
{
    string inputStr = "aabab";
    CustomStrView input = inputStr;

    ParseResult<char> result = char_('A').parse(input);

    BOOST_REQUIRE(!result.has_value());
    BOOST_CHECK(result.error()().message() == "Expected 'a', found 'A'");
}

BOOST_AUTO_TEST_CASE(Char_And_Operator)
{
    string inputStr = "abab";
    CustomStrView input = inputStr;

    CharParser<tuple<char, char>> a_and_b = char_('a') & char_('b');
    ParseResult<tuple<char, char>> result = a_and_b.parse(input);

    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK(*result == tuple('a', 'b'));
    BOOST_CHECK(input == "ab");
}

BOOST_AUTO_TEST_CASE(Char_With_Method)
{
    string inputStr = "ab";
    CustomStrView input = inputStr;

    CharParser<char> a = char_('a');
    CharParser<char> a_with_b = a.with(char_('b'));
    auto result = a_with_b.parse(input);

    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK(*result == 'b');
    BOOST_CHECK(input == "");
}

BOOST_AUTO_TEST_CASE(Char_Parser_End_Of_Input)
{
    ParseResult<char> result = char_('x').parse("");

    BOOST_REQUIRE(!result.has_value());
    BOOST_CHECK(result.error()().message() == "Expected \"end of input\", found \"x\"");
}

BOOST_AUTO_TEST_SUITE_END()

// --------------------------- String Parsers ---------------------------

BOOST_AUTO_TEST_SUITE(String_Parsers)

BOOST_AUTO_TEST_CASE(String_Parser_Success)
{
    string inputStr = "test string";
    CustomStrView input = inputStr;

    ParseResult<string> result = string_("test").parse(input);

    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK(*result == "test");
    BOOST_CHECK(input == " string");
}

BOOST_AUTO_TEST_CASE(String_Parser_With_Space)
{
    string inputStr = " string";
    CustomStrView input = inputStr;

    ParseResult<string> result = (space() >> string_("string")).parse(input);
    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK(*result == "string");
    BOOST_CHECK(input == "");
}

BOOST_AUTO_TEST_CASE(String_Parser_Failure)
{
    ParseResult<string> result = string_("finished").parse("finishes");

    BOOST_REQUIRE(!result.has_value()); // Got 's', wanted 'd'. 
    BOOST_CHECK(result.error()().message() == "Expected 'd', found 's'");
    BOOST_CHECK(result.error()().message_stack() ==
        "Expected 'd', found 's'\n"
        "Expected \"finished\", found \"finishes\"");
}

BOOST_AUTO_TEST_SUITE_END()

// --------------------------- Count Parsers ---------------s------------

BOOST_AUTO_TEST_SUITE(Count_Parsers)

BOOST_AUTO_TEST_CASE(Count_Parser_Success)
{
    string inputStr = "1 2 3 4 5 6 7";
    CustomStrView input = inputStr;

    ParseResult<vector<int>> result = count(5, int_() << optional_(space())).parse(input);

    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK(*result == vector({ 1, 2, 3, 4, 5 }));
    BOOST_CHECK(input == "6 7");
}

BOOST_AUTO_TEST_CASE(Count_Parser_Failure)
{
    string inputStr = "1 2 3";
    CustomStrView input = inputStr;

    ParseResult<vector<int>> result = count(5, int_() << optional_(space())).parse(input);

    BOOST_REQUIRE(!result.has_value());
    BOOST_CHECK(result.error()().message() == "Expected \"<digit>\", found \"end of input\"");
}

BOOST_AUTO_TEST_CASE(Count_Parser_Complex_Type)
{
    string inputStr = "xyxyxyxy!";
    CustomStrView input = inputStr;

    auto is_xy = [](auto xy) { return xy == tuple('x', 'y'); };
    ParseResult<vector<tuple<char, char>>> result = count(4, char_('x') & char_('y')).parse(input);

    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK(result->size() == 4);
    BOOST_CHECK(all_of(*result, is_xy));
    BOOST_CHECK(input == "!");
}

BOOST_AUTO_TEST_SUITE_END()

// --------------------------- Between Parsers ---------------------------

BOOST_AUTO_TEST_SUITE(Between_Parsers)

BOOST_AUTO_TEST_CASE(Between_Parser_Success)
{
    string inputStr = "xyz";
    CustomStrView input = inputStr;

    ParseResult<char> result = char_('y').between(char_('x'), char_('z')).parse(input);

    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK(*result == 'y');
    BOOST_CHECK(eof<CustomStrView>().parse(input).has_value());
}

BOOST_AUTO_TEST_CASE(Between_Parser_Failure)
{
    ParseResult<char> result = char_('Y').between(char_('x'), char_('z')).parse("xyz");

    BOOST_REQUIRE(!result.has_value());
    BOOST_CHECK(result.error()().message() == "Expected 'y', found 'Y'");
}

BOOST_AUTO_TEST_CASE(Between_Parser_Complex_Type)
{
    CharParser<string> excl_between_xz = many1(char_('!')).between(char_('x'), char_('z'));
    ParseResult<string> result = excl_between_xz.parse("x!!!!!!!z");

    BOOST_REQUIRE(result.has_value());
    BOOST_REQUIRE(*result == "!!!!!!!");
}

BOOST_AUTO_TEST_SUITE_END()

// --------------------------- Many Parsers ---------------------------

BOOST_AUTO_TEST_SUITE(Many_Parsers)

BOOST_AUTO_TEST_CASE(Many_Parser_Digits)
{
    string inputStr = "53242k";
    CustomStrView input = inputStr;

    ParseResult<string> result = many(digit()).parse(input);

    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK(*result == "53242");
    BOOST_CHECK(input == "k");
}

BOOST_AUTO_TEST_CASE(Many_Parser_Complex_Type)
{
    string inputStr = "xyxyxyEND";
    CustomStrView input = inputStr;

    auto xy = char_('x') & char_('y');
    auto is_xy = [](auto xy) { return xy == tuple('x', 'y'); };
    ParseResult<vector<tuple<char, char>>> result = many(xy).parse(input);

    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK(result->size() == 3);
    BOOST_CHECK(all_of(*result, is_xy));
    BOOST_CHECK(input == "END");
}

BOOST_AUTO_TEST_CASE(Many1_Parser_UpperCase)
{
    string inputStr = "HELLOworld";
    CustomStrView input = inputStr;

    ParseResult<string> result = many1(upper()).parse(input);

    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK(*result == "HELLO");
    BOOST_CHECK(input == "world");
}

BOOST_AUTO_TEST_CASE(Many1_Parser_Failure)
{
    string inputStr = "helloWORLD";
    CustomStrView input = inputStr;

    ParseResult<string> result = many1(upper()).parse(input);

    BOOST_REQUIRE(!result.has_value());
}

BOOST_AUTO_TEST_SUITE_END()

// --------------------------- SepBy Parsers ---------------------------

BOOST_AUTO_TEST_SUITE(SepBy_Parsers)

BOOST_AUTO_TEST_CASE(SepBy_Parser_Success)
{
    string inputStr = "1 2 3 4 5";
    CustomStrView input = inputStr;

    CharParser<vector<int>> spaced_ints = sep_by(int_(), space());
    ParseResult<vector<int>> result = spaced_ints.parse(input);

    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK(*result == vector({ 1, 2, 3, 4, 5 }));
}

BOOST_AUTO_TEST_CASE(SepBy1_Parser_Complex_Input)
{
    string inputStr = "a 1b2c 3y 123z";
    CustomStrView input = inputStr;

    CharParser<string> char_sepby_spaced_int = sep_by1(any_char(), optional_(space()) >> int_());
    ParseResult<string> result = char_sepby_spaced_int.parse(input);

    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK(*result == std::string({ 'a', 'b', 'c', 'y', 'z' }));
}

BOOST_AUTO_TEST_SUITE_END()

// --------------------------- ManyTill Parsers ---------------------------

BOOST_AUTO_TEST_SUITE(ManyTill_Parsers)

BOOST_AUTO_TEST_CASE(ManyTill_Parser_Success)
{
    string inputStr = "1 2 3 4 5!...";
    CustomStrView input = inputStr;

    CharParser<vector<int>> nums_till_excl = many_till(int_() << optional_(space()), char_('!'));
    ParseResult<vector<int>> result = nums_till_excl.parse(input);

    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK(*result == vector({ 1, 2, 3, 4, 5 }));
    BOOST_CHECK(input == "...");
}

BOOST_AUTO_TEST_CASE(ManyTill_Parser_Empty_Success)
{
    string inputStr = "!nothing";
    CustomStrView input = inputStr;

    CharParser<vector<int>> nums_till_excl = many_till(int_() << optional_(space()), char_('!'));
    ParseResult<vector<int>> result = nums_till_excl.parse(input);

    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK(result->empty());
    BOOST_CHECK(input == "nothing");
}

BOOST_AUTO_TEST_CASE(ManyTill_Parser_Complex_Type)
{
    string inputStr = "/*inside comment*/!";
    CustomStrView input = inputStr;

    CharParser<string> simple_comment =
        string_("/*") >> many1_till(any_char(), try_(string_("*/")));
    ParseResult<string> result = simple_comment.parse(input);

    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK(*result == "inside comment");
    BOOST_CHECK(input == "!");
}

BOOST_AUTO_TEST_SUITE_END()

// --------------------------- TryOr Parsers ---------------------------

BOOST_AUTO_TEST_SUITE(TryOr_Parsers)

BOOST_AUTO_TEST_CASE(TryOr_Parser_Success)
{
    string inputStr = "ab";
    CustomStrView input = inputStr;

    auto a_or_b = char_('a') | char_('b');
    ParseResult<char> result = a_or_b.parse(input);

    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK(*result == 'a');
    BOOST_CHECK(input == "b");
}

BOOST_AUTO_TEST_CASE(TryOr_Parser_With_Try_Success)
{
    string inputStr = "three";
    CustomStrView input = inputStr;

    auto two = string_("two");
    auto three = string_("three");
    auto try_two_or_three = try_(two) | three;
    ParseResult<string> result = try_two_or_three.parse(input);

    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK(*result == "three");
}

BOOST_AUTO_TEST_SUITE_END()

// --------------------------- Int Parsers ---------------------------
BOOST_AUTO_TEST_SUITE(Int_Parsers)

BOOST_AUTO_TEST_CASE(Int_Parser_Simple)
{
    ParseResult<int> result = int_().parse("1");

    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK(*result == 1);
}

BOOST_AUTO_TEST_CASE(Int_Parser_With_Space)
{
    ParseResult<tuple<int, int>> result = (int_() << space() & int_()).parse("25 105");

    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK(*result == tuple(25, 105));
}

BOOST_AUTO_TEST_CASE(Int_Parser_Failure)
{
    ParseResult<int> result = int_().parse("a500");

    BOOST_REQUIRE(!result.has_value());
    // println("{}", result.error()().message());
    //BOOST_CHECK(result.error()().message() == "Expected \"<digit>\", found \"\"");
}

BOOST_AUTO_TEST_SUITE_END()

// --------------------------- Optional Result Parsers ---------------------------

BOOST_AUTO_TEST_SUITE(Optional_Result_Parsers)

BOOST_AUTO_TEST_CASE(Optional_Result_Parser_Int)
{
    CharParser<optional<int>> opt_int = optional_result(int_());
    ParseResult<optional<int>> result = opt_int.parse("123");

    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK(*result == optional(123));
}

BOOST_AUTO_TEST_CASE(Optional_Result_Parser_No_Int)
{
    CharParser<optional<int>> opt_int = optional_result(int_());
    ParseResult<optional<int>> result = opt_int.parse("X");

    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK(*result == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()

// --------------------------- ChainL Parser ---------------------------

BOOST_AUTO_TEST_SUITE(ChainL_Parsers)

BOOST_AUTO_TEST_CASE(ChainL_Parser)
{
    auto add_op = success<CustomStrView>(function<int(int, int)>([](int a, int b) { return a + b; }));
    auto mul_op = success<CustomStrView>(function<int(int, int)>([](int a, int b) { return a * b; }));

    auto spaced = [](auto p) { return p.between(spaces(), spaces()); };

    function<CharParser<int>()> expr, term, factor;

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
    CustomStrView input = inputStr;

    ParseResult<int> result7 = expr().parse(input);

    BOOST_REQUIRE(result7);
    BOOST_CHECK(*result7 == 19);
    BOOST_CHECK(input == "end");
}

BOOST_AUTO_TEST_SUITE_END()

// --------------------------- LookAhead_NotFollowedBy Parser ---------------------------

BOOST_AUTO_TEST_SUITE(LookAhead_NotFollowedBy_Parsers)

BOOST_AUTO_TEST_CASE(LookAhead_NotFollowedBy_Parser)
{
    auto partialParseNum = [](const string& word, int num) {
        return try_(
            look_ahead(string_(word)) >> char_(word[0]).success(num)
        );
    };

    CharParser<int> wordToNum =
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

    CharParser<int> number = digit().transform(charToInt) | wordToNum;

    CharParser<int> numberBetweenLetters = number.between(
        (many(not_followed_by(number) >> letter())),
        (many(not_followed_by(number) >> letter())));

    string inputStr = "x5KZ4threeXtwone0Y";
    CustomStrView input = inputStr;

    ParseResult<vector<int>> result1 = many1(numberBetweenLetters).parse(input);

    BOOST_REQUIRE(result1.has_value());
    BOOST_CHECK(*result1 == vector({ 5, 4, 3, 2, 1, 0 }));
    BOOST_CHECK(input == "");
}

BOOST_AUTO_TEST_SUITE_END()

// --------------------------- Choice Parsers ---------------------------
BOOST_AUTO_TEST_SUITE(Choice_Parsers)

BOOST_AUTO_TEST_CASE(Choice_Parser_Multiple_Choices)
{
    string inputStr = "two threeten two tenEND";
    CustomStrView input = inputStr;

    CharParser<int> t_skeleton_nums = choice(vector({
        try_(string_("two") >> spaces().success(2)),
        try_(string_("three") >> spaces().success(3)),
        try_(string_("ten") >> spaces().success(10)),
    }));

    ParseResult<vector<int>> result = many1(t_skeleton_nums).parse(input);

    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK(*result == vector({ 2, 3, 10, 2, 10 }));
    BOOST_CHECK(input == "END");
}

BOOST_AUTO_TEST_SUITE_END()
