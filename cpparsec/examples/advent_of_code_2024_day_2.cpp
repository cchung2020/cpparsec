#include "../cpparsec.h"
#include "../cpparsec_numeric.h"

/*
    parses input in a helpful format for https://adventofcode.com/2023/day/2
    includes both styles of parsing (Parsec and Combine)
*/

using namespace cpparsec;

using Cube = std::pair<int, std::string>;
using Game = std::pair<int, std::vector<Cube>>;

// Parsec-style parsing
Parser<std::pair<int, std::string>> cube() {
    return CPPARSEC_MAKE(Parser<std::pair<int, std::string>>) {
        CPPARSEC_SAVE(cubeNum, int_() << space());
        CPPARSEC_SAVE(cubeColor, string_("red") | string_("green") | string_("blue"));

        return Cube{ cubeNum, cubeColor };
    };
}

// behaves the same as above, Combine-style parsing
Parser<std::pair<int, std::string>> cube2() {
    return (int_() << space())
        .pair_with((string_("red") | string_("green") | string_("blue")));
}

// behaves the same as above, Combine-style parsing, no explicit operators
Parser<std::pair<int, std::string>> cube3() {
    return (int_().skip(space()))
        .pair_with((string_("red").or_(string_("green")).or_(string_("blue"))));
}

Parser<std::vector<Cube>> gameCubes() {
    return sep_by1(cube(), char_(',') | char_(';'));
}

// Parsec-style parsing
Parser<Game> game() {
    return CPPARSEC_MAKE(Parser<Game>) {
        CPPARSEC_SKIP(string_("Game "));
        CPPARSEC_SAVE(gameNum, int_());
        CPPARSEC_SKIP(char_(';'));
        CPPARSEC_SAVE(cubes, gameCubes());

        return Game{ gameNum, cubes };
    };
}

// Parsec-style parsing + Combine-style
Parser<Game> game2() {
    return CPPARSEC_MAKE(Parser<Game>) {
        CPPARSEC_SAVE(gameNum, string_("Game ") >> int_());
        CPPARSEC_SAVE(cubes, char_(';') >> gameCubes());

        return Game{ gameNum, cubes };
    };
}

// behaves the same as above, Combine-style parsing
Parser<Game> game3() {
    return (string_("Game ") >> int_())
        .pair_with(char_(';') >> gameCubes());
}

// behaves the same as above, Combine-style parsing, no explicit operators
Parser<Game> game4() {
    return (string_("Game ").with(int_()))
        .pair_with(char_(';').with(gameCubes()));
}

Parser<std::vector<Game>> all_input() {
    return sep_by1(game(), newline());
}
