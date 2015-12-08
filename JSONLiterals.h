#ifndef JSONLITERALS_H_INCLUDED__
#define JSONLITERALS_H_INCLUDED__

#include <array>

namespace JSON {

    template<typename CharT>
    struct LiteralsT;

    template<>
    struct LiteralsT<char> {
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
        static constexpr const char* const whitespace_string()
        {
            return " \t\n\r";
        }
        static constexpr std::array<char, 8> string_escapes()
        {
            return{ '"','\\','/','b','f','n','r','t' };
        }
        static constexpr const char* const begin_number()
        {
            return "-0123456789";
        }
        static constexpr const char* const value_false()
        {
            return "false";
        }
        static constexpr const char* const value_true()
        {
            return "true";
        }
        static constexpr const char* const value_null()
        {
            return "null";
        }
    };

    template<>
    struct LiteralsT<wchar_t> {
        static constexpr auto newline = L'\n';
        static constexpr auto space = L' ';
        static constexpr auto begin_array = L'[';
        static constexpr auto begin_object = L'{';
        static constexpr auto end_array = L']';
        static constexpr auto end_object = L'}';
        static constexpr auto name_separator = L':';
        static constexpr auto value_separator = L',';
        static constexpr auto string_escape = '\\';
        static constexpr auto string_unicode_escape = L'u';
        static constexpr auto quotation_mark = L'"';
        static constexpr auto zero = L'0';
        static constexpr auto minus = L'-';
        static constexpr auto plus = L'+';
        static constexpr auto decimal_point = L'.';
        static constexpr auto exponent_upper = L'e';
        static constexpr auto exponent_lower = L'E';

        static constexpr std::array<wchar_t, 4> whitespace()
        {
            return{ L' ', L'\t', L'\n', L'\r' };
        }
        static constexpr const wchar_t* const whitespace_string()
        {
            return L" \t\n\r";
        }
        static constexpr std::array<wchar_t, 8> string_escapes()
        {
            return{ L'"',L'\\',L'/',L'b',L'f',L'n',L'r',L't' };
        }
        static constexpr const wchar_t* const begin_number()
        {
            return L"-0123456789";
        }
        static constexpr const wchar_t* const value_false()
        {
            return L"false";
        }
        static constexpr const wchar_t* const value_true()
        {
            return L"true";
        }
        static constexpr const wchar_t* const value_null()
        {
            return L"null";
        }
    };

    template<>
    struct LiteralsT<char16_t> {
        static constexpr auto newline = u'\n';
        static constexpr auto space = u' ';
        static constexpr auto begin_array = u'[';
        static constexpr auto begin_object = u'{';
        static constexpr auto end_array = u']';
        static constexpr auto end_object = u'}';
        static constexpr auto name_separator = u':';
        static constexpr auto value_separator = u',';
        static constexpr auto string_escape = '\\';
        static constexpr auto string_unicode_escape = u'u';
        static constexpr auto quotation_mark = u'"';
        static constexpr auto zero = u'0';
        static constexpr auto minus = u'-';
        static constexpr auto plus = u'+';
        static constexpr auto decimal_point = u'.';
        static constexpr auto exponent_upper = u'e';
        static constexpr auto exponent_lower = u'E';
        static constexpr std::array<char16_t, 4> whitespace()
        {
            return{ u' ', u'\t', u'\n', u'\r' };
        }
        static constexpr const char16_t* const whitespace_string()
        {
            return u" \t\n\r";
        }
        static constexpr std::array<char16_t, 8> string_escapes() {
            return{ u'"',u'\\',u'/',u'b',u'f',u'n',u'r',u't' };
        }
        static constexpr const char16_t*const begin_number() {
            return u"-0123456789";
        }
        static constexpr const char16_t*const value_false() { return u"false"; }
        static constexpr const char16_t*const value_true() { return u"true"; }
        static constexpr const char16_t*const value_null() { return u"null"; }
    };

    template<>
    struct LiteralsT<char32_t> {
        static constexpr auto newline = U'\n';
        static constexpr auto space = U' ';
        static constexpr auto begin_array = U'[';
        static constexpr auto begin_object = U'{';
        static constexpr auto end_array = U']';
        static constexpr auto end_object = U'}';
        static constexpr auto name_separator = U':';
        static constexpr auto value_separator = U',';
        static constexpr auto string_escape = '\\';
        static constexpr auto string_unicode_escape = U'u';
        static constexpr auto quotation_mark = U'"';
        static constexpr auto zero = U'0';
        static constexpr auto minus = U'-';
        static constexpr auto plus = U'+';
        static constexpr auto decimal_point = U'.';
        static constexpr auto exponent_upper = U'E';
        static constexpr auto exponent_lower = U'e';
        static constexpr std::array<char32_t, 4> whitespace()
        {
            return{ U' ', U'\t', U'\n', U'\r' };
        }
        static constexpr const char32_t* const whitespace_string()
        {
            return U" \t\n\r";
        }
        static constexpr std::array<char32_t, 8> string_escapes() {
            return{ U'"',U'\\',U'/',U'b',U'f',U'n',U'r',U't' };
        }
        static constexpr const char32_t*const begin_number() {
            return U"-0123456789";
        }
        static constexpr const char32_t*const value_false() { return U"false"; }
        static constexpr const char32_t*const value_true() { return U"true"; }
        static constexpr const char32_t*const value_null() { return U"null"; }
    };

}


#endif //JSONLITERALS_H_INCLUDED__
