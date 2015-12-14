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

#ifndef JSONPARSER_H_
#define JSONPARSER_H_

#include <type_traits>
#include <utility>  // pair
#include <string>
#include <tuple>
#include <vector>
#include <stack>
#include <exception>
#include <locale>  // isdigit
#include <functional>  // function
#include <limits>
#include <cmath> // pow

#include "JSONFwd.h"
#include "JSON.h"
#include "JSONLiterals.h"
#include "JSONObjectsFwd.h"
#include "JSONIteratorFwd.h"
#include "JSONObjects.h"
#include "JSONParserFwd.h"

  // TODO(fecjanky): P2 Add Allocator support for parsers
  // TODO(fecjanky): P3 Add Parsing Startegy (Mutable, Immutable)

namespace JSON {

namespace impl {

template<typename ... Parsers>
ISubParser& DispatchFirstSymbol(IParser& p, std::tuple<Parsers...>& parsers);

template<typename JSONLiteral>
const char* GetLiteral() {
    return JSONLiteral::Literal();
}

inline bool IsWhitespace(char c) {
    for (auto w : JSON::Literals::Whitespace())
        if (std::char_traits<char>::eq(c, w))
            return true;
    return false;
}

template<typename Parser, typename ParserState>
static ISubParser& StateTransition(Parser& current_parser, ParserState& state,
        IParser& parser) {
    throw typename Parser::Exception();
}

template<typename Parser, typename ParserState, typename Predicate,
        typename Action, typename NextState, typename ... Predicates,
        typename ... Actions, typename ... NextStates>
static ISubParser& StateTransition(Parser& current_parser, ParserState& s,
        IParser& parser, std::tuple<Predicate, Action, NextState>&& p,
        std::tuple<Predicates, Actions, NextStates>&&... ps) {
    static_assert(
            std::is_base_of<ISubParser, Parser>::value &&
            std::is_base_of<ISubParserState, ParserState>::value,
            "State transition not possible");

    if (std::get<0>(p)(parser.getCurrentChar())) {
        auto& state = ISubParserState::Cast(current_parser, s);
        state.stateFunc = std::get<2>(p);
        return std::get<1>(p)(current_parser, state, parser);
    } else
        return StateTransition(current_parser, s, parser, std::move(ps)...);
}

struct NoOp {
    template<typename ParserT, typename StateT>
    ISubParser& operator()(ParserT& current_parser, StateT& s, IParser&p) {
        return current_parser;
    }
};

struct Store {
    static char DefaultTransform(char c) noexcept {
        return c;
    }
    using Transformer = std::function<char(char)noexcept>;
    explicit Store(Transformer t = Transformer{ DefaultTransform }) :
        transformer{ t } {}
    template<typename ParserT, typename StateT>
    ISubParser& operator()(ParserT& current_parser, StateT& s, IParser&p) {
        static_assert(std::is_same<std::string, decltype(s.token)>::value,
                "State member token is not type of string");
        s.token.push_back(transformer(p.getCurrentChar()));
        return current_parser;
    }
private:
    Transformer transformer;
};

template<typename NextParserT>
struct Call {
    template<typename ParserT, typename StateT>
    ISubParser& operator()(ParserT& current_parser, StateT& s, IParser&p) {
        p.getStateStack().push(
                ISubParserState::template Create<NextParserT>(p));
        return std::get<NextParserT>(p.getParsers())(p);
    }
};

struct Dispatch {
    template<typename ParserT, typename StateT>
    ISubParser& operator()(ParserT& current_parser, StateT& s, IParser&p) {
        return DispatchFirstSymbol(p, p.getParsers());
    }
};

template<typename JSONT>
struct Return {
    template<typename ParserT, typename StateT>
    ISubParser& operator()(ParserT& current_parser, StateT& s, IParser&p) {
        static_assert(std::is_same<JSON::IObjectPtr,
                decltype(s.object)>::value,
                "State member object is not type of Object pointer");
        s.object = JSON::Create<JSONT>(std::move(s.token));
        return current_parser.nextParser(p);
    }
};

struct NoOpReturn {
    template<typename ParserT, typename StateT>
    ISubParser& operator()(ParserT& current_parser, StateT& s, IParser&p) {
        return current_parser.nextParser(p);
    }
};

template<char ... Literal>
class IsLiteral {
 public:
    bool operator()(char c) {
        return eval(c, Literal...);
    }
 private:
    bool eval(char c) {
        return false;
    }
    template<typename Char, typename ... Chars>
    bool eval(char c, Char C, Chars ... Cs) {
        if (std::char_traits<char>::eq(c, C))
            return true;
        else
            return eval(c, Cs...);
    }
};

struct IsStringChar {
    bool operator()(char c) {
        using ct = std::char_traits<char>;
         // TODO(fecjanky): P2 find correct logic for checking string chars
        return ct::eq(c, ct::to_char_type(0x20))
                || ct::eq(c, ct::to_char_type(0x21))
                || (gt_eq(c, ct::to_char_type(0x23))
                        && lt_eq(c, ct::to_char_type(0x5b)))
                || gt_eq(c, ct::to_char_type(0x5D));
    }

