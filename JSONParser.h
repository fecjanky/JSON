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

// TODO: Add Allocator support for parsers
// TODO: Add Parsing Startegy (Mutable, Immutable)
namespace JSON {

template<typename CharT>
using ObjContainerT = std::vector<typename JSON::Types<CharT>::IObjectPtr>;

namespace impl {

template<typename CharT>
struct IParserT;
template<typename CharT>
struct ISubParserT;
template<typename CharT>
struct ISubParserStateT;
template<typename ParserT, typename CharT>
class ParserTemplate;
template<typename CharT>
class WSParserT;
template<template<typename > class JSONLiteral, typename CharT>
class LiteralParser;
template<typename CharT>
using NullParserT = LiteralParser<NullT,CharT>;
template<typename CharT>
using TrueParserT = LiteralParser<TrueT,CharT>;
template<typename CharT>
using FalseParserT = LiteralParser<FalseT,CharT>;
template<typename CharT>
class ObjectParserT;
template<typename CharT>
class ArrayParserT;
template<typename CharT>
class NumberParserT;
template<typename CharT>
class StringParserT;

template<typename CharT>
using ParserTupleT = std::tuple<
WSParserT<CharT>,
ObjectParserT<CharT> ,
ArrayParserT<CharT> ,
StringParserT<CharT> ,
NumberParserT<CharT> ,
TrueParserT<CharT> ,
FalseParserT<CharT> ,
NullParserT<CharT>
>;

template<typename CharT>
using ISubParserStatePtrT = std::unique_ptr<ISubParserStateT<CharT>>;
template<typename CharT>
using ParserStateStackT = std::stack<ISubParserStatePtrT<CharT>>;

struct Exception: public std::exception {
};
struct InvalidStartingSymbol: public impl::Exception {
};
struct LiteralException: public impl::Exception {
};
struct ParsingIncomplete: public impl::Exception {
};

template<typename JSONLiteral>
const typename JSONLiteral::CharT* GetLiteral()
{
    return JSONLiteral::Literal();
}

template<typename CharT>
inline bool IsWhitespace(CharT c)
{
    for (auto w : JSON::LiteralsT<CharT>::whitespace())
        if (c == w)
            return true;
    return false;
}

template<typename CharT>
struct ISubParserT {
    using IParser = IParserT<CharT>;
    virtual ISubParserT& operator()(IParser& p) = 0;
    virtual ~ISubParserT() = default;
};

template<typename CharT>
struct ISubParserStateT {
    using IParser = IParserT<CharT>;
    using ISubParser = ISubParserT<CharT>;
    using ISubParserState = ISubParserStateT<CharT>;
    using ISubParserStatePtr = ISubParserStatePtrT<CharT>;
    using IObjectPtr = JSON::IObjectPtrT<CharT>;

    virtual ISubParser& getParser(IParser& p) = 0;
    virtual IObjectPtr getObject() = 0;
    virtual ~ISubParserStateT() = default;

    template<typename SubParserImpl>
    static typename SubParserImpl::State& Cast(SubParserImpl& t,
            ISubParserState& s)
    {
        return static_cast<typename SubParserImpl::State&>(s);
    }

    template<
        typename SubParserT,
        typename = std::enable_if_t<std::is_base_of<ISubParserT<CharT>, SubParserT>::value>
        >
    static ISubParserStatePtrT<CharT> Create(IParserT<CharT>& p)
    {
        using State = typename SubParserT::State;
        return ISubParserStatePtr(new State(p));
    };

};

template<typename CharT>
struct IParserT {
    struct Exception: public std::exception {
    };
    virtual CharT getCurrentChar() const noexcept = 0;
    virtual ObjContainerT<CharT>& getObjects() noexcept = 0;
    virtual ParserTupleT<CharT>& getParsers() noexcept = 0;
    virtual ParserStateStackT<CharT>& getStateStack() noexcept = 0;
    virtual ISubParserStatePtrT<CharT>& getLastState() noexcept = 0;
protected:
    virtual ~IParserT() = default;
};

template<typename ParserT, typename CharT>
class ParserTemplate: public ISubParserT<CharT> {
public:
    using StatePtr = ISubParserT<CharT>& (ParserT::*)(ISubParserStateT<CharT>&,IParserT<CharT>&);
    using ISubParser = ISubParserT<CharT>;
    using ISubParserState = ISubParserStateT<CharT>;
    using IParser = IParserT<CharT>;

