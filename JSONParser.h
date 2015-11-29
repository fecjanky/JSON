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


namespace JSON {
    
    using ObjContainer = std::vector<IObjectPtr>;
    
    namespace impl {
        
        
        struct IParser;
        struct ISubParser;
        struct ISubParserState;
        template<typename ParserT>
        class ParserTemplate;
        class WSParser;
        template<typename JSONLiteral>
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
            NullParser, 
            TrueParser, 
            FalseParser,
            NumberParser,
            ObjectParser,
            StringParser
        >;
        
        using ISubParserStatePtr = std::unique_ptr<ISubParserState>;
        using ParserStateStack = std::stack<ISubParserStatePtr>;

        struct Exception : public std::exception {};
        struct InvalidStartingSymbol : public impl::Exception {};
        struct LiteralException : public impl::Exception {};
        struct ParsingIncomplete : public impl::Exception {};

        template<typename JSONLiteral>
        const char* GetLiteral() { static_assert(0, "Type is not literal type"); }

        template<> const char* GetLiteral<Null>() { return value_null; }
        template<> const char* GetLiteral<True>() { return value_true; }
        template<> const char* GetLiteral<False>() { return value_false; }

        inline bool IsWhitespace(char c) {
            for (auto w : JSON::ws) if (c == w) return true;
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
            static typename SubParserImpl::State& Cast(
                SubParserImpl* t,ISubParserState& s) {
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

        struct IParser {
            struct Exception : public std::exception {};
            virtual char getCurrentChar() const noexcept = 0;
            virtual ObjContainer& getObjects() noexcept = 0;
            virtual  ParserTuple& getParsers() noexcept = 0;
            virtual ParserStateStack& getStateStack() noexcept = 0;
            virtual ISubParserStatePtr& getLastState() noexcept = 0;
        protected:
            virtual ~IParser() = default;
        };


        template<typename ParserT>
        class ParserTemplate : public JSON::impl::ISubParser{
        public:
            
            using StatePtr = 
                JSON::impl::ISubParser& (ParserT::*)(ISubParserState&,IParser&);

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

        template<typename Parser, typename ParserState>
        static ISubParser& StateTransition(
            Parser& current_parser, ParserState& state, IParser& parser) {
            throw typename Parser::Exception();
        }

        template<typename Parser, typename ParserState,
            typename Predicate, typename Action, typename NextState,
            typename... Predicates, typename... Actions, typename... NextStates>
            static ISubParser& StateTransition(
                Parser& current_parser, ParserState& state, IParser& parser,
                std::tuple<Predicate, Action, NextState>& p, 
                std::tuple<Predicates, Actions, NextStates>&... ps) {
            
            static_assert(
                std::is_base_of<ISubParser, Parser>::value && 
                std::is_base_of<ISubParserState, ParserState>::value, 
                "State transition not possible");

            if (std::get<0>(p)(parser.getCurrentChar())) {
                state.stateFunc = std::get<2>(p);
                ISubParser& next_parser = std::get<1>(p)(current_parser, state, parser);
                return next_parser;
            }
            else
                return StateTransition(current_parser, state, parser, ps...);

        }

        
        struct NoOp {
            template<typename ParserT, typename StateT>
            ISubParser& operator()(
                ParserT& current_parser, StateT& s, IParser&p) {
                return current_parser;
            }
        };

        
        struct Store {
            template<typename ParserT, typename StateT>
            ISubParser& operator()(
                ParserT& current_parser, StateT& s, IParser&p) {
                s.token.push_back(p.getCurrentChar());
                return current_parser;
            }
        };

        template<typename NextParserT>
        struct Call {
            template<typename ParserT, typename StateT>
            ISubParser& operator()(
                ParserT& current_parser, StateT& s, IParser&p) {
                p.getStateStack().push(ISubParserState::Create<NextParserT>(p));
                return std::get<NextParserT>(p.getParsers())(p);
            }
        };

        struct Dispatch {
            template<typename ParserT, typename StateT>
            ISubParser& operator()(
                ParserT& current_parser, StateT& s, IParser&p) {
                return DispatchFirstSymbol(p, p.getParsers());
            }
        };

        template<typename JSONT>
        struct Return {
            template<typename ParserT, typename StateT>
            ISubParser& operator()(
                ParserT& current_parser, StateT& s, IParser&p) {
                s.object = JSON::Create<JSONT>(std::move(s.token));
                return current_parser.nextParser(p);
            }
        };

        template<typename JSONT>
        struct CallBack {
            template<typename ParserT, typename StateT>
            ISubParser& operator()(
                ParserT& current_parser, StateT& s, IParser&p) {
                s.object = JSON::Create<JSONT>(std::move(s.token));
                return current_parser.nextParser(p)(p);
            }
        };


        struct NoOpReturn {
            template<typename ParserT, typename StateT>
            ISubParser& operator()(
                ParserT& current_parser, StateT& s, IParser&p) {
                return current_parser.nextParser(p);
            }
        };

        template<char... Literal>
        class IsLiteral {
        public:
            bool operator()(char c) { return eval(c, Literal...); }
        private:
            bool eval(char c) { return false; }
            template<typename Char, typename... Chars>
            bool eval(char c, Char C, Chars... Cs) {
                if (c == C) return true;
                else return eval(c, Cs...);
            }
        };

        class Others {
        public:
            bool operator()(char c) { return true; }
        };

        ISubParser& CheckStartSymbol(IParser& IParser) {
            throw InvalidStartingSymbol();
        }

        template<typename SubParserT, typename... SubParsersT>
        std::enable_if_t<
            !std::is_same<SubParserT,WSParser>::value &&
            std::is_pointer<
                decltype(std::declval<SubParserT>().getFirstSymbolSet())
            >::value, ISubParser&
        > CheckStartSymbol(
            IParser& parser, SubParserT& subparser, SubParsersT&... subparsers) {
            std::string syms = subparser.getFirstSymbolSet();
            if (syms.find(parser.getCurrentChar()) == std::string::npos) 
                return CheckStartSymbol(parser, subparsers...);
            else {
                parser.getStateStack().push(ISubParserState::Create<SubParserT>(parser));
                return subparser(parser);
            }
        }

        template<typename SubParserT, typename... SubParsersT>
        std::enable_if_t<
            !std::is_same<SubParserT, WSParser>::value &&
            !std::is_pointer<
                decltype(std::declval<SubParserT>().getFirstSymbolSet())
            >::value, ISubParser&
        >CheckStartSymbol(
            IParser& parser, SubParserT& subparser, SubParsersT&... subparsers) {
            if (parser.getCurrentChar() != subparser.getFirstSymbolSet())
                return CheckStartSymbol(parser,subparsers...);
            else {
                parser.getStateStack().push(ISubParserState::Create<SubParserT>(parser));
                return subparser(parser);
            }
        }

        // Skip dispatching on WSParser
        template<typename SubParserT, typename... SubParsersT>
        std::enable_if_t<
            std::is_same<SubParserT, WSParser>::value
            , ISubParser&
        >CheckStartSymbol(
            IParser& parser, SubParserT& subparser, SubParsersT&... subparsers) {
            return CheckStartSymbol(parser, subparsers...);
        }


        template<typename... Parsers>
        ISubParser& DispatchFirstSymbol(IParser& p,std::tuple<Parsers...>& parsers) {
            return CheckStartSymbol(p, std::get<Parsers>(parsers)...);
        }

        template<typename JSONLiteral>
        class LiteralParser : public ParserTemplate<LiteralParser<JSONLiteral>> {
        public:

            struct State : public ParserTemplate<LiteralParser<JSONLiteral>>::State {
                using Base = ParserTemplate<LiteralParser<JSONLiteral>>::State;
                State(IParser& p);
                IObjectPtr getObject() override;
                size_t current_pos;
            };

            static char getFirstSymbolSet();
            static StatePtr getInitState();

        private:
            ISubParser& check(ISubParserState& state, IParser& p);
        };
        
        class NumberParser : public ParserTemplate<NumberParser> {
        public:
            struct Exception : impl::Exception {};

            struct State : public ParserTemplate<NumberParser>::State {
                using ParserTemplate<NumberParser>::State::State;

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

        class ObjectParser : public ParserTemplate<ObjectParser> {
        public:

            struct Exception : impl::Exception {};

            struct State : public ParserTemplate<ObjectParser>::State {
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

        class StringParser : public ParserTemplate<StringParser> {
        public:
            struct Exception : impl::Exception {};
            
            struct State : public ParserTemplate<StringParser>::State {
                using ParserTemplate<StringParser>::State::State;
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

        class WSParser : public ParserTemplate<WSParser> {
        public:
            struct State : public ParserTemplate<WSParser>::State {
                using ParserTemplate<WSParser>::State::State;
            };

            static const char* getFirstSymbolSet();
            static StatePtr getInitState();

        private:
            ISubParser& emplaceAndDispatch(ISubParserState& state, IParser& p);
        };


        ////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////

        template<typename ParserT>
        inline ISubParser& ParserTemplate<ParserT>::operator()(IParser& p) {
            auto& s = ISubParserState::Cast(this, *p.getStateStack().top());
            return (static_cast<ParserT*>(this)->*(s.stateFunc))(s, p);
        }

        template<typename ParserT>
        inline ISubParser& ParserTemplate<ParserT>::nextParser(IParser& p) noexcept {
            p.getLastState() = std::move(p.getStateStack().top());
            p.getStateStack().pop();
            return p.getStateStack().top()->getParser(p);
        }

        template<typename ParserT>
        inline ISubParser& ParserTemplate<ParserT>::State::getParser(IParser& p) {
            return std::get<ParserT>(p.getParsers());
        }

        template<typename ParserT>
        inline IObjectPtr ParserTemplate<ParserT>::State::getObject() {
            return IObjectPtr{ nullptr };
        }

        template<typename ParserT>
        inline ParserTemplate<ParserT>::State::State(IParser& p) :
            stateFunc{ ParserT::getInitState() }
        {}

        template<typename JSONLiteral>
        inline LiteralParser<JSONLiteral>::State::State(IParser& p) 
            : Base(p), current_pos{} {}

        template<typename JSONLiteral>
        inline IObjectPtr LiteralParser<JSONLiteral>::State::getObject() {
            return JSON::Create<JSONLiteral>();
        }

        template<typename JSONLiteral>
        inline ISubParser&  LiteralParser<JSONLiteral>::check(
            ISubParserState& state, IParser& p) {
            auto& s = ISubParserState::Cast(this, state);
            const size_t literal_size = strlen(GetLiteral<JSONLiteral>());
            if (GetLiteral<JSONLiteral>()[s.current_pos++] != p.getCurrentChar())
                throw LiteralException();
            else if (s.current_pos == literal_size) {
                return nextParser(p);
            }
            else
                return *this;
        }

        template<typename JSONLiteral>
        inline char LiteralParser<JSONLiteral>::getFirstSymbolSet() {
            return GetLiteral<JSONLiteral>()[0];
        }

        template<typename JSONLiteral>
        inline typename LiteralParser<JSONLiteral>::StatePtr 
            LiteralParser<JSONLiteral>::getInitState() {
            return &LiteralParser::check;
        }

        inline IObjectPtr NumberParser::State::getObject() {
            return std::move(object);
        }
        
        inline const char* NumberParser::getFirstSymbolSet() {
            return "-0123456789";
        }

        inline NumberParser::StatePtr NumberParser::getInitState() {
            return &NumberParser::start;
        }

        inline ISubParser& NumberParser::start(ISubParserState& state, IParser& p) {
            return StateTransition(*this, ISubParserState::Cast(this, state), p,
                std::make_tuple(IsLiteral<'0'>{}, Store{}, &NumberParser::startingZero),
                std::make_tuple(std::isdigit, Store{}, &NumberParser::integerPart),
                std::make_tuple(IsLiteral<'-'> {}, Store{}, &NumberParser::minus)
                );
        }

        inline ISubParser& NumberParser::minus(ISubParserState& state, IParser& p) {
            return StateTransition(*this, ISubParserState::Cast(this, state), p,
                std::make_tuple(IsLiteral<'0'>{}, Store{}, &NumberParser::startingZero),
                std::make_tuple(std::isdigit, Store{}, &NumberParser::integerPart)
                );
        }

        inline ISubParser& NumberParser::startingZero(ISubParserState& state, IParser& p) {
            return StateTransition(*this, ISubParserState::Cast(this, state), p,
                std::make_tuple(IsLiteral<'.'>{}, Store{}, &NumberParser::fractionPartStart),
                std::make_tuple(Others{}, CallBack<JSON::Number>{}, nullptr)
                );
        }

        inline ISubParser& NumberParser::integerPart(ISubParserState& state, IParser& p) {
            return StateTransition(*this, ISubParserState::Cast(this, state), p,
                std::make_tuple(std::isdigit, Store{}, &NumberParser::integerPart),
                std::make_tuple(IsLiteral<'.'>{}, Store{}, &NumberParser::fractionPartStart),
                std::make_tuple(IsLiteral<'e', 'E'>{}, Store{}, &NumberParser::exponentPartStart),
                std::make_tuple(Others{}, CallBack<JSON::Number>{}, nullptr)
                );
        }

        inline ISubParser& NumberParser::fractionPartStart(ISubParserState& state, IParser& p) {
            return StateTransition(*this, ISubParserState::Cast(this, state), p,
                std::make_tuple(std::isdigit, Store{}, &NumberParser::fractionPart)
                );
        }

        inline ISubParser& NumberParser::fractionPart(ISubParserState& state, IParser& p) {
            return StateTransition(*this, ISubParserState::Cast(this, state), p,
                std::make_tuple(std::isdigit, Store{}, &NumberParser::fractionPart),
                std::make_tuple(IsLiteral<'e', 'E'>{}, Store{}, &NumberParser::exponentPartStart),
                std::make_tuple(Others{}, CallBack<JSON::Number>{}, nullptr)
                );
        }

        inline ISubParser& NumberParser::exponentPartStart(ISubParserState& state, IParser& p) {
            return StateTransition(*this, ISubParserState::Cast(this, state), p,
                std::make_tuple(IsLiteral<'-', '+'>{}, Store{}, &NumberParser::exponentPartStartSigned),
                std::make_tuple(std::isdigit, Store{}, &NumberParser::exponentPart)
                );
        }

        inline ISubParser& NumberParser::exponentPartStartSigned(ISubParserState& state, IParser& p) {
            return StateTransition(*this, ISubParserState::Cast(this, state), p,
                std::make_tuple(std::isdigit, Store{}, &NumberParser::exponentPart)
                );
        }

        inline ISubParser& NumberParser::exponentPart(ISubParserState& state, IParser& p) {
            return StateTransition(*this, ISubParserState::Cast(this, state), p,
                std::make_tuple(std::isdigit, Store{}, &NumberParser::exponentPart),
                std::make_tuple(Others{}, CallBack<JSON::Number>{}, nullptr)
                );
        }

        inline ObjectParser::State::State(IParser& p) :
            ParserTemplate<ObjectParser>::State(p),
            object{ JSON::Create<Object>() }
        {}

        inline IObjectPtr ObjectParser::State::getObject() {
            return std::move(object);
        }

        inline char ObjectParser::getFirstSymbolSet() {
            return begin_object;
        }

        inline ObjectParser::StatePtr ObjectParser::getInitState() {
            return &ObjectParser::start;
        }
        
        inline ISubParser& ObjectParser::start(ISubParserState& s, IParser& p) {
            return StateTransition(*this, ISubParserState::Cast(this, s), p,
                std::make_tuple(IsLiteral<JSON::begin_object>{}, NoOp{}, &ObjectParser::parseKey)
                );
        }

        inline ISubParser& ObjectParser::parseKey(ISubParserState& s, IParser& p) {
            return StateTransition(*this, ISubParserState::Cast(this, s), p,
                std::make_tuple(IsWhitespace, NoOp{}, &ObjectParser::parseKey),
                std::make_tuple(IsLiteral<end_object>{}, NoOpReturn{},nullptr),
                std::make_tuple(IsLiteral<JSON::quotation_mark>{}, 
                    Call<StringParser>{}, &ObjectParser::retrieveKey)
                );
        }

        inline ISubParser& ObjectParser::retrieveKey(ISubParserState& s, IParser& p) {
            auto& state = ISubParserState::Cast(this, s);
            state.currentKey = std::move(p.getLastState()->getObject()->getValue());
            return parseSeparator(s, p);
        }

        inline ISubParser& ObjectParser::parseSeparator(ISubParserState& s, IParser& p) {
            return StateTransition(*this, ISubParserState::Cast(this, s), p,
                std::make_tuple(IsWhitespace, NoOp{}, &ObjectParser::parseSeparator),
                std::make_tuple(IsLiteral<name_separator>{}, NoOp{}, &ObjectParser::parseValue)
                );
        }

        inline ISubParser& ObjectParser::parseValue(ISubParserState& s, IParser& p) {
            return StateTransition(*this, ISubParserState::Cast(this, s), p,
                std::make_tuple(IsWhitespace, NoOp{}, &ObjectParser::parseValue),
                std::make_tuple(Others{}, Dispatch{}, &ObjectParser::retrieveValue)
                );
        }

        inline ISubParser& ObjectParser::retrieveValue(ISubParserState& s, IParser& p) {
            auto& state = ISubParserState::Cast(this, s);
            auto& currentObject = static_cast<Object&>(*state.object);

            currentObject.emplace(
                std::move(state.currentKey),
                std::move(p.getLastState()->getObject()));
            
            return nextMember(s, p);
        }

        ISubParser& ObjectParser::nextMember(ISubParserState& s, IParser& p) {
            return StateTransition(*this, ISubParserState::Cast(this, s), p,
                std::make_tuple(IsWhitespace, NoOp{}, &ObjectParser::nextMember),
                std::make_tuple(IsLiteral<end_object>{}, NoOpReturn{}, nullptr),
                std::make_tuple(IsLiteral<value_separator>{}, NoOp{}, &ObjectParser::parseKey)
                );
        }

        inline const char* WSParser::getFirstSymbolSet() {
            return " \t\n\r";
        }

        inline WSParser::StatePtr WSParser::getInitState() {
            return &WSParser::emplaceAndDispatch;
        }

        inline ISubParser& WSParser::emplaceAndDispatch(
            ISubParserState& state, IParser& p) {
            auto& s = static_cast<State&>(state);

            if (p.getLastState()) {
                auto last_state = std::move(p.getLastState());
                p.getObjects().emplace_back(
                    std::move(last_state->getObject()));
            }

            if (IsWhitespace(p.getCurrentChar()))
                return *this;
            else {
                return DispatchFirstSymbol(p, p.getParsers());
            }
        }

        inline IObjectPtr StringParser::State::getObject() {
            return std::move(object);
        }

        inline char StringParser::getFirstSymbolSet() {
            return JSON::quotation_mark;
        }

        inline StringParser::StatePtr StringParser::getInitState() {
            return &StringParser::start;
        }

        
        ISubParser& StringParser::start(ISubParserState& s, IParser& p) {
            return StateTransition(*this, ISubParserState::Cast(this, s), p,
                std::make_tuple(IsLiteral <JSON::quotation_mark>{}, NoOp{}, &StringParser::parseChar)
                );
        }

        inline ISubParser& StringParser::parseChar(ISubParserState& s, IParser& p) {
            return StateTransition(*this, ISubParserState::Cast(this, s), p,
                std::make_tuple(IsLiteral <JSON::quotation_mark>{}, Return<JSON::String>{}, nullptr),
                std::make_tuple(IsLiteral <JSON::string_escape>{}, NoOp{}, &StringParser::parseEscapeChar),
                std::make_tuple([](char c) -> bool{ 
                return c == 0x20 || c == 0x21 || (c >= 0x23 && c <= 0x5b) ||
                    (c >= 0x5D && c <= 0x10FFFF);
                },Store{}, &StringParser::parseChar)
                );
        }

        inline ISubParser& StringParser::parseEscapeChar(ISubParserState& s, IParser& p) {
            return StateTransition(*this, ISubParserState::Cast(this, s), p,
                std::make_tuple(IsLiteral <JSON::string_unicode_escape>{}, NoOp{}, 
                    &StringParser::parseUnicodeEscapeChar),
                std::make_tuple([](char c) -> bool {
                    for(auto e : JSON::string_escapes) 
                        if(e == c)return true;
                    return false;
                }, Store{}, &StringParser::parseChar)
                );
        }

        inline ISubParser& StringParser::parseUnicodeEscapeChar(ISubParserState& s, IParser& p) {
            // throws exception!
            // TODO: implement parsing of Unicode escapes
            return StateTransition(*this, ISubParserState::Cast(this, s), p);
        }

    }

    class Parser : public impl::IParser {
    public:

        Parser() : current_char{} 
        {
            stateStack.push(
                impl::ISubParserStatePtr(
                    impl::ISubParserState::Create<impl::WSParser>(*this)));
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

        void operator()(char c) {
            current_char = c;
            stateStack.top()->getParser(*this)(*this);
        }

        Parser(const Parser&) = delete;
        Parser& operator=(const Parser&) = delete;
        Parser(Parser&&) = delete;
        Parser& operator=(Parser&&) = delete;

    private:
        char getCurrentChar() const noexcept override { return current_char; }

        ObjContainer& getObjects() noexcept override { return objects; }

        impl::ParserTuple& getParsers() noexcept override { return parsers; }
        
        impl::ParserStateStack& getStateStack() noexcept override {
            return stateStack;
        }
        
        impl::ISubParserStatePtr& getLastState() noexcept override {
            return lastState;
        }

        char current_char;
        ObjContainer objects;
        impl::ParserTuple parsers;
        impl::ParserStateStack stateStack;
        impl::ISubParserStatePtr lastState;

    };
    
    template<typename ForwardIterator>
    ObjContainer parse(ForwardIterator start, ForwardIterator end) {
        Parser p;

        for (; start != end; ++start) {
            p(*start);
        }

        return p.retrieveObjects();
    };


}

#endif //JSONPARSER_H_INCLUDED__