 private:
    static bool lt_eq(char lhs, char rhs) {
        using ct = std::char_traits<char>;
        return ct::lt(lhs, rhs) || ct::eq(lhs, rhs);
    }

    static bool gt(char lhs, char rhs) {
        using ct = std::char_traits<char>;
        return !ct::lt(lhs, rhs) && !ct::eq(lhs, rhs);
    }

    static bool gt_eq(char lhs, char rhs) {
        using ct = std::char_traits<char>;
        return !ct::lt(lhs, rhs);
    }
};

struct IsDigit {
    bool operator()(char c) {
        return std::isdigit(c, std::locale { });
    }
};

struct Others {
    bool operator()(char c) {
        return true;
    }
};

struct CheckSymbol {
    bool operator()(char c, const char* s) {
        using ct = std::char_traits<char>;
        return ct::find(s, ct::length(s), c) != nullptr;
    }

    bool operator()(char c, char s) {
        return std::char_traits<char>::eq(c, s);
    }
};

ISubParser& CheckStartSymbol(IParser& IParser) {
    throw InvalidStartingSymbol();
}

template<typename SubParserT, typename ... SubParsersT>
ISubParser& CheckStartSymbol(IParser& parser, SubParserT& subparser,
        SubParsersT&... subparsers) {
    if (!std::is_same<WSParser, std::decay_t<SubParserT>> { }
            && CheckSymbol { }(parser.getCurrentChar(),
                    subparser.getFirstSymbolSet())) {
        parser.getStateStack().push(
                ISubParserState::Create<SubParserT>(parser));
        return subparser(parser);
    } else {
        return CheckStartSymbol(parser, subparsers...);
    }
}

template<typename ... Parsers>
ISubParser& DispatchFirstSymbol(IParser& p, std::tuple<Parsers...>& parsers) {
    return CheckStartSymbol(p, std::get<Parsers>(parsers)...);
}

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

template<typename ParserT>
inline ISubParser& ParserTemplate<ParserT>::operator()(IParser& p) {
    auto& s = ISubParserState::Cast(*this, *p.getStateStack().top());
    return (static_cast<ParserT*>(this)->*(s.stateFunc))(s, p);
}

template<typename ParserT>
inline ISubParser&
ParserTemplate<ParserT>::nextParser(IParser& p) noexcept {
    p.getLastState() = std::move(p.getStateStack().top());
    p.getStateStack().pop();
     // If last parser in stack
     // add last object to output collection
    if (p.getStateStack().size() == 1) {
        p.getObjects().emplace_back(std::move(p.getLastState()->getObject()));
    }
    return p.getStateStack().top()->getParser(p);
}

template<typename ParserT>
inline IObjectPtr ParserTemplate<ParserT>::State::getObject() {
    return nullptr;
}

template<typename ParserT>
inline ISubParser&
ParserTemplate<ParserT>::State::getParser(IParser& p) {
    return std::get<ParserT>(p.getParsers());
}

template<typename ParserT>
inline ParserTemplate<ParserT>::State::State(IParser& p) :
        stateFunc { ParserT::getInitState() } {
}

template<class JSONLiteral>
inline LiteralParser<JSONLiteral>::State::State(IParser& p) :
        Base(p), current_pos { } {
}

template<class JSONLiteral>
inline IObjectPtr LiteralParser<JSONLiteral>::State::getObject() {
    return JSON::Create<JSONLiteral>();
}

template<class JSONLiteral>
inline ISubParser&
LiteralParser<JSONLiteral>::check(ISubParserState& s, IParser& p) {
    auto& state = ISubParserState::Cast(*this, s);
    using ct = std::char_traits<char>;
    const size_t literal_size = ct::length(GetLiteral<JSONLiteral>());
    if (!ct::eq(GetLiteral<JSONLiteral>()[state.current_pos++],
            p.getCurrentChar())) {
        throw LiteralException();
    } else if (state.current_pos == literal_size) {
        return this->nextParser(p);
    } else {
        return *this;
    }
}

template<class JSONLiteral>
inline char LiteralParser<JSONLiteral>::getFirstSymbolSet() {
    return GetLiteral<JSONLiteral>()[0];
}

template<class JSONLiteral>
inline typename LiteralParser<JSONLiteral>::StatePtr
LiteralParser<JSONLiteral>::getInitState() {
    return &LiteralParser::check;
}

inline IObjectPtr NumberParser::State::getObject() {
    return std::move(object);
}

inline char NumberParser::ExtractDigit(char d) noexcept {
    return d - Literals::zero;
}

template<typename Int>
inline void NumberParser::StoreInteger::
overflowCheck(Int d) {
    constexpr auto maxTreshold = 
        std::numeric_limits<Int>::max() / 10;
    constexpr auto minTreshold =
        std::numeric_limits<Int>::min() / 10;
    if (d > maxTreshold || d < minTreshold)
        throw IntegerOverflow{};
}

inline ISubParser& NumberParser::StoreInteger::
operator()(NumberParser& cp, NumberParser::State& s, IParser& p) {
    overflowCheck(s.integerPart);
    auto c = p.getCurrentChar();
    s.token.push_back(c);
    s.integerPart = 10 * s.integerPart + ExtractDigit(c);
    return cp;
}

inline ISubParser& NumberParser::StoreSign::
operator()(NumberParser& cp, NumberParser::State& s, IParser& p) {
    auto c = p.getCurrentChar();
    if (std::char_traits<char>::eq(c, Literals::minus))
        s.sign = true;
    s.token.push_back(p.getCurrentChar());
    return cp;
}

inline ISubParser& NumberParser::StoreFraction::
operator()(NumberParser& cp, NumberParser::State& s, IParser& p) {
    auto c = p.getCurrentChar();
    s.token.push_back(c);
    s.fractionPart += ExtractDigit(c)*pow(10.0, s.fractionPos--);
    return cp;
}

inline ISubParser& NumberParser::StoreExponent::
operator()(NumberParser& cp, NumberParser::State& s, IParser& p) {
    StoreInteger::overflowCheck(s.exponentPart);
    auto c = p.getCurrentChar();
    s.token.push_back(c);
    s.exponentPart = 10 * s.exponentPart + ExtractDigit(c);
    return cp;
}

inline ISubParser& NumberParser::StoreExponentSign::
operator()(NumberParser& cp, NumberParser::State& s, IParser& p) {
    auto c = p.getCurrentChar();
    s.token.push_back(c);
    if(std::char_traits<char>::eq(c,Literals::minus))
        s.expSigned = true;
    return cp;
}

ISubParser& NumberParser::CallBack::operator()(NumberParser& cp, NumberParser::State& s, IParser&p) {
    auto exp = !s.expSigned ? s.exponentPart : -1 * s.exponentPart;
    auto base = !s.sign ? (s.integerPart + s.fractionPart) : -1.0*(s.integerPart + s.fractionPart);
    auto num = base*pow(10.0, exp);
    s.object = JSON::Create<JSON::Number>(num);
    return cp.nextParser(p)(p);
}


inline const char* NumberParser::getFirstSymbolSet() {
    return Literals::BeginNumber();
}

inline NumberParser::StatePtr NumberParser::getInitState() {
    return &NumberParser::start;
}

inline ISubParser& NumberParser::start(ISubParserState& state, IParser& p) {
    using L = Literals;
    return StateTransition(*this, state, p,
            std::make_tuple(IsLiteral<L::zero> { }, Store { },
                    &NumberParser::startingZero),
            std::make_tuple(IsDigit { }, StoreInteger { }, 
                &NumberParser::integerPart),
            std::make_tuple(IsLiteral<L::minus> { }, StoreSign{ },
                    &NumberParser::minus));
}

inline ISubParser& NumberParser::minus(ISubParserState& state, IParser& p) {
    using L = Literals;
    return StateTransition(*this, state, p,
            std::make_tuple(IsLiteral<L::zero> { }, Store{ },
                    &NumberParser::startingZero),
            std::make_tuple(IsDigit { }, StoreInteger{ },
                    &NumberParser::integerPart));
}

inline ISubParser& NumberParser::startingZero(ISubParserState& state,
        IParser& p) {
    using L = Literals;
    return StateTransition(*this, state, p,
            std::make_tuple(IsLiteral<L::decimalPoint> { }, Store{ },
                    &NumberParser::fractionPartStart),
            std::make_tuple(Others { }, CallBack { }, nullptr));
}

inline ISubParser& NumberParser::integerPart(ISubParserState& state,
        IParser& p) {
    using L = Literals;
    return StateTransition(*this, state, p,
            std::make_tuple(IsDigit { }, StoreInteger { },
                &NumberParser::integerPart),
            std::make_tuple(IsLiteral<L::decimalPoint> { }, Store { },
                    &NumberParser::fractionPartStart),
            std::make_tuple(IsLiteral<L::exponentUpper, L::exponentLower> { },
                Store{ }, &NumberParser::exponentPartStart),
            std::make_tuple(Others { }, CallBack{ }, nullptr));
}

inline ISubParser& NumberParser::fractionPartStart(ISubParserState& state,
        IParser& p) {
    return StateTransition(*this, state, p,
            std::make_tuple(IsDigit { }, StoreFraction { },
                    &NumberParser::fractionPart));
}

inline ISubParser& NumberParser::fractionPart(ISubParserState& state,
        IParser& p) {
    using L = Literals;
    return StateTransition(*this, state, p,
            std::make_tuple(IsDigit { }, StoreFraction { },
                    &NumberParser::fractionPart),
            std::make_tuple(IsLiteral<L::exponentUpper, L::exponentLower> { },
                Store{ }, &NumberParser::exponentPartStart),
            std::make_tuple(Others { }, CallBack { }, nullptr));
}

inline ISubParser& NumberParser::exponentPartStart(ISubParserState& state,
        IParser& p) {
    using L = Literals;
    return StateTransition(*this, state, p,
            std::make_tuple(IsLiteral<L::minus, L::plus> { },
                StoreExponentSign{ }, &NumberParser::exponentPartStartSigned),
            std::make_tuple(IsDigit { }, StoreExponent { },
                    &NumberParser::exponentPart));
}

inline ISubParser& NumberParser::exponentPartStartSigned(ISubParserState& state,
        IParser& p) {
    return StateTransition(*this, state, p,
            std::make_tuple(IsDigit { }, StoreExponent { },
                    &NumberParser::exponentPart));
}

inline ISubParser& NumberParser::exponentPart(ISubParserState& state,
        IParser& p) {
    return StateTransition(*this, state, p,
            std::make_tuple(IsDigit { }, StoreExponent { },
                    &NumberParser::exponentPart),
            std::make_tuple(Others { }, CallBack{ }, nullptr));
}

inline ObjectParser::State::State(IParser& p) :
        ParserTemplate<ObjectParser>::State(p), object(
                JSON::Create<JSON::Object>()) {
}

inline IObjectPtr ObjectParser::State::getObject() {
    return std::move(object);
}

inline char ObjectParser::getFirstSymbolSet() {
    return Literals::beginObject;
}

inline ObjectParser::StatePtr ObjectParser::getInitState() {
    return &ObjectParser::start;
}

inline ISubParser& ObjectParser::start(ISubParserState& s, IParser& p) {
    return StateTransition(*this, s, p,
            std::make_tuple(IsLiteral<Literals::beginObject> { }, NoOp { },
                    &ObjectParser::parseKey));
}

inline ISubParser& ObjectParser::parseKey(ISubParserState& s, IParser& p) {
    return StateTransition(*this, s, p,
            std::make_tuple(IsWhitespace, NoOp { }, &ObjectParser::parseKey),
            std::make_tuple(IsLiteral<Literals::endObject> { }, NoOpReturn { },
                    nullptr),
            std::make_tuple(IsLiteral<Literals::quotationMark> { },
                    Call<StringParser> { }, &ObjectParser::retrieveKey));
}

inline ISubParser& ObjectParser::retrieveKey(ISubParserState& s, IParser& p) {
    auto& state = ISubParserState::Cast(*this, s);
    state.currentKey = std::move(p.getLastState()->getObject()->getValue());
    return parseSeparator(s, p);
}

inline ISubParser& ObjectParser::parseSeparator(ISubParserState& s,
        IParser& p) {
    return StateTransition(*this, s, p,
            std::make_tuple(IsWhitespace, NoOp { },
                    &ObjectParser::parseSeparator),
            std::make_tuple(IsLiteral<Literals::nameSeparator> { }, NoOp { },
                    &ObjectParser::parseValue));
}

inline ISubParser& ObjectParser::parseValue(ISubParserState& s, IParser& p) {
    return StateTransition(*this, s, p,
            std::make_tuple(IsWhitespace, NoOp { }, &ObjectParser::parseValue),
            std::make_tuple(Others { }, Dispatch { },
                    &ObjectParser::retrieveValue));
}

inline ISubParser& ObjectParser::retrieveValue(ISubParserState& s, IParser& p) {
    auto& state = ISubParserState::Cast(*this, s);
    auto& currentObject = static_cast<Object&>(*state.object);

    currentObject.emplace(std::move(state.currentKey),
            std::move(p.getLastState()->getObject()));

    return nextMember(s, p);
}

ISubParser& ObjectParser::nextMember(ISubParserState& s, IParser& p) {
    return StateTransition(*this, s, p,
            std::make_tuple(IsWhitespace, NoOp { }, &ObjectParser::nextMember),
            std::make_tuple(IsLiteral<Literals::endObject> { }, NoOpReturn { },
                    nullptr),
            std::make_tuple(IsLiteral<Literals::valueSeparator> { }, NoOp { },
                    &ObjectParser::parseKey));
}

inline ArrayParser::State::State(IParser& p) :
        ParserTemplate<ArrayParser>::State(p), object {
                JSON::Create<JSON::Array>() } {
}

inline IObjectPtr ArrayParser::State::getObject() {
    return std::move(object);
}

inline char ArrayParser::getFirstSymbolSet() {
    return Literals::beginArray;
}

inline ArrayParser::StatePtr ArrayParser::getInitState() {
    return &ArrayParser::start;
}

inline ISubParser& ArrayParser::start(ISubParserState& s, IParser& p) {
    return StateTransition(*this, s, p,
            std::make_tuple(IsLiteral<Literals::beginArray> { }, NoOp { },
                    &ArrayParser::parseValue));
}

inline ISubParser& ArrayParser::parseValue(ISubParserState& s, IParser& p) {
    return StateTransition(*this, s, p,
            std::make_tuple(IsWhitespace, NoOp { }, &ArrayParser::parseValue),
            std::make_tuple(Others { }, Dispatch { },
                    &ArrayParser::retrieveValue));
}

inline ISubParser& ArrayParser::retrieveValue(ISubParserState& s, IParser& p) {
    auto& state = ISubParserState::Cast(*this, s);
    auto& currentObject = static_cast<Array&>(*state.object);

    currentObject.emplace(std::move(p.getLastState()->getObject()));

    return nextMember(s, p);
}

ISubParser& ArrayParser::nextMember(ISubParserState& s, IParser& p) {
    return StateTransition(*this, s, p,
            std::make_tuple(IsWhitespace, NoOp { }, &ArrayParser::nextMember),
            std::make_tuple(IsLiteral<Literals::endArray> { }, NoOpReturn { },
                    nullptr),
            std::make_tuple(IsLiteral<Literals::valueSeparator> { }, NoOp { },
                    &ArrayParser::parseValue));
}

inline const char* WSParser::getFirstSymbolSet() {
    return Literals::WhitespaceString();
}

inline WSParser::StatePtr WSParser::getInitState() {
    return &WSParser::dispatch;
}

inline ISubParser& WSParser::dispatch(ISubParserState& state, IParser& p) {
    if (IsWhitespace(p.getCurrentChar())) {
        return *this;
    } else {
        return DispatchFirstSymbol(p, p.getParsers());
    }
}

inline IObjectPtr StringParser::State::getObject() {
    return std::move(object);
}

inline char StringParser::getFirstSymbolSet() {
    return Literals::quotationMark;
}

inline StringParser::StatePtr StringParser::getInitState() {
    return &StringParser::start;
}

inline ISubParser& StringParser::start(ISubParserState& s, IParser& p) {
    return StateTransition(*this, s, p,
            std::make_tuple(IsLiteral<Literals::quotationMark> { }, NoOp { },
                    &StringParser::parseChar));
}

inline ISubParser& StringParser::parseChar(ISubParserState& s, IParser& p) {
    return StateTransition(*this, s, p,
            std::make_tuple(IsLiteral<Literals::quotationMark> { },
                    Return<MyJSONType> { }, nullptr),
            std::make_tuple(IsLiteral<Literals::stringEscape> { }, NoOp { },
                    &StringParser::parseEscapeChar),
            std::make_tuple(IsStringChar { }, Store { },
                    &StringParser::parseChar));
}

inline ISubParser& StringParser::parseEscapeChar(ISubParserState& s,
        IParser& p) {
    return StateTransition(*this, s, p,
            std::make_tuple(
                    IsLiteral<JSON::Literals::stringUnicodeEscape> { },
                    NoOp { }, &StringParser::parseUnicodeEscapeChar),
            std::make_tuple([](char c) -> bool {
                for (auto e : JSON::Literals::StringEscapes())
                    if (std::char_traits<char>::eq(e, c))
                        return true;
                return false;
            }, Store { JSON::Literals::EscapeToNative },
               &StringParser::parseChar));
}

inline ISubParser& StringParser::parseUnicodeEscapeChar(ISubParserState& s,
        IParser& p) {
     // throws exception!
     // TODO(fecjanky): P3 implement parsing of Unicode escapes
    return StateTransition(*this, s, p);
}

}  // namespace impl

class Parser: public impl::IParser {
 public:
    Parser() :
            current_char { } {
        stateStack.push(impl::ISubParserState::Create<impl::WSParser>(*this));
    }