    class State: public ISubParserStateT<CharT> {
    public:
        explicit State(IParserT<CharT>& p);
        ISubParserT<CharT>& getParser(IParserT<CharT>& p) override;
        IObjectPtrT<CharT> getObject() override;

        StatePtr stateFunc;
    };

    ISubParserT<CharT>& operator()(IParserT<CharT>& p) override;
    ISubParserT<CharT>& nextParser(IParserT<CharT>& p) noexcept;
};

template<typename Parser, typename ParserState, typename CharT>
static ISubParserT<CharT>& StateTransition(Parser& current_parser,
        ParserState& state, IParserT<CharT>& parser)
{
    throw typename Parser::Exception();
    return current_parser;
}

template<typename Parser, typename ParserState, typename CharT,
        typename Predicate, typename Action, typename NextState,
        typename ... Predicates, typename ... Actions, typename ... NextStates>
static ISubParserT<CharT>& StateTransition(
        Parser& current_parser,
        ParserState& s, IParserT<CharT>& parser,
        std::tuple<Predicate, Action, NextState>& p,
        std::tuple<Predicates, Actions, NextStates>&... ps)
{

    auto& state = ISubParserStateT<CharT>::Cast(current_parser, s);
    static_assert(
            std::is_base_of<ISubParserT<CharT>, Parser>::value &&
            std::is_base_of<ISubParserStateT<CharT>, ParserState>::value,
            "State transition not possible");

    if (std::get<0>(p)(parser.getCurrentChar())) {
        state.stateFunc = std::get<2>(p);
        return std::get<1>(p)(current_parser, state, parser);
    } else
        return StateTransition(current_parser, state, parser, ps...);

}

struct NoOp {
    template<typename ParserT, typename StateT, typename CharT>
    ISubParserT<CharT>& operator()(ParserT& current_parser, StateT& s,
            IParserT<CharT>&p)
    {
        return current_parser;
    }
};

struct Store {
    template<typename ParserT, typename StateT, typename CharT>
    ISubParserT<CharT>& operator()(ParserT& current_parser, StateT& s,
            IParserT<CharT>&p)
    {
        s.token.push_back(p.getCurrentChar());
        return current_parser;
    }
};

template<typename NextParserT>
struct Call {
    template<typename ParserT, typename StateT, typename CharT>
    ISubParserT<CharT>& operator()(ParserT& current_parser, StateT& s,
            IParserT<CharT>&p)
    {
        p.getStateStack().push(ISubParserStateT<CharT>::template Create<NextParserT>(p));
        return std::get<NextParserT>(p.getParsers())(p);
    }
};

struct Dispatch {
    template<typename ParserT, typename StateT, typename CharT>
    ISubParserT<CharT>& operator()(ParserT& current_parser, StateT& s,
            IParserT<CharT>&p)
    {
        return DispatchFirstSymbol(p, p.getParsers());
    }
};

template<typename JSONT>
struct Return {
    template<typename ParserT, typename StateT, typename CharT>
    ISubParserT<CharT>& operator()(ParserT& current_parser, StateT& s,
            IParserT<CharT>&p)
    {
        s.object = JSON::Create<JSONT>(std::move(s.token));
        return current_parser.nextParser(p);
    }
};

template<typename JSONT>
struct CallBack {
    template<typename ParserT, typename StateT, typename CharT>
    ISubParserT<CharT>& operator()(ParserT& current_parser, StateT& s,
            IParserT<CharT>&p)
    {
        s.object = JSON::Create<JSONT>(std::move(s.token));
        return current_parser.nextParser(p)(p);
    }
};

struct NoOpReturn {
    template<typename ParserT, typename StateT, typename CharT>
    ISubParserT<CharT>& operator()(ParserT& current_parser, StateT& s,
            IParserT<CharT>&p)
    {
        return current_parser.nextParser(p);
    }
};

template<typename CharT, CharT ... Literal>
class IsLiteral {
public:
    bool operator()(CharT c)
    {
        return eval(c, Literal...);
    }
private:
    bool eval(CharT c)
    {
        return false;
    }
    template<typename Char, typename ... Chars>
    bool eval(CharT c, Char C, Chars ... Cs)
    {
        if (c == C)
            return true;
        else
            return eval(c, Cs...);
    }
};

struct IsDigit{
    template<typename CharT>
    bool operator()(CharT c){
        return std::isdigit(c,std::locale{});
    }
};

class Others {
public:
    template<typename CharT>
    bool operator()(CharT c)
    {
        return true;
    }
};

template<typename CharT>
ISubParserT<CharT>& CheckStartSymbol(IParserT<CharT>& IParser)
{
    throw InvalidStartingSymbol();
}

template<typename CharT, typename SubParserT, typename ... SubParsersT>
std::enable_if_t<
        !std::is_same<SubParserT, WSParserT<CharT>>::value
                && std::is_pointer<
                        decltype(std::declval<SubParserT>().getFirstSymbolSet())>::value,
        ISubParserT<CharT>&> CheckStartSymbol(IParserT<CharT>& parser,
        SubParserT& subparser, SubParsersT&... subparsers)
{
    std::basic_string<CharT> syms = subparser.getFirstSymbolSet();
    if (syms.find(parser.getCurrentChar()) == std::basic_string<CharT>::npos)
        return CheckStartSymbol(parser, subparsers...);
    else {
        // FIXME: fix this
        ISubParserStateT<CharT>::template Create<SubParserT>(parser);
//        parser.getStateStack().push(
//                ISubParserStateT<CharT>::template Create<SubParserT>(parser));
        return subparser(parser);
    }
}

template<typename CharT, typename SubParserT, typename ... SubParsersT>
std::enable_if_t<
        !std::is_same<SubParserT, WSParserT<CharT>>::value
                && !std::is_pointer<
                        decltype(std::declval<SubParserT>().getFirstSymbolSet())>::value,
        ISubParserT<CharT>&> CheckStartSymbol(IParserT<CharT>& parser,
        SubParserT& subparser, SubParsersT&... subparsers)
{
    if (parser.getCurrentChar() != subparser.getFirstSymbolSet())
        return CheckStartSymbol(parser, subparsers...);
    else {
        // FIXME: fix this
//        parser.getStateStack().push(
//                ISubParserStateT<CharT>::template Create<SubParserT>(parser));
        return subparser(parser);
    }
}

// Skip dispatching on WSParser
template<typename CharT, typename SubParserT, typename ... SubParsersT>
std::enable_if_t<std::is_same<SubParserT, WSParserT<CharT>>::value,
        ISubParserT<CharT>&> CheckStartSymbol(IParserT<CharT>& parser,
        SubParserT& subparser, SubParsersT&... subparsers)
{
    return CheckStartSymbol(parser, subparsers...);
}

template<typename CharT, typename ... Parsers>
ISubParserT<CharT>& DispatchFirstSymbol(IParserT<CharT>& p,
        std::tuple<Parsers...>& parsers)
{
    return CheckStartSymbol(p, std::get<Parsers>(parsers)...);
}

template<template<typename > class JSONLiteral, typename CharT>
class LiteralParser: public ParserTemplate<LiteralParser<JSONLiteral, CharT>,
        CharT> {
public:
    using Base = ParserTemplate<LiteralParser<JSONLiteral,CharT>,CharT>;
    using MyJSONType = JSONLiteral<CharT>;
    using BaseStateT = typename Base::State;
    using ISubParser = ISubParserT<CharT>;
    using ISubParserState = ISubParserStateT<CharT>;
    using IParser = IParserT<CharT>;
    using StatePtr = typename Base::StatePtr;

    struct State: public BaseStateT {
        using Base = BaseStateT;
        State(IParserT<CharT>& p);
        IObjectPtrT<CharT> getObject() override;
        size_t current_pos;
    };

    static CharT getFirstSymbolSet();
    static StatePtr getInitState();

private:
    ISubParserT<CharT>& check(ISubParserStateT<CharT>& state,
            IParserT<CharT>& p);
};

template<typename CharT>
class NumberParserT: public ParserTemplate<NumberParserT<CharT>, CharT> {
public:
    struct Exception: impl::Exception {
    };
    using MyJSONType = JSON::NumberT<CharT>;
    using Base = ParserTemplate<NumberParserT<CharT>,CharT>;
    using BaseStateT = typename Base::State;
    using ISubParser = ISubParserT<CharT>;
    using ISubParserState = ISubParserStateT<CharT>;
    using IParser = IParserT<CharT>;
    using StatePtr = typename Base::StatePtr;

