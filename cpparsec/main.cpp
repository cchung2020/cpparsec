#include <iostream>
#include <functional>
#include <optional>
#include <string>
#include <vector>
#include <memory>
#include <print>
#include <ranges>
#include <algorithm>
#include "cpparsec.h"

using namespace cpparsec;
using namespace std;

void test1();
void test2();
void test3();
void test4();
void test5();
void test6();

template <typename T>
Parser<T> optional_parenthesis(Parser<T> p) {
    println("x");
    return p | p.between(char_('('), char_(')')) | optional_parenthesis(p);
}

int main() {
    string x = {};
    ParserResult r1 = optional_parenthesis(digit()).parse("(1)");
    ParserResult r2 = optional_parenthesis(digit()).parse("2");
    if (r1 && r2) {
        println("{} {}", *r1, *r2);
    }
}

//// & "and" operator joins two parses
//Parser<std::tuple<char,char>> operator&(const Parser<char> lhs, const Parser<char> rhs) {
//    return Parser<std::tuple<char, char>>([=](const std::string& input)
//        -> ParserResult<std::tuple<char, char>>{
//        auto lhsResult = lhs.parse(input);
//        if (!lhsResult) {
//            return std::nullopt;
//        }
//
//        auto rhsResult = rhs.parse(input);
//        if (!rhsResult) {
//            return std::nullopt;
//        }
//
//        return { std::make_tuple(*lhsResult, *rhsResult) };
//        });
//}

//int main() {
//    auto result = cubeParser().parse("2 red");
//    if (result) {
//        auto [num, color] = *result; 
//        println("{} {}", num, color);
//    }
//    else {
//        println("fail");
//    }
//
//    //string x = "255";
//    //string_view x2 = x;
//    //if (auto a = (string_("25") | string_("26")).parse(x2)) {
//    //    println(
//    //        "{} and {}",
//    //        *a, 2
//    //    );
//    //    println("{} left", x2);
//    //}
//    //else {
//    //    println("nothing");
//    //}
//
//    //test1();
//    //test2();
//    //test3();
//    //test4();
//    //test5();
//    //test6();
//
//    return 0;
//}

void test6() {
    string num = "1234";
    if (auto result = int_().parse(num)) {
        println("successful parse of {}", *result);
    }
    else {
        println("test 6: failed to parse {}", num);
    }
}

void test5() {
    auto a1 = any_char();
    auto a2 = any_char();
    Parser<std::tuple<char, char>> abParse = (a1 & a2);

    if (auto result = abParse.parse("ab")) {
        auto [a, b] = *result;
        println("successfully paresd {} {}", a, b);
    }
    else {
        println("test5 fail1");
    }

    if (auto result = (char_('z') & abParse).parse("zab")) {
        auto [z, a, b] = *result;
        println("successfully paresd {} {} {}", z, a, b);
    }
    else {
        println("test5 fail2");
    }

    if (auto result = (abParse & char_('z')).parse("abz")) {
        auto [a, b, z] = *result;
        println("successfully paresd {} {} {}", a, b, z);
    }
    else {
        println("test5 fail3");
    }

    if (auto result = ((digit() & digit()) & abParse).parse("12ab")) {
        auto [a, b, c, d] = *result;
        println("successfully paresd {} {} {} {}", a, b, c, d);
    }
    else {
        println("test5 fail4s");
    }
}

void test4() {
    println("parsing \"azzzxb\" with between(character('a'), character('b'), many(character('z'))");
    if (ParserResult result2 = many(char_('z')).between(char_('a'), char_('b')).parse("azzzxb"))
        println("4: parsed {} 'z' chars (bad, should fail)", result2->size());
    else
        println("4: failed to parse between many (ok)");
    println("parsing \"azzzb\" with between(character('a'), character('b'), many(character('z'))");
    if (ParserResult result2 = many(char_('z')).between(char_('a'), char_('b')).parse("azzzb"))
        println("4: parsed {} 'z' chars (ok)", result2->size());
    else
        println("4: failed to parse between many (bad, should succeed)");

}

void test3() {
    auto zxs = many(char_('z') & (char_('x')));
    if (zxs.parse("zxzxzxzy"))
        cout << "suceed (bad, should commit on z fail seeing y)\n";
    else
        cout << "3:failed to parse (ok)\n";

    if (zxs.parse("zxzxzx"))
        cout << "suceed (ok)\n";
    else
        cout << "3:failed to parse (bad, should eat all input)\n";

    if (zxs.parse("zxzxzxy"))
        cout << "suceed (ok)\n";
    else
        cout << "3:failed to parse (bad, should leave y alone)\n";
}

void test2() {
    println("parsing \"azzzb\" with between(character('a'), character('b'), many(character('z'))");
    if (ParserResult result2 = between(char_('a'), char_('b'), many(char_('z'))).parse("azzzb"))
        println("parsed {} 'z' chars", result2->size());
    else
        println("failed to parse between many");
}

void test1() {
    auto a = char_('a');
    auto b = char_('b');
    auto parseAWithB = a.with(b);
    auto parseAWithB2 = char_('a').with(char_('b'));

    string inputStr = "aababababzzzzxx";
    string_view input = inputStr;
    println("{}", input);

    auto resultX = char_('a').parse(input);
    if (resultX) println("parsed {}, remainder \"{}\"", *resultX, input);
    else println("failed to parse!");

    // ok 
    auto result = parseAWithB.parse(input);
    if (result) println("parsed {}, remainder \"{}\"", *result, input);
    else println("failed to parse");

    // exception thrown
    result = parseAWithB2.parse(input);
    if (result) println("parsed {}, remainder \"{}\"", *result, input);
    else println("failed to parse");

    // ok
    result = a.with(b).parse(input);
    if (result) println("parsed {}, remainder \"{}\"", *result, input);
    else println("failed to parse");

    // ok
    result = char_('a').with(char_('b')).parse(input);
    if (result) println("parsed {}, remainder \"{}\"", *result, input);
    else println("failed to parse");

    // ok
    if (ParserResult result2 = many(char_('z').with(char_('z'))).parse(input))
        println("parsed {} 'zz' strs, remainder \"{}\"", result2->size(), input);
    else
        println("failed to parse many");
}
