#ifndef JSONPARSER_H_INCLUDED__
#define JSONPARSER_H_INCLUDED__

#include <type_traits>
#include <utility>
#include <string>
#include <tuple>
#include <vector>
#include <stack>
#include <exception>
#include <locale>

#include "JSON.h"
#include "JSONObjects.h"

// TODO: Add Allocator support for parsers
// TODO: Add Parsing Startegy (Mutable, Immutable)
// TODO: replace literalas in predicate templates

namespace JSON {


using ObjContainer = std::vector<IObjectPtr>;

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

template<typename JSONLiteral>
const char* GetLiteral()
{
    return JSONLiteral::Literal();
}

inline bool IsWhitespace(char c)
{
    for (auto w : JSON::Literals::whitespace())
        if (std::char_traits<char>::eq(c,w))
            return true;
    return false;
}

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
            ISubParserState& s)
    {
        return static_cast<typename SubParserImpl::State&>(s);
    }

    template<typename SubParserT, typename = std::enable_if_t<
            std::is_base_of<ISubParser, SubParserT>::value> >
    static ISubParserStatePtr Create(IParser& p)
    {
        using State = typename SubParserT::State;
        return ISubParserStatePtr( new State(p) );
    }
    ;

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
    using StatePtr = ISubParser& (ParserT::*)(ISubParserState&,IParser&);

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

template<typename Parser, typename ParserState>
static ISubParser& StateTransition(Parser& current_parser,
        ParserState& state, IParser& parser)
{
    throw typename Parser::Exception();
}

template<typename Parser, typename ParserState,
        typename Predicate, typename Action, typename NextState,
        typename ... Predicates, typename ... Actions, typename ... NextStates>
static ISubParser& StateTransition(Parser& current_parser,
        ParserState& s, IParser& parser,
        std::tuple<Predicate, Action, NextState>&& p,
        std::tuple<Predicates, Actions, NextStates>&&... ps)
{
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
    ISubParser& operator()(ParserT& current_parser, StateT& s,
            IParser&p)
    {
        return current_parser;
    }
};

struct Store {
    template<typename ParserT, typename StateT>
    ISubParser& operator()(ParserT& current_parser, StateT& s,
            IParser&p)
    {
        static_assert(std::is_same<std::string, decltype(s.token)>::value,
            "State member token is not type of string");
        s.token.push_back(p.getCurrentChar());
        return current_parser;
    }
};

template<typename NextParserT>
struct Call {
    template<typename ParserT, typename StateT>
    ISubParser& operator()(ParserT& current_parser, StateT& s,
            IParser&p)
    {
        p.getStateStack().push(
                ISubParserState::template Create<NextParserT>(p));
        return std::get<NextParserT>(p.getParsers())(p);
    }
};

struct Dispatch {
    template<typename ParserT, typename StateT>
    ISubParser& operator()(ParserT& current_parser, StateT& s,
            IParser&p)
    {
        return DispatchFirstSymbol(p, p.getParsers());
    }
};

template<typename JSONT>
struct Return {
    template<typename ParserT, typename StateT>
    ISubParser& operator()(ParserT& current_parser, StateT& s,
            IParser&p)
    {
        static_assert(std::is_same<JSON::IObjectPtr, 
            decltype(s.object)>::value,
            "State member object is not type of Object pointer");
        s.object = JSON::Create<JSONT>(std::move(s.token));
        return current_parser.nextParser(p);
    }
};

template<typename JSONT>
struct CallBack {
    template<typename ParserT, typename StateT>
    ISubParser& operator()(ParserT& current_parser, StateT& s,
            IParser&p)
    {
        static_assert(std::is_same<JSON::IObjectPtr, 
            decltype(s.object)>::value,
            "State member object is not type of Object pointer");
        s.object = JSON::Create<JSONT>(std::move(s.token));
        return current_parser.nextParser(p)(p);
    }
};

struct NoOpReturn {
    template<typename ParserT, typename StateT>
    ISubParser& operator()(ParserT& current_parser, StateT& s,
            IParser&p)
    {
        return current_parser.nextParser(p);
    }
};

template<char... Literal>
class IsLiteral {
public:
    bool operator()(char c)
    {
        return eval(c, Literal...);
    }
private:
    bool eval(char c)
    {
        return false;
    }
    template<typename Char, typename ... Chars>
    bool eval(char c, Char C, Chars ... Cs)
    {
        if (std::char_traits<char>::eq(c,C))
            return true;
        else
            return eval(c, Cs...);
    }
};

