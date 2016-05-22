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

#ifndef JSONPARSERFWD_H_
#define JSONPARSERFWD_H_

#include <tuple>
#include <vector>
#include <string>
#include <stack>

#include "memory.h"

#include "JSONFwd.h"
#include "JSON.h"
#include "JSONUtils.h"

namespace JSON {

using ObjContainer = std::vector<JSON::IObject::Ptr, estd::poly_alloc_wrapper<JSON::IObject::Ptr>>;
//using ObjContainer = std::vector<JSON::IObject::Ptr>;

namespace impl {

struct IParser;
struct ISubParser;
struct ISubParserState;
template<typename ParserT>
class ParserTemplate;
class WSParser;
template<class JSONLiteral>
class LiteralParser;
using NullParser = LiteralParser<Null>;
using TrueParser = LiteralParser<True>;
using FalseParser = LiteralParser<False>;
class ObjectParser;
class ArrayParser;
class NumberParser;
class StringParser;

using ParserTuple = std::tuple<
WSParser,
ObjectParser,
ArrayParser,
StringParser,
NumberParser,
TrueParser,
FalseParser,
NullParser
>;

using ISubParserStatePtr = Utils::UniquePtr<ISubParserState>;
using ParserStateStack = std::stack<ISubParserStatePtr,std::deque<ISubParserStatePtr,estd::poly_alloc_wrapper<ISubParserStatePtr>>>;

struct Exception: public std::exception {
};
struct InvalidStartingSymbol: public impl::Exception {
};
struct LiteralException: public impl::Exception {
};
struct ParsingIncomplete: public impl::Exception {
};

struct ISubParser {
    virtual ISubParser& operator()(IParser& p) = 0;
    virtual ~ISubParser() = default;
};

struct ISubParserState {
    virtual ISubParser& getParser(IParser& p) = 0;
    virtual IObject::Ptr getObject() = 0;
    virtual ~ISubParserState() = default;

    template<typename SubParserImpl>
    static typename SubParserImpl::State& Cast(const SubParserImpl&,
            ISubParserState& s) {
        return static_cast<typename SubParserImpl::State&>(s);
    }

    template<typename SubParserT, typename = std::enable_if_t<
            std::is_base_of<ISubParser, SubParserT>::value> >
    static ISubParserStatePtr Create(IParser& p) {
        using State = typename SubParserT::State;
        return Utils::ToUniquePtr<ISubParserState>(Utils::MakeUnique<State>(std::allocator_arg, p.getAllocator(),State(p)));
    }
};

struct IParser {
    struct Exception: public std::exception {
    };
    virtual char getCurrentChar() const noexcept = 0;
    virtual ObjContainer& getObjects() noexcept = 0;
    virtual ParserTuple& getParsers() noexcept = 0;
    virtual ParserStateStack& getStateStack() noexcept = 0;
    virtual ISubParserStatePtr& getLastState() noexcept = 0;
    virtual estd::poly_alloc_t& getAllocator() noexcept = 0;
 protected:
    virtual ~IParser() = default;
};

template<typename ParserT>
class ParserTemplate: public ISubParser {
 public:
    using StatePtr = ISubParser& (ParserT::*)(ISubParserState&, IParser&);

    class State: public ISubParserState {
     public:
        explicit State(IParser& p);
        ISubParser& getParser(IParser& p) override;
        IObject::Ptr getObject() override;

        StatePtr stateFunc;
    };

    ISubParser& operator()(IParser& p) override;
    ISubParser& nextParser(IParser& p) noexcept;
};

template<class JSONLiteral>
class LiteralParser: public ParserTemplate<LiteralParser<JSONLiteral>> {
 public:
    using Base = ParserTemplate<LiteralParser<JSONLiteral>>;
    using MyJSONType = JSONLiteral;
    using BaseState = typename Base::State;
    using StatePtr = typename Base::StatePtr;

    struct State: public BaseState {
        using Base = BaseState;
        explicit State(IParser& p);
        IObject::Ptr getObject() override;
        size_t current_pos;
        IObject::Ptr obj;
    };

    static char getFirstSymbolSet();
    static StatePtr getInitState();

 private:
    ISubParser& check(ISubParserState& state, IParser& p);
};

class NumberParser : public ParserTemplate<NumberParser> {
public:
    struct Exception : impl::Exception {
    };
    struct IntegerOverflow : public Exception {};

    using MyJSONType = JSON::Number;
    using Base = ParserTemplate<NumberParser>;
    using BaseState = Base::State;
    using StatePtr = Base::StatePtr;

    struct State: public BaseState {
        State(IParser& p) : BaseState(p), token(p.getAllocator()),
            integerPart{}, sign{}, fractionPart{}, 
            fractionPos{ -1 }, exponentPart{}, expSigned{} {
            token.reserve(64);
        }

        IObject::Ptr getObject() override;

