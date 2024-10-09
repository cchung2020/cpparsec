# What is CPParsec?

CPParsec is my senior honors project at the University of Massachusetts Lowell. 

It is an experimental toy C++23 monadic parser-combinator library heavily inspired by [Haskell's Parsec library](https://en.wikipedia.org/wiki/Parsec_(parser)). 

Many of the design decisions are deliberately probing what is possible using the latest features, and are not necessarily the best tool for certain problems. 

# How do I install/compile it?

Currently the project is header only. `include cpparsec.h` for the base character/std::string specialized parsers.

It's a header file which simply includes cpparsec_core.h and cpparsec_char.h.

1. cpparsec_core.h: headers for parser combinators 
2. cpparsec_char.h: headers for char/std::string specializations

# How do I use it?

See [examples folder](https://github.com/cchung2020/cpparsec/tree/master/cpparsec/examples) and [tests](https://github.com/cchung2020/cpparsec/tree/master/cpparsec/tests) for usage.

# To do:
* Make error handling more efficient (!)
* Make the library work on other compilers, only works on MSVC currently (!)
* Investigate lowering compile times (!) but I suspect it's impossible without a total rewrite
* Investigate gap between debug and release build, currently up to 150x difference (!)
* Add more benchmarking tests (maybe GitHub actions-supported?)
* Add a more tests
* Add more examples

This project is licensed under the terms of the MIT license.
