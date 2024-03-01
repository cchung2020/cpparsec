#include <iostream>
#include <print>
#include <string>
#include "cpparsec.h"

using namespace cpparsec;
using namespace std;

int main() {
    auto a = character('a');
    auto b = character('b');
    auto parseAWithB = a.with(b);
    auto parseAWithB2 = character('a').with(character('b'));

    std::string input = "ababababzzzzz";

    println("{}", input);

    // ok 
    auto result = parseAWithB.parse(input);
    if (result) println("parsed {}, remainder \"{}\"", result->first, result->second);
    else println("failed to parse");

    // exception thrown
    result = parseAWithB2.parse(result->second);
    if (result) println("parsed {}, remainder \"{}\"", result->first, result->second);
    else println("failed to parse");

    // ok
    result = a.with(b).parse(result->second);
    if (result) println("parsed {}, remainder \"{}\"", result->first, result->second);
    else println("failed to parse");

    // ok
    result = character('a').with(character('b')).parse(result->second);
    if (result) println("parsed {}, remainder \"{}\"", result->first, result->second);
    else println("failed to parse");

    // ok
    if (ParserResult result2 = many(character('z')).parse(result->second)) 
        println("parsed {} 'z' chars, remainder \"{}\"", result2->first.size(), result2->second);
    else 
        println("failed to parse");


    return 0;
}
