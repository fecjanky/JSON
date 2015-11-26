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
        
        using parsers_t = std::tuple<WSParser, NullParser, TrueParser, FalseParser, ObjectParser, ArrayParser, NumberParser, StringParser>;
        
        using IClosureObjectPtr = std::unique_ptr<IClosureObject>;
        using obj_container_t = std::vector<IObjectPtr>;
        using closure_stack_t = std::stack<IClosureObjectPtr>;
        using object_stack_t = std::stack<IObjectPtr>;

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

        struct IParser {
            struct Exception : public std::exception {};
            virtual char getCurrentChar() const noexcept = 0;
            virtual obj_container_t& getObjects() noexcept = 0;
            virtual closure_stack_t& getClosureStack() noexcept = 0;
            virtual  parsers_t& getParsers() noexcept = 0;
            virtual  object_stack_t& getObjectStack() noexcept = 0;
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
           
            using state_t = ISubParser* (ParserT::*)(IParser& p);

            explicit StatefulParser(state_t init_state) : state(init_state){}

            ISubParser* operator()(IParser& p) override {
                return (static_cast<ParserT*>(this)->*state)(p);
            }

        protected:
            state_t state;
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

        class ObjectParser : public ISubParser {
        public:
            ISubParser* operator()(IParser& p) override {
                return this;
            }

            static char get_starting_symbol() {
                return begin_object;
            }

        private:
        };

        class ArrayParser : public ISubParser {
        public:
            //constexpr static auto starting_symbols = utils::make_array('[');

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

        using obj_container_t = impl::obj_container_t;

        Parser() : current_char{}, parsers{}, current_parser(&std::get<impl::WSParser>(parsers)){}

        impl::obj_container_t retrieveObjects() {
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

        impl::obj_container_t& getObjects() noexcept override {
            return objects;
        }

        impl::closure_stack_t& getClosureStack() noexcept override {
            return closure_stack;
        }

        impl::parsers_t& getParsers() noexcept override {
            return parsers;
        }

        impl::object_stack_t& getObjectStack() noexcept override {
            return object_stack;
        }

        char current_char;
        impl::obj_container_t objects;
        impl::object_stack_t object_stack;
        impl::closure_stack_t closure_stack;
        impl::parsers_t parsers;
        impl::ISubParser* current_parser;
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