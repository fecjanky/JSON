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
    static constexpr auto beginArray = '[';
    static constexpr auto beginObject = '{';
    static constexpr auto endArray = ']';
    static constexpr auto endObject = '}';
    static constexpr auto nameSeparator = ':';
    static constexpr auto valueSeparator = ',';
    static constexpr auto stringEscape = '\\';
    static constexpr auto stringUnicodeEscape = 'u';
    static constexpr auto quotationMark = '"';
    static constexpr auto zero = '0';
    static constexpr auto minus = '-';
    static constexpr auto plus = '+';
    static constexpr auto decimalPoint = '.';
    static constexpr auto exponentUpper = 'e';
    static constexpr auto exponentLower = 'E';
    static constexpr std::array<char, 4> Whitespace() {
        return {' ', '\t', '\n', '\r'};
    }
    static constexpr const char* WhitespaceString() {
        return u8" \t\n\r";
    }
    static constexpr std::array<char, 8> StringEscapes() {
        return {'"', '\\', '/', 'b', 'f', 'n', 'r', 't'};
    }
    static char EscapeToNative(char c) noexcept {
        switch (c) {
        case 'b': return '\b';
        case 'f': return '\f';
        case 'n': return '\n';
        case 'r': return '\r';
        case 't': return '\t';
        default: return c;
        }
    }
    static constexpr const char* BeginNumber() {
        return u8"-0123456789";
    }
    static constexpr const char* ValueFalse() {
        return u8"false";
    }
    static constexpr const char* ValueTrue() {
        return u8"true";
    }
    static constexpr const char* ValueNull() {
        return u8"null";
    }
};

}  // namespace JSON

#endif  // JSONLITERALS_H_