    ObjContainer retrieveObjects() {
        if (!isRetrievable())
            throw impl::ParsingIncomplete();
        return std::move(objects);
    }

    bool isRetrievable() const noexcept {
        return stateStack.size() == 1;
    }

    operator bool() const noexcept {
        return isRetrievable();
    }

    void operator()(char c) {
        current_char = c;
        stateStack.top()->getParser(*this)(*this);
    }

    Parser(const Parser&) = delete;
    Parser& operator=(const Parser&) = delete;
    Parser(Parser&&) = delete;
    Parser& operator=(Parser&&) = delete;

 private:
    using ParserTuple = impl::ParserTuple;
    using ParserStateStack = impl::ParserStateStack;
    using ISubParserStatePtr = impl::ISubParserStatePtr;

    char getCurrentChar() const noexcept override {
        return current_char;
    }

    ObjContainer& getObjects() noexcept override {
        return objects;
    }

    ParserTuple& getParsers() noexcept override {
        return parsers;
    }

    ParserStateStack& getStateStack() noexcept override {
        return stateStack;
    }

    ISubParserStatePtr& getLastState() noexcept override {
        return lastState;
    }

    char current_char;
    ObjContainer objects;
    ParserTuple parsers;
    ParserStateStack stateStack;
    ISubParserStatePtr lastState;
};

template<typename ForwardIterator>
ObjContainer parse(ForwardIterator start, ForwardIterator end) {
    static_assert(
            std::is_same<
            char,
            std::decay_t<decltype(*(std::declval<ForwardIterator>()))>
            >::value,
            "ForwardIterator does not dereference to char");
    Parser p;

    for (; start != end; ++start) {
        p(*start);
    }

    return p.retrieveObjects();
}

}  // namespace JSON

#endif  // JSONPARSER_H_





