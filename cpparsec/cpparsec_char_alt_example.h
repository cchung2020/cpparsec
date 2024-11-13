#ifndef CPPARSEC_CHAR_ALT_EXAMPLE_H
#define CPPARSEC_CHAR_ALT_EXAMPLE_H

#include "cpparsec_core.h"

namespace cpparsec_example {
    using namespace cpparsec;

    // ===================== ALT CHARACTER PARSER EXAMPLE =====================

    // the default cpparsec_char.h is prioritized in the construction
    // of this library, but you can make your own parser without too 
    // much extra hassle 

    // this one behaves the same as cpparsec_char but with a custom InputStream
    // class which behaves similarly to string view, but counts characters 
    // consumed, utilizing the same cpparsec_core.h combinators

    // eventually this character counting behavior will be the default behavior

    class CustomStrView {
    private:
        std::string_view view;
        size_t chars_consumed, line;

    public:
        CustomStrView(const std::string& str) : view(str), chars_consumed(0), line(0) {}

        using const_iterator = std::string_view::const_iterator;

        // functions for accessing the underlying view
        size_t size() { return view.size(); }
        auto data() { return view.data(); }
        bool empty() { return view.empty(); }
        const_iterator begin() { return view.begin(); }
        const_iterator end() { return view.end(); }
        const char& front() const { return view.front(); }
        const char& back() const { return view.back(); }
        auto& operator[](size_t n) { return view[n]; }
        const auto& operator[](size_t n) const { return view[n]; }
        std::string_view substr(size_t offset, size_t count) { return view.substr(offset, count); }

        void remove_prefix(size_t n) {
            //for (int i = 0; i < n; i++) {
            //    if (view[i] == '\n') {
            //        line++;
            //    }
            //}
            line += std::ranges::count(view | std::views::take(n), '\n');
            chars_consumed += n;
            view.remove_prefix(n);
        }

        size_t get_chars_consumed() {
            return chars_consumed;
        }
        size_t get_lines_consumed() {
            return chars_consumed;
        }

        const std::string_view get_view() {
            return view;
        }
    };

    template <typename T>
    using CharParser = Parser<T, CustomStrView>;

    bool operator==(CustomStrView s1, const std::string& s2) {
        return s1.get_view() == s2;
    }
    bool operator==(const std::string& s1, CustomStrView s2) {
        return s1 == s2.get_view();
    }

    //template <typename T>
    //struct CharParser : public Parser<T, std::string_view> {
    //    using Parser<T, std::string_view>::Parser; 

    //    CharParser(const Parser<T, std::string_view>& other) : Parser<T, std::string_view>(other) {}
    //    CharParser(Parser<T, std::string_view>&& other) : Parser<T, std::string_view>(std::move(other)) {}
    //};

    template <typename T>
    using CharParser = Parser<T, CustomStrView>;

    // ======================== CORE CHARACTER PARSERS ========================

    // Parses a single character
    inline CharParser<char> char_(char c);

    // Parses any character
    inline CharParser<char> any_char();

    // Parses a single character that satisfies a constraint
    // Faster than try_(any_char().satisfy(cond))
    inline CharParser<char> char_satisfy(UnaryPredicate<char> auto cond, std::string&& err_msg = "<char_satisfy>");

    // Parses a single string
    inline CharParser<std::string> string_(const std::string& str);

    // Parses a single letter
    inline CharParser<char> letter() {
        return char_satisfy([](auto c) { return isalpha(c);  }, "<letter>");
    }

    // Parses a single digit
    inline CharParser<char> digit() {
        return char_satisfy(isdigit, "<digit>");
    }

    // Parses a single digit
    inline CharParser<char> digit2() {
        return char_satisfy(isdigit);
    }

    // Parses a single space
    inline CharParser<char> space() {
        return char_satisfy(isspace, "<space>");
    }

    // Skips zero or more spaces
    inline CharParser<std::monostate> spaces() {
        return skip_many(space());
    }

    // Skips one or more spaces
    inline CharParser<std::monostate> spaces1() {
        return skip(space().between(space(), space()));
    }

    // Parses a single newline '\n'
    inline CharParser<char> newline() {
        return char_('\n');
    }

    // Parses a single uppercase letter 
    inline CharParser<char> upper() {
        return char_satisfy(isupper, "<uppercase>");
    }

    // Parses a single lowercase letter 
    inline CharParser<char> lower() {
        return char_satisfy(islower, "<lowercase>");
    }

    // Parses a single alphanumeric letter 
    inline CharParser<char> alpha_num() {
        return char_satisfy(isalnum, "<alphanum>");
    }

    // ======================== STRING SPECIALIZATIONS ========================

    // Parse zero or more characters, std::string specialization
    template <PushBack<char> StringContainer>
    CharParser<StringContainer> many(CharParser<char> charP);

    // Parse one or more characters, std::string specialization
    template <PushBack<char> StringContainer>
    CharParser<StringContainer> many1(CharParser<char> charP);

    // Parses p zero or more times until end succeeds, returning the parsed values, std::string specialization
    template <typename T, PushBack<char> StringContainer>
    CharParser<StringContainer> many_till(CharParser<char> p, CharParser<T> end);

    // Parses p one or more times until end succeeds, returning the parsed values, std::string specialization
    template <typename T, PushBack<char> StringContainer>
    CharParser<StringContainer> many1_till(CharParser<char> p, CharParser<T> end);

    // Parse zero or more parses of p separated by sep, std::string specialization
    template <typename T>
    CharParser<std::string> sep_by(CharParser<char> p, CharParser<T> sep);