struct IsStringChar {
    
    bool operator()(char c) {
        using ct = std::char_traits<char>;
        // FIXME: find correct logic for checking string chars
        return  ct::eq(c, ct::to_char_type(0x20)) || 
                ct::eq(c, ct::to_char_type(0x21)) || 
                gt_eq(c, ct::to_char_type(0x23)) && 
                lt_eq(c, ct::to_char_type(0x5b)) || 
                gt_eq(c, ct::to_char_type(0x5D));
    }
private:
    
    static bool lt_eq(char lhs,char rhs) {
        using ct = std::char_traits<char>;
        return ct::lt(lhs, rhs) || ct::eq(lhs,rhs);
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
    
    bool operator()(char c)
    {
        return std::isdigit(c, std::locale { });
    }
};

class Others {
public:
    
    bool operator()(char c)
    {
        return true;
    }
};

struct CheckSymbol {
    
    bool operator()(char c, const char* s)
    {
        using ct = std::char_traits<char>;
        return ct::find(s, ct::length(s), c) != nullptr;
    }
    
    bool operator()(char c, char s)
    {
        return std::char_traits<char>::eq(c, s);
    }
};

ISubParser& CheckStartSymbol(IParser& IParser)
{
    throw InvalidStartingSymbol();
}

template< typename SubParserT, typename ... SubParsersT>
ISubParser& CheckStartSymbol(IParser& parser,
        SubParserT& subparser, SubParsersT&... subparsers)
{
    if (!std::is_same<WSParser, std::decay_t<SubParserT>> { }
            && CheckSymbol { }(parser.getCurrentChar(),
                    subparser.getFirstSymbolSet())) {
        parser.getStateStack().push(
                ISubParserState::Create<SubParserT>(parser));
        return subparser(parser);
    } else
        return CheckStartSymbol(parser, subparsers...);
}

template< typename ... Parsers>
ISubParser& DispatchFirstSymbol(IParser& p,
        std::tuple<Parsers...>& parsers)
{
    return CheckStartSymbol(p, std::get<Parsers>(parsers)...);
}

template<class JSONLiteral>
class LiteralParser: public ParserTemplate<LiteralParser<JSONLiteral>> {
public:
    using Base = ParserTemplate<LiteralParser<JSONLiteral>>;
    using MyJSONType = JSONLiteral;
    using BaseState = Base::State;
    using StatePtr = Base::StatePtr;

    struct State: public BaseState {
        using Base = BaseState;
        State(IParser& p);
        IObjectPtr getObject() override;
        size_t current_pos;
    };

