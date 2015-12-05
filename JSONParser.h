#ifndef JSONPARSER_H_INCLUDED__
#define JSONPARSER_H_INCLUDED__

#include <type_traits>
#include <utility>
#include <string>
#include <tuple>
#include <vector>
#include <stack>
#include <exception>
#include <cctype>

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
        template<typename ParserT>
        class ParserTemplate;
        template<typename CharT>
        class WSParserT;
        template<typename JSONLiteral>
        class LiteralParser;
        template<typename CharT>
        using NullParserT = LiteralParser<NullT<CharT>>;
        template<typename CharT>
        using TrueParserT = LiteralParser<TrueT<CharT>>;
        template<typename CharT>
        using FalseParserT = LiteralParser<FalseT<CharT>>;
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

        template<typename CharT_>
        struct ParserTypes {
            using CharT = CharT_;
            using StringT = std::basic_string<CharT>;
            using IParser = IParserT<CharT>;
            using ISubParser = ISubParserT<CharT>;
            using ISubParserState = ISubParserStateT<CharT>;
            using WSParser = WSParserT<CharT>;
            using NullParser = NullParserT<CharT>;
            using TrueParser = TrueParserT<CharT>;
            using FalseParser = FalseParserT<CharT>;
            using ObjectParser = ObjectParserT<CharT>;
            using ArrayParser = ArrayParserT<CharT>;
            using NumberParser = NumberParserT<CharT>;
            using StringParser = StringParserT<CharT>;
            using ParserTuple = ParserTupleT<CharT>;
            using ISubParserStatePtr = ISubParserStatePtrT<CharT>;
            using ParserStateStack = ParserStateStackT<CharT>;
            using ObjContainer = ObjContainerT<CharT>;
            using IObjectPtr = IObjectPtrT<CharT>;
        };


        struct Exception : public std::exception {};
        struct InvalidStartingSymbol : public impl::Exception {};
        struct LiteralException : public impl::Exception {};
        struct ParsingIncomplete : public impl::Exception {};

        template<typename JSONLiteral>
        const typename JSONLiteral::CharT* GetLiteral() {
            return JSONLiteral::Literal();
        }
        
        template<typename CharT>
        inline bool IsWhitespace(CharT c) {
            for (auto w : JSON::LiteralsT<CharT>::whitespace()) if (c == w) return true;
            return false;
        }

        template<typename CharT>
        struct ISubParserT : public ParserTypes<CharT> {
            virtual ISubParserT& operator()(IParser& p) = 0;
            virtual ~ISubParserT() = default;
        };

        template<typename CharT>
        struct ISubParserStateT : public ParserTypes<CharT> {
            
            virtual ISubParser& getParser(IParser& p) = 0;
            virtual IObjectPtr getObject() = 0;
            virtual ~ISubParserStateT() = default;
            
            template<typename SubParserImpl>
            static typename SubParserImpl::State& Cast(
                SubParserImpl& t,ISubParserState& s) {
                return static_cast<typename SubParserImpl::State&>(s);
            }

            template<typename SubParserT>
            static
            std::enable_if_t<std::is_base_of<ISubParser, SubParserT>::value
                , ISubParserStatePtr
            > Create(IParser& p) {
                return ISubParserStatePtr(new typename SubParserT::State(p));
            }

        };

        template<typename CharT>
        struct IParserT : public ParserTypes<CharT>{
            struct Exception : public std::exception {};
            virtual CharT getCurrentChar() const noexcept = 0;
            virtual ObjContainer& getObjects() noexcept = 0;
            virtual ParserTuple& getParsers() noexcept = 0;
            virtual ParserStateStack& getStateStack() noexcept = 0;
            virtual ISubParserStatePtr& getLastState() noexcept = 0;
        protected:
            virtual ~IParserT() = default;
        };

        template<typename T>
        struct ExtractCharT{
        private:
            template<template <typename> class L>
            struct IsLiteralParser {
                static constexpr bool value = false;
            };

            template<>
            struct IsLiteralParser<LiteralParser> {
                static constexpr bool value = true;
            };

            template<typename L>
            struct LiteralParserChar;
            template<template <typename> class L,typename C>
            struct LiteralParserChar<LiteralParser<L<C>>> {
                using Char = C;
            };

            template<
                template <typename...> class P, 
                typename CharT,
                typename = std::enable_if_t<!IsLiteralParser<P>::value>
            >
            static CharT test(const P<CharT>&);

            template<
                template <typename...> class P,
                typename CharT,
                typename = std::enable_if_t<IsLiteralParser<P>::value>
            >
            static typename LiteralParserChar<P<CharT>>::Char test(const P<CharT>&);

        public:
            using Type = decltype(test(std::declval<T>()));

        };

        template<typename ParserT>
        class ParserTemplate : 
            public ISubParserT<typename ExtractCharT<ParserT>::Type> {
        public:
            using StatePtr = ISubParser& (ParserT::*)(ISubParserState&,IParser&);
            
            static CharT test() {
                return CharT{};
            }

            class State : public ISubParserState {
            public:
                explicit State(IParser& p);
                ISubParser& getParser(IParser& p) override;
                IObjectPtr getObject() override;

                StatePtr stateFunc;
            };               

            ISubParser& operator()(IParser& p) override;
            ISubParser& nextParser(IParser& p) noexcept;
        };

        template<typename Parser, typename ParserState,typename CharT>
        static ISubParserT<CharT>& StateTransition(
            Parser& current_parser, ParserState& state, IParserT<CharT>& parser)
        {
            throw typename Parser::Exception();
        }

        template<typename Parser, typename ParserState, typename CharT,
            typename Predicate, typename Action, typename NextState,
            typename... Predicates, typename... Actions, typename... NextStates>
            static ISubParserT<CharT>& StateTransition(
                Parser& current_parser, ParserState& s, IParserT<CharT>& parser,
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
            }
            else
                return StateTransition(current_parser, state, parser, ps...);

        }

        
        struct NoOp {
            template<typename ParserT, typename StateT, typename CharT >
            ISubParserT<CharT>& operator()(
                ParserT& current_parser, StateT& s, IParserT<CharT>&p)
            {
                return current_parser;
            }
        };

        
        struct Store {
            template<typename ParserT, typename StateT, typename CharT >
            ISubParserT<CharT>& operator()(
                ParserT& current_parser, StateT& s, IParserT<CharT>&p)
            {
                s.token.push_back(p.getCurrentChar());
                return current_parser;
            }
        };

        template<typename NextParserT>
        struct Call {
            template<typename ParserT, typename StateT, typename CharT>
            ISubParserT<CharT>& operator()(
                ParserT& current_parser, StateT& s, IParserT<CharT>&p)
            {
                p.getStateStack().push(ISubParserStateT<CharT>::Create<NextParserT>(p));
                return std::get<NextParserT>(p.getParsers())(p);
            }
        };

        struct Dispatch {
            template<typename ParserT, typename StateT, typename CharT>
            ISubParserT<CharT>& operator()(
                ParserT& current_parser, StateT& s, IParserT<CharT>&p)
            {
                return DispatchFirstSymbol(p, p.getParsers());
            }
        };

        template<typename JSONT>
        struct Return {
            template<typename ParserT, typename StateT, typename CharT>
            ISubParserT<CharT>& operator()(
                ParserT& current_parser, StateT& s, IParserT<CharT>&p)
            {
                s.object = JSON::Create<JSONT>(std::move(s.token));
                return current_parser.nextParser(p);
            }
        };

        template<typename JSONT>
        struct CallBack {
            template<typename ParserT, typename StateT, typename CharT>
            ISubParserT<CharT>& operator()(
                ParserT& current_parser, StateT& s, IParserT<CharT>&p)
            {
                s.object = JSON::Create<JSONT>(std::move(s.token));
                return current_parser.nextParser(p)(p);
            }
        };


        struct NoOpReturn {
            template<typename ParserT, typename StateT, typename CharT>
            ISubParserT<CharT>& operator()(
                ParserT& current_parser, StateT& s, IParserT<CharT>&p)
            {
                return current_parser.nextParser(p);
            }
        };

        template<typename CharT, CharT... Literal>
        class IsLiteral {
        public:
            bool operator()(CharT c) { return eval(c, Literal...); }
        private:
            bool eval(CharT c) { return false; }
            template<typename Char, typename... Chars>
            bool eval(CharT c, Char C, Chars... Cs) {
                if (c == C) return true;
                else return eval(c, Cs...);
            }
        };

        class Others {
        public:
            template<typename CharT>
            bool operator()(CharT c) { return true; }
        };

        template<typename CharT>
        ISubParserT<CharT>& CheckStartSymbol(IParserT<CharT>& IParser) {
            throw InvalidStartingSymbol();
        }

        template<typename CharT,typename SubParserT, typename... SubParsersT>
        std::enable_if_t<
            !std::is_same<SubParserT,WSParserT<CharT>>::value &&
            std::is_pointer<
                decltype(std::declval<SubParserT>().getFirstSymbolSet())
            >::value, ISubParserT<CharT>&
        > CheckStartSymbol(
            IParserT<CharT>& parser, SubParserT& subparser, SubParsersT&... subparsers) {
            std::basic_string<CharT> syms = subparser.getFirstSymbolSet();
            if (syms.find(parser.getCurrentChar()) == std::basic_string<CharT>::npos)
                return CheckStartSymbol(parser, subparsers...);
            else {
                parser.getStateStack().push(ISubParserStateT<CharT>::Create<SubParserT>(parser));
                return subparser(parser);
            }
        }

        template<typename CharT, typename SubParserT, typename... SubParsersT>
        std::enable_if_t<
            !std::is_same<SubParserT, WSParserT<CharT>>::value &&
            !std::is_pointer<
                decltype(std::declval<SubParserT>().getFirstSymbolSet())
            >::value, ISubParserT<CharT>&
        >CheckStartSymbol(
            IParserT<CharT>& parser, SubParserT& subparser, SubParsersT&... subparsers) {
            if (parser.getCurrentChar() != subparser.getFirstSymbolSet())
                return CheckStartSymbol(parser,subparsers...);
            else {
                parser.getStateStack().push(ISubParserStateT<CharT>::Create<SubParserT>(parser));
                return subparser(parser);
            }
        }

        // Skip dispatching on WSParser
        template<typename CharT,typename SubParserT, typename... SubParsersT>
        std::enable_if_t<
            std::is_same<SubParserT, WSParserT<CharT>>::value
            , ISubParserT<CharT>&
        >CheckStartSymbol(
            IParserT<CharT>& parser, SubParserT& subparser, SubParsersT&... subparsers) {
            return CheckStartSymbol(parser, subparsers...);
        }


        template<typename CharT,typename... Parsers>
        ISubParserT<CharT>& DispatchFirstSymbol(IParserT<CharT>& p,std::tuple<Parsers...>& parsers) {
            return CheckStartSymbol(p, std::get<Parsers>(parsers)...);
        }

        template<typename JSONLiteral>
        class LiteralParser : public ParserTemplate<LiteralParser<JSONLiteral>> {
        public:
            
            using BaseStateT = typename ParserTemplate<LiteralParser<JSONLiteral>>::State;
            struct State : public BaseStateT {
                using Base = BaseStateT;
                State(IParser& p);
                IObjectPtr getObject() override;
                size_t current_pos;
            };

            static CharT getFirstSymbolSet();
            static StatePtr getInitState();

        private:
            ISubParser& check(ISubParserState& state, IParser& p);
        };
        
        template<typename CharT>
        class NumberParserT : public ParserTemplate<NumberParserT<CharT>> {
        public:
            struct Exception : impl::Exception {};
            using MyJSONType = JSON::NumberT<CharT>;
            using BaseStateT = typename ParserTemplate<NumberParserT>::State;
            struct State : public BaseStateT {
                using BaseStateT::BaseStateT;

                IObjectPtr getObject() override;

                IObjectPtr object;
                StringT token;
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
        class ObjectParserT : public ParserTemplate<ObjectParserT<CharT>> {
        public:

            struct Exception : impl::Exception {};
            using MyJSONType = JSON::ObjectT<CharT>;
            struct State : public ParserTemplate<ObjectParserT>::State {
                explicit State(IParser& p);
                IObjectPtr getObject() override;

                IObjectPtr object;
                StringT currentKey;
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
        class ArrayParserT : public ParserTemplate<ArrayParserT<CharT>> {
        public:

            struct Exception : impl::Exception {};
            using MyJSONType = JSON::ArrayT<CharT>;
            struct State : public ParserTemplate<ArrayParserT>::State {
                explicit State(IParser& p);
                IObjectPtr getObject() override;

                IObjectPtr object;
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
        class StringParserT : public ParserTemplate<StringParserT<CharT>> {
        public:
            struct Exception : impl::Exception {};
            using MyJSONType = JSON::StringT<CharT>;
            using BaseStateT = typename ParserTemplate<StringParserT>::State;
            struct State : public BaseStateT {
                using BaseStateT::BaseStateT;
                IObjectPtr getObject() override;
                IObjectPtr object;
                StringT token;
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
        class WSParserT : public ParserTemplate<WSParserT<CharT>> {
        public:
            using BaseStateT = typename ParserTemplate<WSParserT>::State;
            struct State : public BaseStateT {
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

        template<typename ParserT>
        inline ISubParserT<typename ParserTemplate<ParserT>::CharT>&
            ParserTemplate<ParserT>::operator()(IParser& p) 
        {
            auto& s = ISubParserState::Cast(*this, *p.getStateStack().top());
            return (static_cast<ParserT*>(this)->*(s.stateFunc))(s, p);
        }

        template<typename ParserT>
        inline ISubParserT<typename ParserTemplate<ParserT>::CharT>&
            ParserTemplate<ParserT>::nextParser(IParser& p) noexcept 
        {
            p.getLastState() = std::move(p.getStateStack().top());
            p.getStateStack().pop();
            return p.getStateStack().top()->getParser(p);
        }

        template<typename ParserT>
        inline IObjectPtrT<typename ParserTemplate<ParserT>::CharT>
            ParserTemplate<ParserT>::State::getObject()
        {
            return  nullptr;
        }

        template<typename ParserT>
        inline ISubParserT<typename ParserTemplate<ParserT>::CharT>&
            ParserTemplate<ParserT>::State::getParser(IParser& p) 
        {
            return std::get<ParserT>(p.getParsers());
        }
        

        template<typename ParserT>
        inline ParserTemplate<ParserT>::State::State(IParser& p) :
            stateFunc{ ParserT::getInitState() }
        {}

        template<typename JSONLiteral>
        inline LiteralParser<JSONLiteral>::State::State(IParser& p) 
            : Base(p), current_pos{} {}

        template<typename JSONLiteral>
        inline IObjectPtrT<typename LiteralParser<JSONLiteral>::CharT>
            LiteralParser<JSONLiteral>::State::getObject() 
        {
            return JSON::Create<JSONLiteral>();
        }

        template<typename JSONLiteral>
        inline ISubParserT<typename LiteralParser<JSONLiteral>::CharT>&
            LiteralParser<JSONLiteral>::check(ISubParserState& s, IParser& p) 
        {
            auto& state = ISubParserState::Cast(*this,s);
            const size_t literal_size = 
                std::char_traits<CharT>::length(GetLiteral<JSONLiteral>());
            if (GetLiteral<JSONLiteral>()[state.current_pos++] != p.getCurrentChar())
                throw LiteralException();
            else if (state.current_pos == literal_size) {
                return nextParser(p);
            }
            else
                return *this;
        }

        template<typename JSONLiteral>
        inline typename LiteralParser<JSONLiteral>::CharT 
            LiteralParser<JSONLiteral>::getFirstSymbolSet() {
            return GetLiteral<JSONLiteral>()[0];
        }

        template<typename JSONLiteral>
        inline typename LiteralParser<JSONLiteral>::StatePtr 
            LiteralParser<JSONLiteral>::getInitState() {
            return &LiteralParser::check;
        }

        template<typename CharT>
        inline IObjectPtrT<CharT> NumberParserT<CharT>::State::getObject() {
            return std::move(object);
        }

        template<typename CharT>
        inline const CharT* NumberParserT<CharT>::getFirstSymbolSet() {
            return LiteralsT<CharT>::begin_number();
        }

        template<typename CharT>
        inline typename NumberParserT<CharT>::StatePtr 
            NumberParserT<CharT>::getInitState() {
            return &NumberParser::start;
        }


        template<typename CharT>
        inline ISubParserT<CharT>& NumberParserT<CharT>::start(
            ISubParserState& state, IParser& p) 
        {
            return StateTransition(*this, state, p,
                std::make_tuple(IsLiteral<CharT,'0'>{}, Store{}, &NumberParser::startingZero),
                std::make_tuple(std::isdigit, Store{}, &NumberParser::integerPart),
                std::make_tuple(IsLiteral<CharT,'-'> {}, Store{}, &NumberParser::minus)
                );
        }

        template<typename CharT>
        inline ISubParserT<CharT>& NumberParserT<CharT>::minus(
            ISubParserState& state, IParser& p) 
        {
            return StateTransition(*this, state, p,
                std::make_tuple(IsLiteral<CharT,'0'>{}, Store{}, &NumberParser::startingZero),
                std::make_tuple(std::isdigit, Store{}, &NumberParser::integerPart)
                );
        }


        template<typename CharT>
        inline ISubParserT<CharT>& NumberParserT<CharT>::startingZero(
            ISubParserState& state, IParser& p) 
        {
            return StateTransition(*this, state, p,
                std::make_tuple(IsLiteral<CharT,'.'>{}, Store{}, 
                    &NumberParser::fractionPartStart),
                std::make_tuple(Others{}, CallBack<MyJSONType>{}, nullptr)
                );
        }


        template<typename CharT>
        inline ISubParserT<CharT>& NumberParserT<CharT>::integerPart(
            ISubParserState& state, IParser& p) 
        {
            return StateTransition(*this, state, p,
                std::make_tuple(std::isdigit, Store{}, &NumberParser::integerPart),
                std::make_tuple(IsLiteral<CharT,'.'>{}, Store{}, &NumberParser::fractionPartStart),
                std::make_tuple(IsLiteral<CharT,'e', 'E'>{}, Store{}, &NumberParser::exponentPartStart),
                std::make_tuple(Others{}, CallBack<MyJSONType>{}, nullptr)
                );
        }

        template<typename CharT>
        inline ISubParserT<CharT>& NumberParserT<CharT>::fractionPartStart(
            ISubParserState& state, IParser& p) 
        {
            return StateTransition(*this, state, p,
                std::make_tuple(std::isdigit, Store{}, &NumberParser::fractionPart)
                );
        }

        template<typename CharT>
        inline ISubParserT<CharT>& NumberParserT<CharT>::fractionPart(
            ISubParserState& state, IParser& p) 
        {
            return StateTransition(*this, state, p,
                std::make_tuple(std::isdigit, Store{}, &NumberParser::fractionPart),
                std::make_tuple(IsLiteral<CharT,'e', 'E'>{}, Store{}, &NumberParser::exponentPartStart),
                std::make_tuple(Others{}, CallBack<MyJSONType>{}, nullptr)
                );
        }

        template<typename CharT>
        inline ISubParserT<CharT>& NumberParserT<CharT>::exponentPartStart(
            ISubParserState& state, IParser& p) 
        {
            return StateTransition(*this, state, p,
                std::make_tuple(IsLiteral<CharT,'-', '+'>{}, Store{}, &NumberParser::exponentPartStartSigned),
                std::make_tuple(std::isdigit, Store{}, &NumberParser::exponentPart)
                );
        }

        template<typename CharT>
        inline ISubParserT<CharT>& NumberParserT<CharT>::exponentPartStartSigned(
            ISubParserState& state, IParser& p) 
        {
            return StateTransition(*this, state, p,
                std::make_tuple(std::isdigit, Store{}, &NumberParser::exponentPart)
                );
        }

        template<typename CharT>
        inline ISubParserT<CharT>& NumberParserT<CharT>::exponentPart(
            ISubParserState& state, IParser& p) 
        {
            return StateTransition(*this, state, p,
                std::make_tuple(std::isdigit, Store{}, &NumberParser::exponentPart),
                std::make_tuple(Others{}, CallBack<MyJSONType>{}, nullptr)
                );
        }

        template<typename CharT>
        inline ObjectParserT<CharT>::State::State(IParser& p) :
            ParserTemplate<ObjectParserT<CharT>>::State(p),
            object{ JSON::Create<ObjectT<CharT>>() }
        {}

        template<typename CharT>
        inline IObjectPtrT<CharT> ObjectParserT<CharT>::State::getObject() 
        {
            return std::move(object);
        }

        template<typename CharT>
        inline CharT ObjectParserT<CharT>::getFirstSymbolSet() {
            return LiteralsT<CharT>::begin_object;
        }


        template<typename CharT>
        inline typename ObjectParserT<CharT>::StatePtr ObjectParserT<CharT>::getInitState() {
            return &ObjectParser::start;
        }
        
        template<typename CharT>
        inline ISubParserT<CharT>& ObjectParserT<CharT>::start(
            ISubParserState& s, IParser& p) 
        {
            return StateTransition(*this, s, p,
                std::make_tuple(IsLiteral<CharT,LiteralsT<CharT>::begin_object>{},
                    NoOp{}, &ObjectParser::parseKey)
                );
        }

        template<typename CharT>
        inline ISubParserT<CharT>& ObjectParserT<CharT>::parseKey(
            ISubParserState& s, IParser& p) 
        {
            return StateTransition(*this, s, p,
                std::make_tuple(IsWhitespace<CharT>, NoOp{}, &ObjectParser::parseKey),
                std::make_tuple(IsLiteral<CharT,LiteralsT<CharT>::end_object>{}, 
                    NoOpReturn{},nullptr),
                std::make_tuple(IsLiteral<CharT,LiteralsT<CharT>::quotation_mark>{},
                    Call<StringParser>{}, &ObjectParser::retrieveKey)
                );
        }

        template<typename CharT>
        inline ISubParserT<CharT>& ObjectParserT<CharT>::retrieveKey(
            ISubParserState& s, IParser& p) 
        {
            auto& state = ISubParserState::Cast(*this,s);
            state.currentKey = std::move(p.getLastState()->getObject()->getValue());
            return parseSeparator(s, p);
        }

        template<typename CharT>
        inline ISubParserT<CharT>& ObjectParserT<CharT>::parseSeparator(
            ISubParserState& s, IParser& p) 
        {
            return StateTransition(*this, s, p,
                std::make_tuple(IsWhitespace<CharT>, NoOp{}, &ObjectParser::parseSeparator),
                std::make_tuple(IsLiteral<CharT,LiteralsT<CharT>::name_separator>{}, 
                    NoOp{}, &ObjectParser::parseValue)
                );
        }

        template<typename CharT>
        inline ISubParserT<CharT>& ObjectParserT<CharT>::parseValue(
            ISubParserState& s, IParser& p) 
        {
            return StateTransition(*this, s, p,
                std::make_tuple(IsWhitespace<CharT>, NoOp{}, &ObjectParser::parseValue),
                std::make_tuple(Others{}, Dispatch{}, &ObjectParser::retrieveValue)
                );
        }

        template<typename CharT>
        inline ISubParserT<CharT>& ObjectParserT<CharT>::retrieveValue(
            ISubParserState& s, IParser& p) 
        {
            auto& state = ISubParserState::Cast(*this,s);
            auto& currentObject = static_cast<ObjectT<CharT>&>(*state.object);

            currentObject.emplace(
                std::move(state.currentKey),
                std::move(p.getLastState()->getObject()));
            
            return nextMember(s, p);
        }
        
        template<typename CharT>
        ISubParserT<CharT>& ObjectParserT<CharT>::nextMember(
            ISubParserState& s, IParser& p) 
        {
            return StateTransition(*this, s, p,
                std::make_tuple(IsWhitespace<CharT>, NoOp{}, &ObjectParser::nextMember),
                std::make_tuple(IsLiteral<CharT,LiteralsT<CharT>::end_object>{}, NoOpReturn{}, nullptr),
                std::make_tuple(IsLiteral<CharT,LiteralsT<CharT>::value_separator>{}, 
                    NoOp{}, &ObjectParser::parseKey)
                );
        }


        template<typename CharT>
        inline ArrayParserT<CharT>::State::State(IParser& p) :
            ParserTemplate<ArrayParser>::State(p),
            object{ JSON::Create<ArrayT<CharT>>() }
        {}

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
            return &ArrayParser::start;
        }

        template<typename CharT>
        inline ISubParserT<CharT>& ArrayParserT<CharT>::start(
            ISubParserState& s, IParser& p) 
        {
            return StateTransition(*this, s, p,
                std::make_tuple(
                    IsLiteral<CharT,LiteralsT<CharT>::begin_array>{}, NoOp{}, &ArrayParser::parseValue)
                );
        }

        template<typename CharT>
        inline ISubParserT<CharT>& ArrayParserT<CharT>::parseValue(
            ISubParserState& s, IParser& p) 
        {
            return StateTransition(*this, s, p,
                std::make_tuple(IsWhitespace<CharT>, NoOp{}, &ArrayParser::parseValue),
                std::make_tuple(Others{}, Dispatch{}, &ArrayParser::retrieveValue)
                );
        }

        template<typename CharT>
        inline ISubParserT<CharT>& ArrayParserT<CharT>::retrieveValue(
            ISubParserState& s, IParser& p) 
        {
            auto& state = ISubParserState::Cast(*this,s);
            auto& currentObject = static_cast<ArrayT<CharT>&>(*state.object);

            currentObject.emplace(
                std::move(p.getLastState()->getObject()));

            return nextMember(s, p);
        }

        template<typename CharT>
        ISubParserT<CharT>& ArrayParserT<CharT>::nextMember(
            ISubParserState& s, IParser& p) 
        {
            return StateTransition(*this, s, p,
                std::make_tuple(IsWhitespace<CharT>, NoOp{}, &ArrayParser::nextMember),
                std::make_tuple(IsLiteral<CharT,LiteralsT<CharT>::end_array>{}, NoOpReturn{}, nullptr),
                std::make_tuple(IsLiteral<CharT,LiteralsT<CharT>::value_separator>{}, NoOp{}, &ArrayParser::parseValue)
                );
        }


        template<typename CharT>
        inline const CharT* WSParserT<CharT>::getFirstSymbolSet() {
            return LiteralsT<CharT>::whitespace_string();
        }

        template<typename CharT>
        inline typename WSParserT<CharT>::StatePtr WSParserT<CharT>::getInitState() {
            return &WSParser::emplaceAndDispatch;
        }

        template<typename CharT>
        inline ISubParserT<CharT>& WSParserT<CharT>::emplaceAndDispatch(
            ISubParserState& state, IParser& p) 
        {
            auto& s = static_cast<State&>(state);

            if (p.getLastState()) {
                auto last_state = std::move(p.getLastState());
                p.getObjects().emplace_back(
                    std::move(last_state->getObject()));
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
        inline CharT StringParserT<CharT>::getFirstSymbolSet() {
            return LiteralsT<CharT>::quotation_mark;
        }

        template<typename CharT>
        inline typename StringParserT<CharT>::StatePtr 
            StringParserT<CharT>::getInitState() {
            return &StringParser::start;
        }

        
        template<typename CharT>
        inline ISubParserT<CharT>& StringParserT<CharT>::start(
            ISubParserState& s, IParser& p) 
        {
            return StateTransition(*this, s, p,
                std::make_tuple(IsLiteral<CharT,LiteralsT<CharT>::quotation_mark>{}, NoOp{}, &StringParserT<CharT>::parseChar)
                );
        }

        template<typename CharT>
        inline ISubParserT<CharT>& StringParserT<CharT>::parseChar(
            ISubParserState& s, IParser& p) 
        {
            return StateTransition(*this, s, p,
                std::make_tuple(IsLiteral<CharT,LiteralsT<CharT>::quotation_mark>{}, 
                    Return<MyJSONType>{}, nullptr),
                std::make_tuple(IsLiteral<CharT,LiteralsT<CharT>::string_escape>{}, NoOp{},
                    &StringParser::parseEscapeChar),
                std::make_tuple([](CharT c) -> bool{
                return c == 0x20 || c == 0x21 || (c >= 0x23 && c <= 0x5b) ||
                    (c >= 0x5D && c <= 0x10FFFF);
                },Store{}, &StringParser::parseChar)
                );
        }

        template<typename CharT>
        inline ISubParserT<CharT>& StringParserT<CharT>::parseEscapeChar(
            ISubParserState& s, IParser& p) 
        {
            return StateTransition(*this, s, p,
                std::make_tuple(IsLiteral<CharT,JSON::LiteralsT<CharT>::string_unicode_escape>{}, 
                    NoOp{}, &StringParser::parseUnicodeEscapeChar),
                std::make_tuple([](CharT c) -> bool {
                    for(auto e : JSON::LiteralsT<CharT>::string_escapes()) 
                        if(e == c)return true;
                    return false;
                }, Store{}, &StringParser::parseChar)
                );
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
    class Parser : public impl::IParserT<CharT> {
    public:

        Parser() : current_char{} 
        {
            stateStack.push(
                ISubParserStatePtr(
                    ISubParserState::Create<impl::WSParserT<CharT>>(*this)));
        }

        ObjContainer retrieveObjects() {
            if (!isRetrievable()) throw impl::ParsingIncomplete();
            return std::move(objects);
        }

        bool isRetrievable()const noexcept {
            return stateStack.size() == 1;
        }

        operator bool()const noexcept {
            return isRetrievable();
        }

        void operator()(CharT c) {
            current_char = c;
            stateStack.top()->getParser(*this)(*this);
        }

        Parser(const Parser&) = delete;
        Parser& operator=(const Parser&) = delete;
        Parser(Parser&&) = delete;
        Parser& operator=(Parser&&) = delete;

    private:
        CharT getCurrentChar() const noexcept override { return current_char; }

        ObjContainer& getObjects() noexcept override { return objects; }

        ParserTuple& getParsers() noexcept override { return parsers; }
        
        ParserStateStack& getStateStack() noexcept override {
            return stateStack;
        }
        
        ISubParserStatePtr& getLastState() noexcept override {
            return lastState;
        }

        CharT current_char;
        ObjContainer objects;
        ParserTuple parsers;
        ParserStateStack stateStack;
        ISubParserStatePtr lastState;

    };
    
    template<typename ForwardIterator>
    ObjContainerT<std::decay_t<decltype(*(std::declval<ForwardIterator>()))>> 
        parse(ForwardIterator start, ForwardIterator end) {
        
        Parser<std::decay_t<decltype(*(std::declval<ForwardIterator>()))>> p;

        for (; start != end; ++start) {
            p(*start);
        }

        return p.retrieveObjects();
    };


}

#endif //JSONPARSER_H_INCLUDED__