    struct State: public BaseStateT {
        using BaseStateT::BaseStateT;

        IObjectPtrT<CharT> getObject() override;

        IObjectPtrT<CharT> object;
        std::basic_string<CharT> token;
    };

    static const CharT* getFirstSymbolSet();
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

template<typename CharT>
class ObjectParserT: public ParserTemplate<ObjectParserT<CharT>, CharT> {
public:
    struct Exception: impl::Exception {
    };
    using MyJSONType = JSON::ObjectT<CharT>;
    using ISubParser = ISubParserT<CharT>;
    using ISubParserState = ISubParserStateT<CharT>;
    using IParser = IParserT<CharT>;
    using Base = ParserTemplate<ObjectParserT<CharT>,CharT>;
    using StatePtr = typename Base::StatePtr;

    struct State: public Base::State {
        explicit State(IParser& p);
        IObjectPtrT<CharT> getObject() override;

        IObjectPtrT<CharT> object;
        std::basic_string<CharT> currentKey;
    };

    static CharT getFirstSymbolSet();
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

template<typename CharT>
class ArrayParserT: public ParserTemplate<ArrayParserT<CharT>, CharT> {
public:

    struct Exception: impl::Exception {
    };
    using MyJSONType = JSON::ArrayT<CharT>;
    using Base = ParserTemplate<ArrayParserT<CharT>,CharT>;
    using BaseStateT = typename Base::State;
    using ISubParser = ISubParserT<CharT>;
    using ISubParserState = ISubParserStateT<CharT>;
    using IParser = IParserT<CharT>;
    using StatePtr = typename Base::StatePtr;

