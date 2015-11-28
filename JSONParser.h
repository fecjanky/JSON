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
        
        using ParserTuple = std::tuple<WSParser, NullParser, TrueParser, FalseParser, ObjectParser, ArrayParser, NumberParser, StringParser>;
        
        using IClosureObjectPtr = std::unique_ptr<IClosureObject>;
        using ObjContainer = std::vector<IObjectPtr>;
        using ClosureStack = std::stack<IClosureObjectPtr>;
        using ObjectStack = std::stack<IObjectPtr>;
        using ParserStateStack = std::stack<std::unique_ptr<ISubParserState>>;

        struct Exception : public std::exception {};
        struct InvalidStartingSymbol : public Exception {};
        struct LiteralException : public Exception {};
        struct ParsingIncomplete : public Exception {};

        template<typename JSONLiteral>
        const char* getLiteral() { static_assert(0, "Type is not literal type"); }

        template<> const char* getLiteral<Null>() { return value_null; }
        template<> const char* getLiteral<True>() { return value_true; }
        template<> const char* getLiteral<False>() { return value_false; }


        inline bool is_ws(char c) {
            for (auto w : ws) if (c == w) return true;
            return false;
        }

        struct ISubParser {
            struct Exception : public std::exception {};
            virtual ISubParser* operator()(IParser& p) = 0;
            virtual ~ISubParser() = default;
        };

        struct ISubParserState {
            virtual ISubParser* getNextParser() = 0;
            virtual ~ISubParserState() = default;
        };

        struct IParser {
            struct Exception : public std::exception {};
            virtual char getCurrentChar() const noexcept = 0;
            virtual ObjContainer& getObjects() noexcept = 0;
            virtual ClosureStack& getClosureStack() noexcept = 0;
            virtual  ParserTuple& getParsers() noexcept = 0;
            virtual  ObjectStack& getObjectStack() noexcept = 0;
            virtual ParserStateStack& getParserStateStack() noexcept = 0;
        protected:
            virtual ~IParser() = default;
        };

        struct IClosureObject {
            virtual ISubParser* select_parser(IParser&) = 0;
            virtual ~IClosureObject() = default;
        };

        template<typename ParserT>
        class StatefulParser : public ISubParser{
        public:
            using state_ptr_t = ISubParser* (ParserT::*)(IParser& p);
            
            class State : public ISubParserState {
            public:
                State(IParser& p) : next_parser{ &std::get<ParserT>(p.getParsers()) } {}
                ISubParser* getNextParser() override {
                    return next_parser;
                }
            private:
                ISubParser* next_parser;
            };               

            explicit StatefulParser(state_ptr_t init_state) : state(init_state){}

            ISubParser* operator()(IParser& p) override {
                return (static_cast<ParserT*>(this)->*state)(p);
            }

        protected:
            state_ptr_t state;
        };


        ISubParser* check_start_symbol(IParser& IParser) {
            throw InvalidStartingSymbol();
        }

        template<typename SubParserT, typename... SubParsersT>
        std::enable_if_t<std::is_pointer<decltype(std::declval<SubParserT>().get_starting_symbol())>::value, ISubParser*>
            check_start_symbol(IParser& parser, SubParserT& subparser, SubParsersT&... subparsers) {
            std::string syms = subparser.get_starting_symbol();
            if (syms.find(parser.getCurrentChar()) == std::string::npos) 
                return check_start_symbol(parser, subparsers...);
            else
                return subparser(parser);
        }

        template<typename SubParserT, typename... SubParsersT>
        std::enable_if_t<!std::is_pointer<decltype(std::declval<SubParserT>().get_starting_symbol())>::value, ISubParser*>
            check_start_symbol(IParser& parser, SubParserT& subparser, SubParsersT&... subparsers) {
            if (parser.getCurrentChar() != subparser.get_starting_symbol())
                return check_start_symbol(parser,subparsers...);
            else
                return subparser(parser);
        }


        template<typename... Parsers>
        ISubParser* dispatch_first_symbol(IParser& p,std::tuple<Parsers...>& parsers) {
            return check_start_symbol(p, std::get<Parsers>(parsers)...);
        }

        template<typename JSONLiteral>
        class LiteralParser : public StatefulParser<LiteralParser<JSONLiteral>> {
        public:
            LiteralParser() : 
                StatefulParser<LiteralParser<JSONLiteral>>(&LiteralParser<JSONLiteral>::check), 
                current_pos{}, 
                literal_size{ strlen(getLiteral<JSONLiteral>()) }{}
            static char get_starting_symbol() {
                return getLiteral<JSONLiteral>()[0];
            }
        private:
            size_t current_pos;
            const size_t literal_size;

            ISubParser* check(IParser& p) {
                if (getLiteral<JSONLiteral>()[current_pos++] != p.getCurrentChar())
                    throw LiteralException();
                if (current_pos == literal_size) {
                    current_pos = 0;
                    if (p.getClosureStack().empty()) {
                        p.getObjects().emplace_back(Create<JSONLiteral>(getLiteral<JSONLiteral>()));
                        return &std::get<WSParser>(p.getParsers());
                    }
                    else {
                        p.getObjectStack().emplace(Create<JSONLiteral>(getLiteral<JSONLiteral>()));
                        return p.getClosureStack().top()->select_parser(p);
                    }
                }
                else
                    return this;
            }

        };

        class ObjectParser : public StatefulParser<ObjectParser> {
        public:

            ObjectParser() : StatefulParser<ObjectParser>(&ObjectParser::init) {}

            struct State : public StatefulParser<ObjectParser>::State {
                std::string name;
                IObjectPtr object;
            };

            static char get_starting_symbol() {
                return begin_object;
            }

        private:
            ISubParser* init(IParser& p) {
                return this;
            }

        };

        class ArrayParser : public ISubParser {
        public:

            ISubParser* operator()(IParser& p) override {
                return this;
            }

            static char get_starting_symbol() {
                return begin_array;
            }

        private:
        };

        class NumberParser : public StatefulParser<NumberParser> {
        public:
            //constexpr static auto starting_symbols = utils::make_array('-','0','1','2','3','4','5','6','7','8','9' );
            struct Exception : public std::exception {};

            NumberParser() : StatefulParser<NumberParser>(&NumberParser::init_state) {}

            static const char* get_starting_symbol() {
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
                else if (is_ws(c)) return end(p);
                else throw Exception();
                token.push_back(c);
                return this;
            }

            ISubParser* integer_part(IParser& p) {
                char c = p.getCurrentChar();
                if (std::isdigit(c)) state = &NumberParser::integer_part;
                else if (c == '.') state = &NumberParser::fraction_part;
                else if (c == 'e' || c == 'E') state = &NumberParser::exponent_part;
                else if (is_ws(c)) return end(p);
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

            static char get_starting_symbol() {
                return quotation_mark;
            }
        private:
        };

        class WSParser : public ISubParser {
        public:
            ISubParser* operator()(IParser& p) override {
                if (is_ws(p.getCurrentChar()))
                    return this;
                else {
                    return dispatch_first_symbol(p,p.getParsers());
                }
            }
            
            static const char* get_starting_symbol() {
                return " \t\n\r";
            }

        private:
        };

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

    }

    class Parser : public impl::IParser {
    public:

        using obj_container_t = impl::ObjContainer;

        Parser() : current_char{}, parsers{}, current_parser(&std::get<impl::WSParser>(parsers)){}

        impl::ObjContainer retrieveObjects() {
            if (!isRetrievable()) throw impl::ParsingIncomplete();
            return std::move(objects);
        }

        bool isRetrievable()const noexcept {
            return object_stack.empty() && closure_stack.empty();
        }

        void operator()(char c) {
            current_char = c;
            current_parser = (*current_parser)(*this);
        }

        Parser(const Parser&) = delete;
        Parser& operator=(const Parser&) = delete;
        Parser(Parser&&) = delete;
        Parser& operator=(Parser&&) = delete;

    private:
        char getCurrentChar() const noexcept override {
            return current_char;
        }

        impl::ObjContainer& getObjects() noexcept override {
            return objects;
        }

        impl::ClosureStack& getClosureStack() noexcept override {
            return closure_stack;
        }

        impl::ParserTuple& getParsers() noexcept override {
            return parsers;
        }

        impl::ObjectStack& getObjectStack() noexcept override {
            return object_stack;
        }

        impl::ParserStateStack& getParserStateStack() noexcept override {
            return state_stack;
        }

        char current_char;
        impl::ObjContainer objects;
        impl::ObjectStack object_stack;
        impl::ClosureStack closure_stack;
        impl::ParserTuple parsers;
        impl::ISubParser* current_parser;
        impl::ParserStateStack state_stack;
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