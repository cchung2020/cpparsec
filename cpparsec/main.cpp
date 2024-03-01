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
void test2() {

}

int main() {
    println("parsing \"azzzb\" with between(character('a'), character('b'), many(character('z'))");
    if (ParserResult result2 = between(character('a'), character('b'), many(character('z'))).parse("azzzb"))
        println("parsed {} 'z' chars", result2->size());
    else
        println("failed to parse");

    return 0;
}

void test1() {
    auto a = character('a');
    auto b = character('b');
    auto parseAWithB = a.with(b);
    auto parseAWithB2 = character('a').with(character('b'));

    string inputStr = "ababababzzzzz";
    string_view input = inputStr;
    println("{}", input);

    string string2 = "a";
    string_view input2 = string2;

    auto resultX = character('a').parse(string2);
    if (resultX) println("parsed {}, remainder \"{}\"", *resultX, input2);
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
    result = character('a').with(character('b')).parse(input);
    if (result) println("parsed {}, remainder \"{}\"", *result, input);
    else println("failed to parse");

    // ok
    if (ParserResult result2 = many(character('z')).parse(input))
        println("parsed {} 'z' chars, remainder \"{}\"", result2->size(), input);
    else
        println("failed to parse");
}