    struct State: public BaseStateT {
        explicit State(IParserT<CharT>& p);
        IObjectPtrT<CharT> getObject() override;

        IObjectPtrT<CharT> object;
    };

    static CharT getFirstSymbolSet();
    static StatePtr getInitState();

private:
    ISubParser& start(ISubParserState& s, IParser& p);
    ISubParser& parseValue(ISubParserState& s, IParser& p);
    ISubParser& retrieveValue(ISubParserState& s, IParser& p);
    ISubParser& nextMember(ISubParserState& s, IParser& p);
};

template<typename CharT>
class StringParserT: public ParserTemplate<StringParserT<CharT>, CharT> {
public:
    struct Exception: impl::Exception {
    };
    using MyJSONType = JSON::StringT<CharT>;
    using Base = ParserTemplate<StringParserT<CharT>,CharT>;
    using BaseStateT = typename Base::State;
    using ISubParser = ISubParserT<CharT>;
    using ISubParserState = ISubParserStateT<CharT>;
    using IParser = IParserT<CharT>;
    using StatePtr = typename Base::StatePtr;

    struct State: public BaseStateT {
        using BaseStateT::BaseStateT;
        IObjectPtrT<CharT> getObject() override;
        IObjectPtrT<CharT> object;
        std::basic_string<CharT> token;
    };

    static CharT getFirstSymbolSet();
    static StatePtr getInitState();

private:
    ISubParser& start(ISubParserState& s, IParser& p);
    ISubParser& parseChar(ISubParserState& s, IParser& p);
    ISubParser& parseEscapeChar(ISubParserState& s, IParser& p);
    ISubParser& parseUnicodeEscapeChar(ISubParserState& s, IParser& p);
};

template<typename CharT>
class WSParserT: public ParserTemplate<WSParserT<CharT>, CharT> {
public:
    using Base = ParserTemplate<WSParserT<CharT>,CharT>;
    using BaseStateT = typename Base::State;
    using ISubParser = ISubParserT<CharT>;
    using ISubParserState = ISubParserStateT<CharT>;
    using IParser = IParserT<CharT>;
    using StatePtr = typename Base::StatePtr;

    struct State: public BaseStateT {
        using BaseStateT::BaseStateT;
    };

