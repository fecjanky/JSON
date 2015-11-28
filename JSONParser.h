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

    namespace impl {
        
        
        struct IParser;
        struct ISubParser;
        struct ISubParserState;
        struct IClosureObject;
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
        
        //using ParserTuple = std::tuple<WSParser, NullParser, TrueParser, FalseParser, ObjectParser, ArrayParser, NumberParser, StringParser>;
        using ParserTuple = std::tuple<WSParser, NullParser, TrueParser, FalseParser>;
        
        using IClosureObjectPtr = std::unique_ptr<IClosureObject>;
        using ISubParserStatePtr = std::unique_ptr<ISubParserState>;
        using ObjContainer = std::vector<IObjectPtr>;
        using ClosureStack = std::stack<IClosureObjectPtr>;
        using ObjectStack = std::stack<IObjectPtr>;
        using ParserStateStack = std::stack<ISubParserStatePtr>;

        struct Exception : public std::exception {};
        struct InvalidStartingSymbol : public Exception {};
        struct LiteralException : public Exception {};
        struct ParsingIncomplete : public Exception {};

        template<typename JSONLiteral>
        const char* GetLiteral() { static_assert(0, "Type is not literal type"); }

        template<> const char* GetLiteral<Null>() { return value_null; }
        template<> const char* GetLiteral<True>() { return value_true; }
        template<> const char* GetLiteral<False>() { return value_false; }


        inline bool IsWhitespace(char c) {
            for (auto w : ws) if (c == w) return true;
            return false;
        }

        struct ISubParser {
            struct Exception : public std::exception {};
            virtual ISubParser& operator()(IParser& p) = 0;
            virtual ~ISubParser() = default;
        };

        struct ISubParserState {
            virtual ISubParser& getParser(IParser& p) = 0;
            virtual IObjectPtr getObject() = 0;
            virtual ~ISubParserState() = default;
        };

        template<typename SubParserT>
        std::enable_if_t<std::is_base_of<ISubParser,SubParserT>::value
            ,ISubParserStatePtr
        > CreateState(IParser& p) {
            return ISubParserStatePtr(new typename SubParserT::State{p});
        }

        struct IParser {
            struct Exception : public std::exception {};
            virtual char getCurrentChar() const noexcept = 0;
            virtual ObjContainer& getObjects() noexcept = 0;
            virtual  ParserTuple& getParsers() noexcept = 0;
            virtual ParserStateStack& getStateStack() noexcept = 0;
            virtual ISubParserStatePtr& getLastState() noexcept = 0;
            
            //virtual ClosureStack& getClosureStack() noexcept = 0;
            //virtual  ObjectStack& getObjectStack() noexcept = 0;
        protected:
            virtual ~IParser() = default;
        };

        struct IClosureObject {
            virtual ISubParser* select_parser(IParser&) = 0;
            virtual ~IClosureObject() = default;
        };

        template<typename ParserT>
        class ParserTemplate : public ISubParser{
        public:
            //using ParserStatePtr = typename ParserT::State *;
            using state_ptr_t = ISubParser& (ParserT::*)(ISubParserState&,IParser&);

            class State : public ISubParserState {
            public:
                State(IParser& p, state_ptr_t state_function_ = ParserT::getInitState()) :
                    state_function{ state_function_ }
                     {}

                ISubParser& getParser(IParser& p) override { 
                    return std::get<ParserT>(p.getParsers()); 
                }
                
                IObjectPtr getObject() override {
                    return IObjectPtr{nullptr};
                }

                state_ptr_t state_function;
                
            };               

            ParserTemplate() = default;

            ISubParser& operator()(IParser& p) override {
                auto& s = static_cast<State&>(*p.getStateStack().top());
                return (static_cast<ParserT*>(this)->*(s.state_function))(s,p);
            }

            ISubParser& next_parser(IParser& p) noexcept {
                return p.getStateStack().top()->getParser(p);
            }
        };

        ISubParser& CheckStartSymbol(IParser& IParser) {
            throw InvalidStartingSymbol();
        }

        template<typename SubParserT, typename... SubParsersT>
        std::enable_if_t<
            std::is_pointer<
                decltype(std::declval<SubParserT>().getFirstSymbolSet())
            >::value, ISubParser&
        > CheckStartSymbol(IParser& parser, SubParserT& subparser, SubParsersT&... subparsers) {
            std::string syms = subparser.getFirstSymbolSet();
            if (syms.find(parser.getCurrentChar()) == std::string::npos) 
                return CheckStartSymbol(parser, subparsers...);
            else {
                parser.getStateStack().push(CreateState<SubParserT>(parser));
                return subparser(parser);
            }
        }

        template<typename SubParserT, typename... SubParsersT>
        std::enable_if_t<
            !std::is_pointer<
                decltype(std::declval<SubParserT>().getFirstSymbolSet())
            >::value, ISubParser&
        >CheckStartSymbol(IParser& parser, SubParserT& subparser, SubParsersT&... subparsers) {
            if (parser.getCurrentChar() != subparser.getFirstSymbolSet())
                return CheckStartSymbol(parser,subparsers...);
            else {
                parser.getStateStack().push(CreateState<SubParserT>(parser));
                return subparser(parser);
            }
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
                    return Create<JSONLiteral>();
                }
                size_t current_pos;
            };

            static char getFirstSymbolSet() {
                return GetLiteral<JSONLiteral>()[0];
            }
            
            static state_ptr_t getInitState() {
                return &LiteralParser::check;
            }

        private:
            ISubParser& check(ISubParserState& state, IParser& p) {
                auto& s = static_cast<State&>(state);
                const size_t literal_size = strlen(GetLiteral<JSONLiteral>());
                if (GetLiteral<JSONLiteral>()[s.current_pos++] != p.getCurrentChar())
                    throw LiteralException();
                if (s.current_pos == literal_size) {
                    p.getLastState() = std::move(p.getStateStack().top());
                    p.getStateStack().pop();
                    return next_parser(p);
                }
                else
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

            static state_ptr_t getInitState() {
                return &WSParser::parseEmplaceAndDispatch;
            }
        private:
            ISubParser& parseEmplaceAndDispatch(ISubParserState& state, IParser& p) {
                auto& s = static_cast<State&>(state);
                
                if (p.getLastState())
                    p.getObjects().emplace_back(
                        std::move(p.getLastState()->getObject()));

                if (IsWhitespace(p.getCurrentChar()))
                    return *this;
                else {
                    return DispatchFirstSymbol(p, p.getParsers());
                }
            }
        };
        /*
        class NumberParser : public ParserTemplate<NumberParser> {
        public:
            struct Exception : public std::exception {};

            struct State : public ParserTemplate<NumberParser>::State {
                using ParserTemplate<NumberParser>::State::State;
                std::string number;
            };

            static const char* getFirstSymbolSet() {
                return "-0123456789";
            }

            static state_ptr_t getInitState() {
                return &NumberParser::start;
            }


        private:

            ISubParser& start(ISubParserState& state, IParser& p) {

                char c = p.getCurrentChar();
                if (c == '0') state = &NumberParser::starting_zero;
                else if (std::isdigit(c)) state = &NumberParser::integer_part;
                else if (c == '-')  state = &NumberParser::minus;
                else throw Exception();
                token.push_back(c);
                return this;
            }

            ISubParser* minus(IParser& p) {
                char c = p.getCurrentChar();
                if (c == '0') state = &NumberParser::starting_zero;
                else if (std::isdigit(c)) state = &NumberParser::integer_part;
                else throw Exception();
                token.push_back(c);
                return this;
            }

            ISubParser& starting_zero(ISubParserState& state, IParser& p) {
                char c = p.getCurrentChar();
                if (c == '.') state = &NumberParser::fraction_part;
                else if (IsWhitespace(c)) return end(p);
                else throw Exception();
                token.push_back(c);
                return this;
            }

            ISubParser* integer_part(IParser& p) {
                char c = p.getCurrentChar();
                if (std::isdigit(c)) state = &NumberParser::integer_part;
                else if (c == '.') state = &NumberParser::fraction_part;
                else if (c == 'e' || c == 'E') state = &NumberParser::exponent_part;
                else if (IsWhitespace(c)) return end(p);
                else throw Exception();
                token.push_back(c);
                return this;
            }

            ISubParser* fraction_part(IParser& p) {
                return this;
            }

            ISubParser* exponent_part(IParser& p) {
                return this;
            }

            ISubParser* end(IParser& p) {
                state = &NumberParser::init_state;
                if (p.getClosureStack().empty()) {
                    p.getObjects().emplace_back(Create<Number>(std::move(token)));
                }
                else {

                }
                //TODO: get next state based on closure type
                return this;
            }

        };
        */
        /*
        class ObjectParser : public ParserTemplate<ObjectParser> {
        public:

            ObjectParser() : ParserTemplate<ObjectParser>(&ObjectParser::init) {}

            struct State : public ParserTemplate<ObjectParser>::State {
                std::string name;
                IObjectPtr object;
            };

            static char getFirstSymbolSet() {
                return begin_object;
            }

        private:
            ISubParser* init(IParser& p) {
                return this;
            }

        };

        class ArrayParser : public ParserTemplate<ArrayParser> {
        public:

            struct State : public ParserTemplate<ObjectParser>::State {};

            static char getFirstSymbolSet() {
                return begin_array;
            }

            static state_ptr_t getInitState() {
                return &ArrayParser::start;
            }

        private:
            ISubParser* start(State& s, IParser& p) {
                return this;
            }
        };

        class NumberParser : public ParserTemplate<NumberParser> {
        public:
            //constexpr static auto starting_symbols = utils::make_array('-','0','1','2','3','4','5','6','7','8','9' );
            struct Exception : public std::exception {};

            NumberParser() : ParserTemplate<NumberParser>(&NumberParser::init_state) {}

            static const char* getFirstSymbolSet() {
                return "-0123456789";
            }

        private:
            std::string token;

            ISubParser* init_state(IParser& p) {
                char c = p.getCurrentChar();
                if (c == '0') state = &NumberParser::starting_zero;
                else if(std::isdigit(c)) state = &NumberParser::integer_part;
                else if (c == '-')  state = &NumberParser::minus;
                else throw Exception();
                token.push_back(c);
                return this;
            }

            ISubParser* minus(IParser& p) {
                char c = p.getCurrentChar();
                if (c == '0') state = &NumberParser::starting_zero;
                else if (std::isdigit(c)) state = &NumberParser::integer_part;
                else throw Exception();                
                token.push_back(c);
                return this;
            }

            ISubParser* starting_zero(IParser& p) {
                char c = p.getCurrentChar();
                if (c == '.') state = &NumberParser::fraction_part;
                else if (IsWhitespace(c)) return end(p);
                else throw Exception();
                token.push_back(c);
                return this;
            }

            ISubParser* integer_part(IParser& p) {
                char c = p.getCurrentChar();
                if (std::isdigit(c)) state = &NumberParser::integer_part;
                else if (c == '.') state = &NumberParser::fraction_part;
                else if (c == 'e' || c == 'E') state = &NumberParser::exponent_part;
                else if (IsWhitespace(c)) return end(p);
                else throw Exception();
                token.push_back(c);
                return this;
            }

            ISubParser* fraction_part(IParser& p) {
                return this;
            }

            ISubParser* exponent_part(IParser& p) {
                return this;
            }

            ISubParser* end(IParser& p) {
                state = &NumberParser::init_state;
                if (p.getClosureStack().empty()) {
                    p.getObjects().emplace_back(Create<Number>(std::move(token)));
                }
                else {

                }
                //TODO: get next state based on closure type
                return this;
            }
            
        };



        class StringParser : public ISubParser {
        public:

            ISubParser* operator()(IParser& p) override {
                return this;
            }

            static char getFirstSymbolSet() {
                return quotation_mark;
            }
        private:
        };
        */

        /*
        struct ClosureObject : public Object, public IClosureObject {
            ISubParser* select_parser(IParser& p) override {
                return &(std::get<ObjectParser>(p.getParsers()));
            }
        };

        struct ClosureArray : public Array, public IClosureObject {
            ISubParser* select_parser(IParser& p) override {
                return &(std::get<ArrayParser>(p.getParsers()));
            }
        };
        */

    }

    class Parser : public impl::IParser {
    public:

        using obj_container_t = impl::ObjContainer;

        Parser() : current_char{} 
        {
            stateStack.push(
                impl::ISubParserStatePtr(
                    impl::CreateState<impl::WSParser>(*this)));
        }

        impl::ObjContainer retrieveObjects() {
            if (!isRetrievable()) throw impl::ParsingIncomplete();
            return std::move(objects);
        }

        bool isRetrievable()const noexcept {
            return stateStack.size() == 1;
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

        impl::ObjContainer& getObjects() noexcept override { return objects; }

        impl::ParserTuple& getParsers() noexcept override { return parsers; }
        
        impl::ParserStateStack& getStateStack() noexcept override {
            return stateStack;
        }
        
        impl::ISubParserStatePtr& getLastState() noexcept override {
            return lastState;
        }
        /*

        impl::ClosureStack& getClosureStack() noexcept override {
            return closure_stack;
        }



        impl::ObjectStack& getObjectStack() noexcept override {
            return object_stack;
        }
        */



        char current_char;
        impl::ObjContainer objects;
        impl::ParserTuple parsers;
        impl::ParserStateStack stateStack;
        impl::ISubParserStatePtr lastState;

        //impl::ObjectStack object_stack;
        //impl::ClosureStack closure_stack;
        //impl::ISubParser* current_parser;
    };
    
    template<typename ForwardIterator>
    Parser::obj_container_t parse(ForwardIterator start, ForwardIterator end) {
        Parser p;

        for (; start != end; ++start) {
            p(*start);
        }

        return p.retrieveObjects();
    };


}

#endif //JSONPARSER_H_INCLUDED__