    // Parse one or more parses of p separated by sep, std::string specialization
    template <typename T>
    CharParser<std::string> sep_by1(CharParser<char> p, CharParser<T> sep);

    // Parse zero or more parses of p separated by and ending with sep, std::string specialization
    template <typename T>
    CharParser<std::string> end_by(CharParser<char> p, CharParser<T> sep);

    // Parse one or more parses of p separated by and ending with sep, std::string specialization
    template <typename T>
    CharParser<std::string> end_by1(CharParser<char> p, CharParser<T> sep);

    // ========================================================================
    // 
    // ======================= TEMPLATE IMPLEMENTATIONS =======================
    // 
    // ========================================================================

    // ======================== Core Character Parsers ========================

        // Parses a single character
    inline CharParser<char> char_(char c) {
        return CPPARSEC_MAKE(CharParser<char>) {
            CPPARSEC_FAIL_IF(input.empty(), ParseError("end of input", { c }));
            CPPARSEC_FAIL_IF(input[0] != c, ParseError(input[0], c));

            input.remove_prefix(1);
            return c;
        };
    }

    // Parses any character
    inline CharParser<char> any_char() {
        return CPPARSEC_MAKE(CharParser<char>) {
            CPPARSEC_FAIL_IF(input.empty(), ParseError("any_char: end of input"));

            char c = input[0];
            input.remove_prefix(1);
            return c;
        };
    }

    // Parses a single character that satisfies a constraint
    // Faster than try_(any_char().satisfy(cond))
    inline CharParser<char> char_satisfy(UnaryPredicate<char> auto cond, std::string&& err_msg) {
        return CPPARSEC_MAKE(CharParser<char>) {
            CPPARSEC_FAIL_IF(input.empty(), ParseError(err_msg, "end of input"));
            CPPARSEC_FAIL_IF(!cond(input[0]), ParseError(err_msg, { input[0] }));

            char c = input[0];
            input.remove_prefix(1);
            return c;
        };
    }

    // Parses a single string
    inline CharParser<std::string> string_(const std::string& str) {
        return CPPARSEC_MAKE(CharParser<std::string>) {
            CPPARSEC_FAIL_IF(str.size() > input.size(), ParseError("end of input", { str[0] }));

            for (auto [i, c] : str | std::views::enumerate) {
                if (c != input[i]) {
                    char c2 = input[i];
                    auto str2 = std::string(input.substr(0, i + 1));
                    input.remove_prefix(i);

                    CPPARSEC_FAIL(ParseError(c, c2).add_error({ std::pair{ str, str2 } }));
                }
            }

            input.remove_prefix(str.size());
            return str;
        };
    }

    // ======================== String Specializations ========================

    // Parse zero or more characters, std::string specialization
    template <PushBack<char> StringContainer = std::string>
    CharParser<StringContainer> many(CharParser<char> charP) {
        return detail::many_accumulator(charP, StringContainer());
    }

    // Parse one or more characters, std::string specialization
    template <PushBack<char> StringContainer = std::string>
    CharParser<StringContainer> many1(CharParser<char> charP) {
        return CPPARSEC_MAKE(CharParser<StringContainer>) {
            CPPARSEC_SAVE(first, charP);
            CPPARSEC_SAVE(values, detail::many_accumulator(charP, StringContainer({ first })));

            return values;
        };
    }

    // Parses p zero or more times until end succeeds, returning the parsed values, std::string specialization
    template <typename T, PushBack<char> StringContainer = std::string>
    CharParser<StringContainer> many_till(CharParser<char> p, CharParser<T> end) {
        return detail::many_till_accumulator<char, T, StringContainer, CustomStrView>(p, end, StringContainer());
    }

    // Parses p one or more times until end succeeds, returning the parsed values, std::string specialization
    template <typename T, PushBack<char> StringContainer = std::string>
    CharParser<StringContainer> many1_till(CharParser<char> p, CharParser<T> end) {
        return CPPARSEC_MAKE(CharParser<StringContainer>) {
            CPPARSEC_SAVE(first, p);
            CPPARSEC_SAVE(values, detail::many_till_accumulator<char, T, StringContainer, CustomStrView>(p, end, StringContainer({ first })));

            return values;
        };
    }

    // Parse zero or more parses of p separated by sep, std::string specialization
    template <typename T>
    CharParser<std::string> sep_by(CharParser<char> p, CharParser<T> sep) {
        return sep_by1(p, sep) | success<CustomStrView>("");
    }

    // Parse one or more parses of p separated by sep, std::string specialization
    template <typename T>
    CharParser<std::string> sep_by1(CharParser<char> p, CharParser<T> sep) {
        return CPPARSEC_MAKE(CharParser<std::string>) {
            CPPARSEC_SAVE(first, p);
            CPPARSEC_SAVE(values, detail::many_accumulator(sep >> p, std::string(1, first)));

            return values;
        };
    }

    // Parse zero or more parses of p separated by and ending with sep, std::string specialization
    template <typename T>
    CharParser<std::string> end_by(CharParser<char> p, CharParser<T> sep) {
        return many(p << sep);
    }

    // Parse one or more parses of p separated by and ending with sep, std::string specialization
    template <typename T>
    CharParser<std::string> end_by1(CharParser<char> p, CharParser<T> sep) {
        return many1(p << sep);
    }

    // =========================== Numeric Parsers ============================

    // Parses an int
    inline CharParser<int> int_() {
        return many1(digit()).transform([](auto&& s) { return std::stoi(s); });
    }
};

#endif /* CPPARSEC_CHAR_ALT_EXAMPLE_H */