    static const CharT* getFirstSymbolSet();
    static StatePtr getInitState();

private:
    ISubParser& emplaceAndDispatch(ISubParserState& state, IParser& p);
};

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

template<typename ParserT, typename CharT>
inline ISubParserT<CharT>& ParserTemplate<ParserT, CharT>::operator()(
        IParser& p)
{
    auto& s = ISubParserState::Cast(*this, *p.getStateStack().top());
    return (static_cast<ParserT*>(this)->*(s.stateFunc))(s, p);
}

template<typename ParserT, typename CharT>
inline ISubParserT<CharT>&
ParserTemplate<ParserT, CharT>::nextParser(IParser& p) noexcept
{
    p.getLastState() = std::move(p.getStateStack().top());
    p.getStateStack().pop();
    return p.getStateStack().top()->getParser(p);
}

template<typename ParserT, typename CharT>
inline IObjectPtrT<CharT> ParserTemplate<ParserT, CharT>::State::getObject()
{
    return nullptr;
}

template<typename ParserT, typename CharT>
inline ISubParserT<CharT>&
ParserTemplate<ParserT, CharT>::State::getParser(IParser& p)
{
    return std::get<ParserT>(p.getParsers());
}

template<typename ParserT, typename CharT>
inline ParserTemplate<ParserT, CharT>::State::State(IParser& p) :
        stateFunc { ParserT::getInitState() }
{
}

template<template<typename > class JSONLiteral, typename CharT>
inline LiteralParser<JSONLiteral, CharT>::State::State(IParser& p) :
        Base(p), current_pos { }
{
}

template<template<typename > class JSONLiteral, typename CharT>
inline IObjectPtrT<CharT> LiteralParser<JSONLiteral, CharT>::State::getObject()
{
    return JSON::Create<JSONLiteral<CharT>>();
}

template<template<typename > class JSONLiteral, typename CharT>
inline ISubParserT<CharT>&
LiteralParser<JSONLiteral, CharT>::check(ISubParserState& s, IParser& p)
{
    auto& state = ISubParserState::Cast(*this, s);
    const size_t literal_size = std::char_traits<CharT>::length(
            GetLiteral<JSONLiteral<CharT>>());
    if (GetLiteral<JSONLiteral<CharT>>()[state.current_pos++]
            != p.getCurrentChar())
        throw LiteralException();
    else if (state.current_pos == literal_size) {
        return this->nextParser(p);
    } else
        return *this;
}

template<template<typename > class JSONLiteral, typename CharT>
inline CharT LiteralParser<JSONLiteral, CharT>::getFirstSymbolSet()
{
    return GetLiteral<JSONLiteral<CharT>>()[0];
}

template<template<typename > class JSONLiteral, typename CharT>
inline typename LiteralParser<JSONLiteral, CharT>::StatePtr LiteralParser<
        JSONLiteral, CharT>::getInitState()
{
    return &LiteralParser::check;
}

template<typename CharT>
inline IObjectPtrT<CharT> NumberParserT<CharT>::State::getObject()
{
    return std::move(object);
}

template<typename CharT>
inline const CharT* NumberParserT<CharT>::getFirstSymbolSet()
{
    return LiteralsT<CharT>::begin_number();
}

template<typename CharT>
inline typename NumberParserT<CharT>::StatePtr NumberParserT<CharT>::getInitState()
{
    return &NumberParserT<CharT>::start;
}

template<typename CharT>
inline ISubParserT<CharT>& NumberParserT<CharT>::start(ISubParserState& state,
        IParser& p)
{
    return StateTransition(*this, state, p,
            std::make_tuple(IsLiteral<CharT, '0'> { }, Store { },
                    &NumberParserT<CharT>::startingZero),
            std::make_tuple(IsDigit{}, Store { },
                    &NumberParserT<CharT>::integerPart),
            std::make_tuple(IsLiteral<CharT, '-'> { }, Store { },
                    &NumberParserT<CharT>::minus));
}

template<typename CharT>
inline ISubParserT<CharT>& NumberParserT<CharT>::minus(ISubParserState& state,
        IParser& p)
{
    return StateTransition(*this, state, p,
            std::make_tuple(IsLiteral<CharT, '0'> { }, Store { },
                    &NumberParserT<CharT>::startingZero),
            std::make_tuple(IsDigit{}, Store { },
                    &NumberParserT<CharT>::integerPart));
}

template<typename CharT>
inline ISubParserT<CharT>& NumberParserT<CharT>::startingZero(
        ISubParserState& state, IParser& p)
{
    return StateTransition(*this, state, p,
            std::make_tuple(IsLiteral<CharT, '.'> { }, Store { },
                    &NumberParserT<CharT>::fractionPartStart),
            std::make_tuple(Others { }, CallBack<MyJSONType> { }, nullptr));
}

template<typename CharT>
inline ISubParserT<CharT>& NumberParserT<CharT>::integerPart(
        ISubParserState& state, IParser& p)
{
    return StateTransition(*this, state, p,
            std::make_tuple(IsDigit{}, Store { },
                    &NumberParserT<CharT>::integerPart),
            std::make_tuple(IsLiteral<CharT, '.'> { }, Store { },
                    &NumberParserT<CharT>::fractionPartStart),
            std::make_tuple(IsLiteral<CharT, 'e', 'E'> { }, Store { },
                    &NumberParserT<CharT>::exponentPartStart),
            std::make_tuple(Others { }, CallBack<MyJSONType> { }, nullptr));
}

template<typename CharT>
inline ISubParserT<CharT>& NumberParserT<CharT>::fractionPartStart(
        ISubParserState& state, IParser& p)
{
    return StateTransition(*this, state, p,
            std::make_tuple(IsDigit{}, Store { },
                    &NumberParserT<CharT>::fractionPart));
}

template<typename CharT>
inline ISubParserT<CharT>& NumberParserT<CharT>::fractionPart(
        ISubParserState& state, IParser& p)
{
    return StateTransition(*this, state, p,
            std::make_tuple(IsDigit{}, Store { },
                    &NumberParserT<CharT>::fractionPart),
            std::make_tuple(IsLiteral<CharT, 'e', 'E'> { }, Store { },
                    &NumberParserT<CharT>::exponentPartStart),
            std::make_tuple(Others { }, CallBack<MyJSONType> { }, nullptr));
}

template<typename CharT>
inline ISubParserT<CharT>& NumberParserT<CharT>::exponentPartStart(
        ISubParserState& state, IParser& p)
{
    return StateTransition(*this, state, p,
            std::make_tuple(IsLiteral<CharT, '-', '+'> { }, Store { },
                    &NumberParserT<CharT>::exponentPartStartSigned),
            std::make_tuple(IsDigit{}, Store { },
                    &NumberParserT<CharT>::exponentPart));
}

template<typename CharT>
inline ISubParserT<CharT>& NumberParserT<CharT>::exponentPartStartSigned(
        ISubParserState& state, IParser& p)
{
    return StateTransition(*this, state, p,
            std::make_tuple(IsDigit{}, Store { },
                    &NumberParserT<CharT>::exponentPart));
}

template<typename CharT>
inline ISubParserT<CharT>& NumberParserT<CharT>::exponentPart(
        ISubParserState& state, IParser& p)
{
    return StateTransition(*this, state, p,
            std::make_tuple(IsDigit{}, Store { },
                    &NumberParserT<CharT>::exponentPart),
            std::make_tuple(Others { }, CallBack<MyJSONType> { }, nullptr));
}

template<typename CharT>
inline ObjectParserT<CharT>::State::State(IParser& p) :
        ParserTemplate<ObjectParserT<CharT>, CharT>::State(p), object {
                JSON::Create<ObjectT<CharT>>() }
{
}

template<typename CharT>
inline IObjectPtrT<CharT> ObjectParserT<CharT>::State::getObject()
{
    return std::move(object);
}

template<typename CharT>
inline CharT ObjectParserT<CharT>::getFirstSymbolSet()
{
    return LiteralsT<CharT>::begin_object;
}

template<typename CharT>
inline typename ObjectParserT<CharT>::StatePtr ObjectParserT<CharT>::getInitState()
{
    return &ObjectParserT<CharT>::start;
}

template<typename CharT>
inline ISubParserT<CharT>& ObjectParserT<CharT>::start(ISubParserState& s,
        IParser& p)
{
    return StateTransition(*this, s, p,
            std::make_tuple(
                    IsLiteral<CharT, LiteralsT<CharT>::begin_object> { },
                    NoOp { }, &ObjectParserT<CharT>::parseKey));
}

template<typename CharT>
inline ISubParserT<CharT>& ObjectParserT<CharT>::parseKey(ISubParserState& s,
        IParser& p)
{
    return StateTransition(*this, s, p,
            std::make_tuple(IsWhitespace<CharT>, NoOp { },
                    &ObjectParserT<CharT>::parseKey),
            std::make_tuple(IsLiteral<CharT, LiteralsT<CharT>::end_object> { },
                    NoOpReturn { }, nullptr),
            std::make_tuple(
                    IsLiteral<CharT, LiteralsT<CharT>::quotation_mark> { },
                    Call<StringParserT<CharT>> { },
                    &ObjectParserT<CharT>::retrieveKey));
}

template<typename CharT>
inline ISubParserT<CharT>& ObjectParserT<CharT>::retrieveKey(ISubParserState& s,
        IParser& p)
{
    auto& state = ISubParserState::Cast(*this, s);
    state.currentKey = std::move(p.getLastState()->getObject()->getValue());
    return parseSeparator(s, p);
}

template<typename CharT>
inline ISubParserT<CharT>& ObjectParserT<CharT>::parseSeparator(
        ISubParserState& s, IParser& p)
{
    return StateTransition(*this, s, p,
            std::make_tuple(IsWhitespace<CharT>, NoOp { },
                    &ObjectParserT<CharT>::parseSeparator),
            std::make_tuple(
                    IsLiteral<CharT, LiteralsT<CharT>::name_separator> { },
                    NoOp { }, &ObjectParserT<CharT>::parseValue));
}

template<typename CharT>
inline ISubParserT<CharT>& ObjectParserT<CharT>::parseValue(ISubParserState& s,
        IParser& p)
{
    return StateTransition(*this, s, p,
            std::make_tuple(IsWhitespace<CharT>, NoOp { },
                    &ObjectParserT<CharT>::parseValue),
            std::make_tuple(Others { }, Dispatch { },
                    &ObjectParserT<CharT>::retrieveValue));
}

template<typename CharT>
inline ISubParserT<CharT>& ObjectParserT<CharT>::retrieveValue(
        ISubParserState& s, IParser& p)
{
    auto& state = ISubParserState::Cast(*this, s);
    auto& currentObject = static_cast<ObjectT<CharT>&>(*state.object);

    currentObject.emplace(std::move(state.currentKey),
            std::move(p.getLastState()->getObject()));

    return nextMember(s, p);
}

template<typename CharT>
ISubParserT<CharT>& ObjectParserT<CharT>::nextMember(ISubParserState& s,
        IParser& p)
{
    return StateTransition(*this, s, p,
            std::make_tuple(IsWhitespace<CharT>, NoOp { },
                    &ObjectParserT<CharT>::nextMember),
            std::make_tuple(IsLiteral<CharT, LiteralsT<CharT>::end_object> { },
                    NoOpReturn { }, nullptr),
            std::make_tuple(
                    IsLiteral<CharT, LiteralsT<CharT>::value_separator> { },
                    NoOp { }, &ObjectParserT<CharT>::parseKey));
}

template<typename CharT>
inline ArrayParserT<CharT>::State::State(IParser& p) :
        ParserTemplate<ArrayParserT<CharT>, CharT>::State(p), object {
                JSON::Create<ArrayT<CharT>>() }
{
}

template<typename CharT>
inline IObjectPtrT<CharT> ArrayParserT<CharT>::State::getObject()
{
    return std::move(object);
}

template<typename CharT>
inline CharT ArrayParserT<CharT>::getFirstSymbolSet()
{
    return LiteralsT<CharT>::begin_array;
}

template<typename CharT>
inline typename ArrayParserT<CharT>::StatePtr ArrayParserT<CharT>::getInitState()
{
    return &ArrayParserT<CharT>::start;
}

template<typename CharT>
inline ISubParserT<CharT>& ArrayParserT<CharT>::start(ISubParserState& s,
        IParser& p)
{
    return StateTransition(*this, s, p,
            std::make_tuple(IsLiteral<CharT, LiteralsT<CharT>::begin_array> { },
                    NoOp { }, &ArrayParserT<CharT>::parseValue));
}

template<typename CharT>
inline ISubParserT<CharT>& ArrayParserT<CharT>::parseValue(ISubParserState& s,
        IParser& p)
{
    return StateTransition(*this, s, p,
            std::make_tuple(IsWhitespace<CharT>, NoOp { },
                    &ArrayParserT<CharT>::parseValue),
            std::make_tuple(Others { }, Dispatch { },
                    &ArrayParserT<CharT>::retrieveValue));
}

template<typename CharT>
inline ISubParserT<CharT>& ArrayParserT<CharT>::retrieveValue(
        ISubParserState& s, IParser& p)
{
    auto& state = ISubParserState::Cast(*this, s);
    auto& currentObject = static_cast<ArrayT<CharT>&>(*state.object);

    currentObject.emplace(std::move(p.getLastState()->getObject()));

    return nextMember(s, p);
}

template<typename CharT>
ISubParserT<CharT>& ArrayParserT<CharT>::nextMember(ISubParserState& s,
        IParser& p)
{
    return StateTransition(*this, s, p,
            std::make_tuple(IsWhitespace<CharT>, NoOp { },
                    &ArrayParserT<CharT>::nextMember),
            std::make_tuple(IsLiteral<CharT, LiteralsT<CharT>::end_array> { },
                    NoOpReturn { }, nullptr),
            std::make_tuple(
                    IsLiteral<CharT, LiteralsT<CharT>::value_separator> { },
                    NoOp { }, &ArrayParserT<CharT>::parseValue));
}

template<typename CharT>
inline const CharT* WSParserT<CharT>::getFirstSymbolSet()
{
    return LiteralsT<CharT>::whitespace_string();
}

template<typename CharT>
inline typename WSParserT<CharT>::StatePtr WSParserT<CharT>::getInitState()
{
    return &WSParserT<CharT>::emplaceAndDispatch;
}

template<typename CharT>
inline ISubParserT<CharT>& WSParserT<CharT>::emplaceAndDispatch(
        ISubParserState& state, IParser& p)
{
    auto& s = static_cast<State&>(state);

    if (p.getLastState()) {
        auto last_state = std::move(p.getLastState());
        p.getObjects().emplace_back(std::move(last_state->getObject()));
    }

    if (IsWhitespace<CharT>(p.getCurrentChar()))
        return *this;
    else {
        return DispatchFirstSymbol(p, p.getParsers());
    }
}

template<typename CharT>
inline IObjectPtrT<CharT> StringParserT<CharT>::State::getObject()
{
    return std::move(object);
}

template<typename CharT>
inline CharT StringParserT<CharT>::getFirstSymbolSet()
{
    return LiteralsT<CharT>::quotation_mark;
}

template<typename CharT>
inline typename StringParserT<CharT>::StatePtr StringParserT<CharT>::getInitState()
{
    return &StringParserT<CharT>::start;
}

template<typename CharT>
inline ISubParserT<CharT>& StringParserT<CharT>::start(ISubParserState& s,
        IParser& p)
{
    return StateTransition(*this, s, p,
            std::make_tuple(
                    IsLiteral<CharT, LiteralsT<CharT>::quotation_mark> { },
                    NoOp { }, &StringParserT<CharT>::parseChar));
}

template<typename CharT>
inline ISubParserT<CharT>& StringParserT<CharT>::parseChar(ISubParserState& s,
        IParser& p)
{
    return StateTransition(*this, s, p,
            std::make_tuple(
                    IsLiteral<CharT, LiteralsT<CharT>::quotation_mark> { },
                    Return<MyJSONType> { }, nullptr),
            std::make_tuple(
                    IsLiteral<CharT, LiteralsT<CharT>::string_escape> { },
                    NoOp { }, &StringParserT<CharT>::parseEscapeChar),
            std::make_tuple([](CharT c) -> bool {
                return c == 0x20 || c == 0x21 || (c >= 0x23 && c <= 0x5b) ||
                (c >= 0x5D && c <= 0x10FFFF);
            }, Store { }, &StringParserT<CharT>::parseChar));
}

template<typename CharT>
inline ISubParserT<CharT>& StringParserT<CharT>::parseEscapeChar(
        ISubParserState& s, IParser& p)
{
    return StateTransition(*this, s, p,
            std::make_tuple(
                    IsLiteral<CharT,
                            JSON::LiteralsT<CharT>::string_unicode_escape> { },
                    NoOp { }, &StringParserT<CharT>::parseUnicodeEscapeChar),
            std::make_tuple([](CharT c) -> bool {
                for(auto e : JSON::LiteralsT<CharT>::string_escapes())
                if(e == c)return true;
                return false;
            }, Store { }, &StringParserT<CharT>::parseChar));
}

template<typename CharT>
inline ISubParserT<CharT>& StringParserT<CharT>::parseUnicodeEscapeChar(
        ISubParserState& s, IParser& p)
{
    // throws exception!
    // TODO: implement parsing of Unicode escapes
    return StateTransition(*this, s, p);
}

}

template<typename CharT>
class Parser: public impl::IParserT<CharT> {
public:
    using ObjContainer = ObjContainerT<CharT>;

