#ifndef JSONLITERALS_H_INCLUDED__
#define JSONLITERALS_H_INCLUDED__

#include <array>

namespace JSON {
    
    struct Literals {
        static constexpr auto newline = '\n';
        static constexpr auto space = ' ';
        static constexpr auto begin_array = '[';
        static constexpr auto begin_object = '{';
        static constexpr auto end_array = ']';
        static constexpr auto end_object = '}';
        static constexpr auto name_separator = ':';
        static constexpr auto value_separator = ',';
        static constexpr auto string_escape = '\\';
        static constexpr auto string_unicode_escape = 'u';
        static constexpr auto quotation_mark = '"';
        static constexpr auto zero = '0';
        static constexpr auto minus = '-';
        static constexpr auto plus = '+';
        static constexpr auto decimal_point = '.';
        static constexpr auto exponent_upper = 'e';
        static constexpr auto exponent_lower = 'E';
        static constexpr std::array<char, 4> whitespace()
        {
            return{ ' ', '\t', '\n', '\r' };
        }
        static constexpr const char* whitespace_string()
        {
            return u8" \t\n\r";
        }
        static constexpr std::array<char, 8> string_escapes()
        {
            return{ '"','\\','/','b','f','n','r','t' };
        }
        static constexpr const char* begin_number()
        {
            return u8"-0123456789";
        }
        static constexpr const char* value_false()
        {
            return u8"false";
        }
        static constexpr const char* value_true()
        {
            return u8"true";
        }
        static constexpr const char* value_null()
        {
            return u8"null";
        }
    };

}


#endif //JSONLITERALS_H_INCLUDED__
