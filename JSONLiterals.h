// Copyright (c) 2015 Ferenc Nandor Janky
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef JSONLITERALS_H_
#define JSONLITERALS_H_

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
    static constexpr std::array<char, 4> whitespace() {
        return {' ', '\t', '\n', '\r'};
    }
    static constexpr const char* whitespace_string() {
        return u8" \t\n\r";
    }
    static constexpr std::array<char, 8> string_escapes() {
        return {'"', '\\', '/', 'b', 'f', 'n', 'r', 't'};
    }
    static constexpr const char* begin_number() {
        return u8"-0123456789";
    }
    static constexpr const char* value_false() {
        return u8"false";
    }
    static constexpr const char* value_true() {
        return u8"true";
    }
    static constexpr const char* value_null() {
        return u8"null";
    }
};

}  // namespace JSON

#endif  // JSONLITERALS_H_