        IObject::Ptr object;
        IObject::StringType token;
        int integerPart;
        bool sign;
        double fractionPart;
        int fractionPos;
        int exponentPart;
        bool expSigned;
    };

    static char ExtractDigit(char d) noexcept;
    struct StoreInteger {
        template<typename Int>
        static void overflowCheck(Int s);
        ISubParser& operator()(
            NumberParser& cp, NumberParser::State& s, IParser& p);
    };

    struct StoreFraction {
        ISubParser& operator()(
            NumberParser& cp, NumberParser::State& s, IParser& p);
    };

    struct StoreExponent {
        ISubParser& operator()(
            NumberParser& cp, NumberParser::State& s, IParser& p);
    };

    struct StoreExponentSign {
        ISubParser& operator()(
            NumberParser& cp, NumberParser::State& s, IParser& p);
    };

    struct StoreSign {
        ISubParser& operator()(
            NumberParser& cp, NumberParser::State& s, IParser& p);
    };

    struct CallBack {
        ISubParser& operator()(NumberParser& cp, NumberParser::State& s, IParser&p);
    };

    static const char* getFirstSymbolSet();
    static StatePtr getInitState();

 private:
    ISubParser& start(ISubParserState& state, IParser& p);
    ISubParser& minus(ISubParserState& state, IParser& p);
    ISubParser& startingZero(ISubParserState& state, IParser& p);
    ISubParser& integerPart(ISubParserState& state, IParser& p);
    ISubParser& fractionPartStart(ISubParserState& state, IParser& p);
    ISubParser& fractionPart(ISubParserState& state, IParser& p);
    ISubParser& exponentPartStart(ISubParserState& state, IParser& p);
    ISubParser& exponentPartStartSigned(ISubParserState& state, IParser& p);
    ISubParser& exponentPart(ISubParserState& state, IParser& p);
};

class ObjectParser: public ParserTemplate<ObjectParser> {
 public:
    struct Exception: impl::Exception {
    };
    using MyJSONType = JSON::Object;
    using Base = ParserTemplate<ObjectParser>;
    using StatePtr = Base::StatePtr;

    struct State: public Base::State {
        explicit State(IParser& p);
        IObject::Ptr getObject() override;

        IObject::Ptr object;
        IObject::StringType currentKey;
    };

    static char getFirstSymbolSet();
    static StatePtr getInitState();

 private:
    ISubParser& start(ISubParserState& s, IParser& p);
    ISubParser& parseKey(ISubParserState& s, IParser& p);
    ISubParser& retrieveKey(ISubParserState& s, IParser& p);
    ISubParser& parseSeparator(ISubParserState& s, IParser& p);
    ISubParser& parseValue(ISubParserState& s, IParser& p);
    ISubParser& retrieveValue(ISubParserState& s, IParser& p);
    ISubParser& nextMember(ISubParserState& s, IParser& p);
};

class ArrayParser: public ParserTemplate<ArrayParser> {
 public:
    struct Exception: impl::Exception {
    };
    using MyJSONType = JSON::Array;
    using Base = ParserTemplate<ArrayParser>;
    using BaseState = Base::State;
    using StatePtr = Base::StatePtr;

    struct State: public BaseState {
        explicit State(IParser& p);
        IObject::Ptr getObject() override;

        IObject::Ptr object;
    };

    static char getFirstSymbolSet();
    static StatePtr getInitState();

 private:
    ISubParser& start(ISubParserState& s, IParser& p);
    ISubParser& parseValue(ISubParserState& s, IParser& p);
    ISubParser& retrieveValue(ISubParserState& s, IParser& p);
    ISubParser& nextMember(ISubParserState& s, IParser& p);
};

class StringParser: public ParserTemplate<StringParser> {
 public:
    struct Exception: impl::Exception {
    };
    using MyJSONType = JSON::String;
    using Base = ParserTemplate<StringParser>;
    using BaseState = Base::State;
    using StatePtr = Base::StatePtr;

    struct State: public BaseState {
        explicit State(IParser& p);
        IObject::Ptr getObject() override;
        IObject::Ptr object;
        IObject::StringType token;
    };

    static char getFirstSymbolSet();
    static StatePtr getInitState();

 private:
    ISubParser& start(ISubParserState& s, IParser& p);
    ISubParser& parseChar(ISubParserState& s, IParser& p);
    ISubParser& parseEscapeChar(ISubParserState& s, IParser& p);
    ISubParser& parseUnicodeEscapeChar(ISubParserState& s, IParser& p);
};

class WSParser: public ParserTemplate<WSParser> {
 public:
    using Base = ParserTemplate<WSParser>;
    using BaseState = Base::State;
    using StatePtr = Base::StatePtr;

    struct State: public BaseState {
        using BaseState::BaseState;
    };

    static const char* getFirstSymbolSet();
    static StatePtr getInitState();

 private:
    ISubParser& dispatch(ISubParserState& state, IParser& p);
};

}  // namespace impl

}  // namespace JSON

#endif  // JSONPARSERFWD_H_