    static char getFirstSymbolSet();
    static StatePtr getInitState();

private:
    ISubParser& check(ISubParserState& state,IParser& p);
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

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

template<typename ParserT>
inline ISubParser& ParserTemplate<ParserT>::operator()(
        IParser& p)
{
    auto& s = ISubParserState::Cast(*this, *p.getStateStack().top());
    return (static_cast<ParserT*>(this)->*(s.stateFunc))(s, p);
}

template<typename ParserT>
inline ISubParser&
ParserTemplate<ParserT>::nextParser(IParser& p) noexcept
{
    p.getLastState() = std::move(p.getStateStack().top());
    p.getStateStack().pop();
    // If last parser in stack
    // add last object to output collection
    if(p.getStateStack().size() == 1){
        p.getObjects().emplace_back(
            std::move(p.getLastState()->getObject()));
    }
    return p.getStateStack().top()->getParser(p);
}

template<typename ParserT>
inline IObjectPtr ParserTemplate<ParserT>::State::getObject()
{
    return nullptr;
}

template<typename ParserT>
inline ISubParser&
ParserTemplate<ParserT>::State::getParser(IParser& p)
{
    return std::get<ParserT>(p.getParsers());
}

template<typename ParserT>
inline ParserTemplate<ParserT>::State::State(IParser& p) :
        stateFunc { ParserT::getInitState() }
{
}

template<class JSONLiteral>
inline LiteralParser<JSONLiteral>::State::State(IParser& p) :
        Base(p), current_pos { }
{
}

template<class JSONLiteral>
inline IObjectPtr LiteralParser<JSONLiteral>::State::getObject()
{
    return JSON::Create<JSONLiteral>();
}

template<class JSONLiteral>
inline ISubParser&
LiteralParser<JSONLiteral>::check(ISubParserState& s, IParser& p)
{
    auto& state = ISubParserState::Cast(*this, s);
    using ct = std::char_traits<char>;
    const size_t literal_size = ct::length(GetLiteral<JSONLiteral>());
    if (!ct::eq(GetLiteral<JSONLiteral>()[state.current_pos++],
        p.getCurrentChar()))
        throw LiteralException();
    else if (state.current_pos == literal_size) {
        return this->nextParser(p);
    } else
        return *this;
}

template<class JSONLiteral>
inline char LiteralParser<JSONLiteral>::getFirstSymbolSet()
{
    return GetLiteral<JSONLiteral>()[0];
}

template<class JSONLiteral>
inline typename LiteralParser<JSONLiteral>::StatePtr LiteralParser<JSONLiteral>::getInitState()
{
    return &LiteralParser::check;
}


inline IObjectPtr NumberParser::State::getObject()
{
    return std::move(object);
}


inline const char* NumberParser::getFirstSymbolSet()
{
    return Literals::begin_number();
}


inline NumberParser::StatePtr NumberParser::getInitState()
{
    return &NumberParser::start;
}


inline ISubParser& NumberParser::start(ISubParserState& state,
        IParser& p)
{
    using L = Literals;
    return StateTransition(*this, state, p,
            std::make_tuple(IsLiteral<L::zero> { }, Store { },
                    &NumberParser::startingZero),
            std::make_tuple(IsDigit { }, Store { },
                    &NumberParser::integerPart),
            std::make_tuple(IsLiteral<L::minus> { }, Store { },
                    &NumberParser::minus));
}


inline ISubParser& NumberParser::minus(ISubParserState& state,
        IParser& p)
{
    using L = Literals;
    return StateTransition(*this, state, p,
            std::make_tuple(IsLiteral<L::zero> { }, Store { },
                    &NumberParser::startingZero),
            std::make_tuple(IsDigit { }, Store { },
                    &NumberParser::integerPart));
}


inline ISubParser& NumberParser::startingZero(
        ISubParserState& state, IParser& p)
{
    using L = Literals;
    return StateTransition(*this, state, p,
            std::make_tuple(IsLiteral<L::decimal_point> { }, Store { },
                    &NumberParser::fractionPartStart),
            std::make_tuple(Others { }, CallBack<MyJSONType> { }, nullptr));
}


inline ISubParser& NumberParser::integerPart(
        ISubParserState& state, IParser& p)
{
    using L = Literals;
    return StateTransition(*this, state, p,
            std::make_tuple(IsDigit { }, Store { },
                    &NumberParser::integerPart),
            std::make_tuple(IsLiteral<L::decimal_point> { }, Store { },
                    &NumberParser::fractionPartStart),
            std::make_tuple(IsLiteral<L::exponent_upper, L::exponent_lower> { }, 
                    Store { },&NumberParser::exponentPartStart),
            std::make_tuple(Others { }, CallBack<MyJSONType> { }, nullptr));
}


inline ISubParser& NumberParser::fractionPartStart(
        ISubParserState& state, IParser& p)
{
    return StateTransition(*this, state, p,
            std::make_tuple(IsDigit { }, Store { },
                    &NumberParser::fractionPart));
}


inline ISubParser& NumberParser::fractionPart(
        ISubParserState& state, IParser& p)
{
    using L = Literals;
    return StateTransition(*this, state, p,
            std::make_tuple(IsDigit { }, Store { },
                    &NumberParser::fractionPart),
            std::make_tuple(IsLiteral<L::exponent_upper, L::exponent_lower> { }, Store { },
                    &NumberParser::exponentPartStart),
            std::make_tuple(Others { }, CallBack<MyJSONType> { }, nullptr));
}


inline ISubParser& NumberParser::exponentPartStart(
        ISubParserState& state, IParser& p)
{
    using L = Literals;
    return StateTransition(*this, state, p,
            std::make_tuple(IsLiteral<L::minus, L::plus> { }, Store { },
                    &NumberParser::exponentPartStartSigned),
            std::make_tuple(IsDigit { }, Store { },
                    &NumberParser::exponentPart));
}


inline ISubParser& NumberParser::exponentPartStartSigned(
        ISubParserState& state, IParser& p)
{
    return StateTransition(*this, state, p,
            std::make_tuple(IsDigit { }, Store { },
                    &NumberParser::exponentPart));
}


inline ISubParser& NumberParser::exponentPart(
        ISubParserState& state, IParser& p)
{
    return StateTransition(*this, state, p,
            std::make_tuple(IsDigit { }, Store { },
                    &NumberParser::exponentPart),
            std::make_tuple(Others { }, CallBack<MyJSONType> { }, nullptr));
}


inline ObjectParser::State::State(IParser& p) :
        ParserTemplate<ObjectParser>::State(p), 
        object (JSON::Create<JSON::Object>())
{
}


inline IObjectPtr ObjectParser::State::getObject()
{
    return std::move(object);
}


inline char ObjectParser::getFirstSymbolSet()
{
    return Literals::begin_object;
}


inline ObjectParser::StatePtr ObjectParser::getInitState()
{
    return &ObjectParser::start;
}


inline ISubParser& ObjectParser::start(ISubParserState& s,
        IParser& p)
{
    return StateTransition(*this, s, p,
            std::make_tuple(
                    IsLiteral<Literals::begin_object> { },
                    NoOp { }, &ObjectParser::parseKey));
}


inline ISubParser& ObjectParser::parseKey(ISubParserState& s,
        IParser& p)
{
    return StateTransition(*this, s, p,
            std::make_tuple(IsWhitespace, NoOp { },
                    &ObjectParser::parseKey),
            std::make_tuple(IsLiteral<Literals::end_object> { },
                    NoOpReturn { }, nullptr),
            std::make_tuple(
                    IsLiteral<Literals::quotation_mark> { },
                    Call<StringParser> { },
                    &ObjectParser::retrieveKey));
}


inline ISubParser& ObjectParser::retrieveKey(ISubParserState& s,
        IParser& p)
{
    auto& state = ISubParserState::Cast(*this, s);
    state.currentKey = std::move(p.getLastState()->getObject()->getValue());
    return parseSeparator(s, p);
}


inline ISubParser& ObjectParser::parseSeparator(
        ISubParserState& s, IParser& p)
{
    return StateTransition(*this, s, p,
            std::make_tuple(IsWhitespace, NoOp { },
                    &ObjectParser::parseSeparator),
            std::make_tuple(
                    IsLiteral<Literals::name_separator> { },
                    NoOp { }, &ObjectParser::parseValue));
}


inline ISubParser& ObjectParser::parseValue(ISubParserState& s,
        IParser& p)
{
    return StateTransition(*this, s, p,
            std::make_tuple(IsWhitespace, NoOp { },
                    &ObjectParser::parseValue),
            std::make_tuple(Others { }, Dispatch { },
                    &ObjectParser::retrieveValue));
}


inline ISubParser& ObjectParser::retrieveValue(
        ISubParserState& s, IParser& p)
{
    auto& state = ISubParserState::Cast(*this, s);
    auto& currentObject = static_cast<Object&>(*state.object);

    currentObject.emplace(std::move(state.currentKey),
            std::move(p.getLastState()->getObject()));

    return nextMember(s, p);
}


ISubParser& ObjectParser::nextMember(ISubParserState& s,
        IParser& p)
{
    return StateTransition(*this, s, p,
            std::make_tuple(IsWhitespace, NoOp { },
                    &ObjectParser::nextMember),
            std::make_tuple(IsLiteral<Literals::end_object> { },
                    NoOpReturn { }, nullptr),
            std::make_tuple(
                    IsLiteral<Literals::value_separator> { },
                    NoOp { }, &ObjectParser::parseKey));
}


inline ArrayParser::State::State(IParser& p) :
        ParserTemplate<ArrayParser>::State(p), object {
                JSON::Create<JSON::Array>() }
{
}


inline IObjectPtr ArrayParser::State::getObject()
{
    return std::move(object);
}


inline char ArrayParser::getFirstSymbolSet()
{
    return Literals::begin_array;
}


inline ArrayParser::StatePtr ArrayParser::getInitState()
{
    return &ArrayParser::start;
}


inline ISubParser& ArrayParser::start(ISubParserState& s,
        IParser& p)
{
    return StateTransition(*this, s, p,
            std::make_tuple(IsLiteral<Literals::begin_array> { },
                    NoOp { }, &ArrayParser::parseValue));
}


inline ISubParser& ArrayParser::parseValue(ISubParserState& s,
        IParser& p)
{
    return StateTransition(*this, s, p,
            std::make_tuple(IsWhitespace, NoOp { },
                    &ArrayParser::parseValue),
            std::make_tuple(Others { }, Dispatch { },
                    &ArrayParser::retrieveValue));
}


inline ISubParser& ArrayParser::retrieveValue(
        ISubParserState& s, IParser& p)
{
    auto& state = ISubParserState::Cast(*this, s);
    auto& currentObject = static_cast<Array&>(*state.object);

    currentObject.emplace(std::move(p.getLastState()->getObject()));

    return nextMember(s, p);
}


ISubParser& ArrayParser::nextMember(ISubParserState& s,
        IParser& p)
{
    return StateTransition(*this, s, p,
            std::make_tuple(IsWhitespace, NoOp { },
                    &ArrayParser::nextMember),
            std::make_tuple(IsLiteral<Literals::end_array> { },
                    NoOpReturn { }, nullptr),
            std::make_tuple(
                    IsLiteral<Literals::value_separator> { },
                    NoOp { }, &ArrayParser::parseValue));
}


inline const char* WSParser::getFirstSymbolSet()
{
    return Literals::whitespace_string();
}


inline WSParser::StatePtr WSParser::getInitState()
{
    return &WSParser::dispatch;
}


inline ISubParser& WSParser::dispatch(
        ISubParserState& state, IParser& p)
{
    if (IsWhitespace(p.getCurrentChar()))
        return *this;
    else {
        return DispatchFirstSymbol(p, p.getParsers());
    }
}


inline IObjectPtr StringParser::State::getObject()
{
    return std::move(object);
}


inline char StringParser::getFirstSymbolSet()
{
    return Literals::quotation_mark;
}


inline StringParser::StatePtr StringParser::getInitState()
{
    return &StringParser::start;
}


inline ISubParser& StringParser::start(ISubParserState& s,
        IParser& p)
{
    return StateTransition(*this, s, p,
            std::make_tuple(
                    IsLiteral<Literals::quotation_mark> { },
                    NoOp { }, &StringParser::parseChar));
}


inline ISubParser& StringParser::parseChar(ISubParserState& s,
        IParser& p)
{
    return StateTransition(*this, s, p,
            std::make_tuple(
                    IsLiteral<Literals::quotation_mark> { },
                    Return<MyJSONType> { }, nullptr),
            std::make_tuple(
                    IsLiteral<Literals::string_escape> { },
                    NoOp { }, &StringParser::parseEscapeChar),
            std::make_tuple(IsStringChar{}, Store{}, &StringParser::parseChar));
}


inline ISubParser& StringParser::parseEscapeChar(
        ISubParserState& s, IParser& p)
{
    return StateTransition(*this, s, p,
            std::make_tuple(
                    IsLiteral<JSON::Literals::string_unicode_escape> { },
                    NoOp { }, &StringParser::parseUnicodeEscapeChar),
            std::make_tuple([](char c) -> bool {
                for(auto e : JSON::Literals::string_escapes())
                    if(std::char_traits<char>::eq(e,c))return true;
                return false;
            }, Store { }, &StringParser::parseChar));
}


inline ISubParser& StringParser::parseUnicodeEscapeChar(
        ISubParserState& s, IParser& p)
{
    // throws exception!
    // TODO: implement parsing of Unicode escapes
    return StateTransition(*this, s, p);
}

} // namespace JSON::impl


class Parser: public impl::IParser {
public:

    Parser() :
            current_char { }
    {
        stateStack.push(
                impl::ISubParserState::Create<impl::WSParser>(*this));
    }

    ObjContainer retrieveObjects()
    {
        if (!isRetrievable())
            throw impl::ParsingIncomplete();
        return std::move(objects);
    }

    bool isRetrievable() const noexcept
    {
        return stateStack.size() == 1;
    }

    operator bool() const noexcept
    {
        return isRetrievable();
    }

    void operator()(char c)
    {
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

    char getCurrentChar() const noexcept override
    {
        return current_char;
    }

    ObjContainer& getObjects() noexcept override
    {
        return objects;
    }

    ParserTuple& getParsers() noexcept override
    {
        return parsers;
    }

    ParserStateStack& getStateStack() noexcept override
    {
        return stateStack;
    }

    ISubParserStatePtr& getLastState() noexcept override
    {
        return lastState;
    }

    char current_char;
    ObjContainer objects;
    ParserTuple parsers;
    ParserStateStack stateStack;
    ISubParserStatePtr lastState;

};

template<typename ForwardIterator>
ObjContainer parse(
        ForwardIterator start, ForwardIterator end)
{
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

} // namespace JSON

#endif //JSONPARSER_H_INCLUDED__
