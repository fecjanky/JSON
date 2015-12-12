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

#include "JSONFwd.h"
#include "JSON.h"

namespace JSON {

using ObjContainer = std::vector<JSON::IObjectPtr>;

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

using ISubParserStatePtr = std::unique_ptr<ISubParserState>;
using ParserStateStack = std::stack<ISubParserStatePtr>;

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
    virtual IObjectPtr getObject() = 0;
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
        return ISubParserStatePtr(new State(p));
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
        IObjectPtr getObject() override;

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
        IObjectPtr getObject() override;
        size_t current_pos;
    };

    static char getFirstSymbolSet();
    static StatePtr getInitState();

 private:
    ISubParser& check(ISubParserState& state, IParser& p);
};

class NumberParser: public ParserTemplate<NumberParser> {
 public:
    struct Exception: impl::Exception {
    };
    using MyJSONType = JSON::Number;
    using Base = ParserTemplate<NumberParser>;
    using BaseState = Base::State;
    using StatePtr = Base::StatePtr;

    struct State: public BaseState {
        using BaseState::BaseState;

        IObjectPtr getObject() override;

        IObjectPtr object;
        std::string token;
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
        IObjectPtr getObject() override;

        IObjectPtr object;
        std::string currentKey;
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
        IObjectPtr getObject() override;

        IObjectPtr object;
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
        using BaseState::BaseState;
        IObjectPtr getObject() override;
        IObjectPtr object;
        std::string token;
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