#ifndef CPPARSEC_NUMERIC_H
#define CPPARSEC_NUMERIC_H

#include "cpparsec.h"

namespace cpparsec {
    namespace detail {
        int fast_stoi(std::string& s) {
            int value;
            std::from_chars(s.data(), s.data() + s.size(), value);
            return value;
        }
    }

    // =========================== Numeric Parsers ============================

    // Parses an int
    inline Parser<int> int_() {
        return CPPARSEC_MAKE(Parser<int>) {
            CPPARSEC_SAVE(sign, optional_result(char_('-') | char_('+')));
            CPPARSEC_SAVE(digits_str, many1(digit()));

            int num_sign = (sign.has_value() && sign.value() == '-') ? -1 : 1;
            return num_sign * std::stoi(digits_str);
        };
    }

    // Parses an int
    inline Parser<int> int_fromchar() {
        return CPPARSEC_MAKE(Parser<int>) {
            CPPARSEC_SAVE(sign, optional_result(char_('-') | char_('+')));
            CPPARSEC_SAVE(digits, many1(digit()));

            int num_sign = (sign.has_value() && sign.value() == '-') ? -1 : 1;
            return num_sign * detail::fast_stoi(digits);
        };
    }


    // Parses an int
    inline Parser<int> int_alt() {
        return CPPARSEC_MAKE(Parser<int>) {
            int sign = 1;

            if (!input.empty()) {
                if (input.front() == '-') {
                    sign = -1;
                    input.remove_prefix(1);
                }
                else if (input.front() == '+') {
                    input.remove_prefix(1);
                }
            }

            CPPARSEC_SAVE(digits_str, many1(digit()));

            return sign * detail::fast_stoi(digits_str);
        };
    }

    // Parses an int
    inline Parser<int> int_2() {
        return (optional_result(char_('-') | char_('+')) & many1(digit()))
            .transform([](auto&& s) constexpr { 
                auto&& [sign, digits_str] = s;
                int num_sign = (sign.has_value() && sign.value() == '-') ? -1 : 1;
                return num_sign * std::stoi(digits_str);
            });
    }

    // Parses an unsigned int
    inline Parser<unsigned int> uint() {
        return CPPARSEC_MAKE(Parser<unsigned int>) {
            CPPARSEC_SAVE(digits_str, many1(digit()));
            return std::stoi(digits_str);
        };
    }

    // Parses an unsigned int
    inline Parser<int> uint_2() {
        return many1(digit()).transform([](auto&& s) constexpr { return std::stoi(s); });
    }

};

#endif /* CPPARSEC_NUMERIC_H */
