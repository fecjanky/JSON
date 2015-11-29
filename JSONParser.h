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
        
        using ParserTuple = std::tuple<WSParser, NullParser, TrueParser, FalseParser,NumberParser,ObjectParser>;
        
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
            static typename SubParserImpl::State& cast(SubParserImpl* t,ISubParserState& s) {
                return static_cast<typename SubParserImpl::State&>(s);
            }

            template<typename SubParserT>
            static
            std::enable_if_t<std::is_base_of<ISubParser, SubParserT>::value
                , ISubParserStatePtr
            > Create(IParser& p) {
                return ISubParserStatePtr(new typename SubParserT::State{ p });
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
            
            using StatePtr = JSON::impl::ISubParser& (ParserT::*)(ISubParserState&,IParser&);

            class State : public ISubParserState {
            public:
                State(IParser& p, StatePtr state_function_);
                ISubParser& getParser(IParser& p) override;
                IObjectPtr getObject() override;

                StatePtr stateFunc;
                
            };               

            ISubParser& operator()(IParser& p) override {
                auto& s = ISubParserState::cast(this, *p.getStateStack().top());
                return (static_cast<ParserT*>(this)->*(s.stateFunc))(s,p);
            }

            ISubParser& nextParser(IParser& p) noexcept {
                p.getLastState() = std::move(p.getStateStack().top());
                p.getStateStack().pop();
                return p.getStateStack().top()->getParser(p);
            }
        };

        template<typename Parser, typename ParserState>
        static ISubParser& StateTransition(Parser& current_parser, ParserState& state, IParser& parser) {
            throw typename Parser::Exception();
        }

        template<typename Parser, typename ParserState,
            typename Predicate, typename Action, typename NextState,
            typename... Predicates, typename... Actions, typename... NextStates>
            static ISubParser& StateTransition(Parser& current_parser, ParserState& state, IParser& parser,
                std::tuple<Predicate, Action, NextState>& p, std::tuple<Predicates, Actions, NextStates>&... ps) {
            
            static_assert(
                std::is_base_of<ISubParser, Parser>::value && 
                std::is_base_of<ISubParserState, ParserState>::value, "StateTransition not possible");

            if (std::get<0>(p)(parser.getCurrentChar())) {
                ISubParser& next_parser = std::get<1>(p)(current_parser, state, parser);
                state.stateFunc = std::get<2>(p);
                return next_parser;
            }
            else
                return StateTransition(current_parser, state, parser, ps...);

        }

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

        ISubParser& CheckStartSymbol(IParser& IParser) {
            throw InvalidStartingSymbol();
        }

        template<typename SubParserT, typename... SubParsersT>
        std::enable_if_t<
            !std::is_same<SubParserT,WSParser>::value &&
            std::is_pointer<
                decltype(std::declval<SubParserT>().getFirstSymbolSet())
            >::value, ISubParser&
        > CheckStartSymbol(IParser& parser, SubParserT& subparser, SubParsersT&... subparsers) {
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
        >CheckStartSymbol(IParser& parser, SubParserT& subparser, SubParsersT&... subparsers) {
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
        >CheckStartSymbol(IParser& parser, SubParserT& subparser, SubParsersT&... subparsers) {
            return CheckStartSymbol(parser, subparsers...);
        }


        template<typename... Parsers>
        ISubParser& DispatchFirstSymbol(IParser& p,std::tuple<Parsers...>& parsers) {
            return CheckStartSymbol(p, std::get<Parsers>(parsers)...);
        }

        template<typename JSONLiteral>
        class LiteralParser : public ParserTemplate<LiteralParser<JSONLiteral>> {
        public:
            using BaseState = typename ParserTemplate<LiteralParser<JSONLiteral>>::State;
            
            struct State : public BaseState  {
                State(IParser& p) : BaseState(p), current_pos{} {}
                IObjectPtr getObject() override {
                    return JSON::Create<JSONLiteral>();
                }
                size_t current_pos;
            };

            static char getFirstSymbolSet() {
                return GetLiteral<JSONLiteral>()[0];
            }
            
            static StatePtr getInitState() {
                return &LiteralParser::check;
            }

        private:
            ISubParser& check(ISubParserState& state, IParser& p) {
                auto& s = ISubParserState::cast(this,state);
                const size_t literal_size = strlen(GetLiteral<JSONLiteral>());
                if (GetLiteral<JSONLiteral>()[s.current_pos++] != p.getCurrentChar())
                    throw LiteralException();
                else if (s.current_pos == literal_size) {
                    return nextParser(p);
                }
                else
                    return *this;
            }

        };
        
        class NumberParser : public ParserTemplate<NumberParser> {
        public:
            struct Exception : impl::Exception {};

            struct State : public ParserTemplate<NumberParser>::State {
                using ParserTemplate<NumberParser>::State::State;

                IObjectPtr getObject() override {
                    return std::move(json_number);
                }

                IObjectPtr json_number;
                std::string number;
            };

            static const char* getFirstSymbolSet() {
                return "-0123456789";
            }

            static StatePtr getInitState() {
                return &NumberParser::start;
            }
            
        private:

            struct NoOp {
                ISubParser& operator()(NumberParser& current_parser,State& s, IParser&p) {
                    s.number.push_back(p.getCurrentChar());
                    return current_parser;
                }
            };

            struct Finished {
                ISubParser& operator()(NumberParser& current_parser,State& s, IParser& p) {
                    s.json_number = JSON::Create<Number>(std::move(s.number));
                    return current_parser.nextParser(p);
                }
            };

            ISubParser& start(ISubParserState& state, IParser& p) {
                return StateTransition(*this, ISubParserState::cast(this,state), p,
                    std::make_tuple(IsLiteral<'0'>{}, NoOp{},&NumberParser::startingZero),
                    std::make_tuple(std::isdigit, NoOp{}, &NumberParser::integerPart),
                    std::make_tuple(IsLiteral<'-'> {}, NoOp{}, &NumberParser::minus)
                );
            }

            ISubParser& minus(ISubParserState& state, IParser& p) {
                return StateTransition(*this, ISubParserState::cast(this, state), p,
                    std::make_tuple(IsLiteral<'0'>{}, NoOp{}, &NumberParser::startingZero),
                    std::make_tuple(std::isdigit, NoOp{}, &NumberParser::integerPart)
                    );
            }

            ISubParser& startingZero(ISubParserState& state, IParser& p) {
                return StateTransition(*this, ISubParserState::cast(this, state), p,
                    std::make_tuple(IsLiteral<'.'>{}, NoOp{}, &NumberParser::fractionPartStart),
                    std::make_tuple(IsWhitespace, Finished{}, nullptr)
                    );
            }

            ISubParser& integerPart(ISubParserState& state, IParser& p) {
                return StateTransition(*this, ISubParserState::cast(this, state), p,
                    std::make_tuple(std::isdigit, NoOp{}, &NumberParser::integerPart),
                    std::make_tuple(IsLiteral<'.'>{}, NoOp{}, &NumberParser::fractionPartStart),
                    std::make_tuple(IsLiteral<'e', 'E'>{}, NoOp{}, &NumberParser::exponentPartStart),
                    std::make_tuple(IsWhitespace, Finished{}, nullptr)
                    );
            }

            ISubParser& fractionPartStart(ISubParserState& state, IParser& p) {
                return StateTransition(*this, ISubParserState::cast(this, state), p,
                    std::make_tuple(std::isdigit, NoOp{}, &NumberParser::fractionPart)
                    );
            }

            ISubParser& fractionPart(ISubParserState& state, IParser& p) {
                return StateTransition(*this, ISubParserState::cast(this, state), p,
                    std::make_tuple(IsWhitespace, Finished{}, nullptr),
                    std::make_tuple(IsLiteral<'e', 'E'>{}, NoOp{}, &NumberParser::exponentPartStart),
                    std::make_tuple(std::isdigit, NoOp{}, &NumberParser::fractionPart)
                    );
            }

            ISubParser& exponentPartStart(ISubParserState& state, IParser& p) {
                return StateTransition(*this, ISubParserState::cast(this, state), p,
                    std::make_tuple(IsLiteral<'-', '+'>{}, NoOp{}, &NumberParser::exponentPartStartSigned),
                    std::make_tuple(std::isdigit, NoOp{}, &NumberParser::exponentPart)                    
                    );
            }

            ISubParser& exponentPartStartSigned(ISubParserState& state, IParser& p) {
                return StateTransition(*this, ISubParserState::cast(this, state), p,
                    std::make_tuple(std::isdigit, NoOp{}, &NumberParser::exponentPart)
                    );
            }

            ISubParser& exponentPart(ISubParserState& state, IParser& p) {
                return StateTransition(*this, ISubParserState::cast(this, state), p,
                    std::make_tuple(IsWhitespace, Finished{}, nullptr),
                    std::make_tuple(std::isdigit, NoOp{}, &NumberParser::exponentPart)
                    );
            }

        };

        class ObjectParser : public ParserTemplate<ObjectParser> {
        public:

            struct Exception : impl::Exception {};

            struct State : public ParserTemplate<ObjectParser>::State {
                State(IParser& p, StatePtr state_function_ = getInitState()) :
                    ParserTemplate<ObjectParser>::State(p, state_function_),
                    object{JSON::Create<Object>()}
                {}
                
                IObjectPtr getObject() override {
                    return std::move(object);
                }

                IObjectPtr object;
                std::string currentKey;
                IObjectPtr currentValue;
            };

            static char getFirstSymbolSet() {
                return begin_object;
            }

            static StatePtr getInitState() {
                return &ObjectParser::parseKey;
            }

        private:
            ISubParser& parseKey(ISubParserState& s, IParser& p) {
                return *this;
            }

            ISubParser& parseSeparator(ISubParserState& s, IParser& p) {
                return *this;
            }

            ISubParser& parseValue(ISubParserState& s, IParser& p) {
                return *this;
            }

        };



        class WSParser : public ParserTemplate<WSParser> {
        public:
            struct State : public ParserTemplate<WSParser>::State {
                using ParserTemplate<WSParser>::State::State;
            };

            static const char* getFirstSymbolSet() {
                return " \t\n\r";
            }

            static StatePtr getInitState() {
                return &WSParser::emplaceAndDispatch;
            }

        private:
            ISubParser& emplaceAndDispatch(ISubParserState& state, IParser& p) {
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
        };

        ////////////////////////////////////////////////////////////////////
        template<typename ParserT>
        inline ISubParser& ParserTemplate<ParserT>::operator()(IParser& p) {
            auto& s = ISubParserState::cast(this, *p.getStateStack().top());
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
        inline IObjectPtr ISubParser& ParserTemplate<ParserT>::State::getObject() {
            return IObjectPtr{ nullptr };
        }

        template<typename ParserT>
        inline ParserTemplate<ParserT>::State::State(IParser& p, StatePtr state_function_ = ParserT::getInitState()) :
            stateFunc{ state_function_ }
        {}


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