    Parser() :
            current_char { }
    {
        using ISubParserState = impl::ISubParserStateT<CharT>;
        stateStack.push(ISubParserState::template Create<impl::WSParserT<CharT>>(*this));
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

    void operator()(CharT c)
    {
        current_char = c;
        stateStack.top()->getParser(*this)(*this);
    }

    Parser(const Parser&) = delete;
    Parser& operator=(const Parser&) = delete;
    Parser(Parser&&) = delete;
    Parser& operator=(Parser&&) = delete;

private:
    using ParserTuple = impl::ParserTupleT<CharT>;
    using ParserStateStack = impl::ParserStateStackT<CharT>;
    using ISubParserStatePtr = impl::ISubParserStatePtrT<CharT>;

    CharT getCurrentChar() const noexcept override
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

    CharT current_char;
    ObjContainer objects;
    ParserTuple parsers;
    ParserStateStack stateStack;
    ISubParserStatePtr lastState;

};

template<typename ForwardIterator>
ObjContainerT<std::decay_t<decltype(*(std::declval<ForwardIterator>()))>> parse(
        ForwardIterator start, ForwardIterator end)
{

    Parser<std::decay_t<decltype(*(std::declval<ForwardIterator>()))>> p;

    for (; start != end; ++start) {
        p(*start);
    }

    return p.retrieveObjects();
}
;

}

#endif //JSONPARSER_H_INCLUDED__
