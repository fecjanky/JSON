#ifndef JSON_H_INCLUDED__
#define JSON_H_INCLUDED__

#include <exception>
#include <string>
#include <sstream>
#include <utility>
#include <unordered_map>
#include <memory>
#include <vector>
#include <type_traits>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <cstddef>
#include <stack>
#include <functional>
#include <array>
#include <tuple>

/*
#if __cplusplus  < 201402L
namespace std{
    template<bool Val,typename T = void>
    using enable_if_t = typename std::enable_if<Val,T>::type;

    template<typename T>
    using decay_t = typename std::decay<T>::type;
}
#endif
*/

namespace JSON {

    namespace utils {
        template<typename Arg,typename... Args>
        constexpr std::array<Arg, sizeof...(Args)+1> make_array(Arg a, Args... args) {
            return std::array<Arg, sizeof...(Args)+1>{a, args...};
        }
    }

    namespace RFC7159 {

        // JSON grammar
        // ============
        // JSON-text = ws value ws
        // value = false / null / true / object / array / number / string
        // object = begin - object[member *(value - separator member)]
        //    end - object
        // member = string name - separator value
        // array = begin-array [ value *( value-separator value ) ] end-array
        // number = [ minus ] int [ frac ] [ exp ]
        // decimal-point = %x2E       ; .
        // digit1-9 = %x31-39         ; 1-9
        // e = %x65 / %x45            ; e E
        // exp = e [ minus / plus ] 1*DIGIT
        // frac = decimal-point 1*DIGIT
        // int = zero / ( digit1-9 *DIGIT )
        // minus = %x2D               ; -
        // plus = %x2B                ; +
        // zero = %x30                ; 0
        //string = quotation-mark *char quotation-mark
        //char = unescaped /
        //    escape (
        //        %x22 /          ; "    quotation mark  U+0022
        //        %x5C /          ; \    reverse solidus U+005C
        //        %x2F /          ; /    solidus         U+002F
        //        %x62 /          ; b    backspace       U+0008
        //        %x66 /          ; f    form feed       U+000C
        //        %x6E /          ; n    line feed       U+000A
        //        %x72 /          ; r    carriage return U+000D
        //        %x74 /          ; t    tab             U+0009
        //        %x75 4HEXDIG )  ; uXXXX                U+XXXX
        //escape = %x5C              ; \ //
        //quotation-mark = %x22      ; "
        //unescaped = %x20-21 / %x23-5B / %x5D-10FFFF

        constexpr char  begin_array = 0x5B; // [ left square bracket
        constexpr char  begin_object = 0x7B; // { left curly bracket
        constexpr char  end_array = 0x5D; // ] right square bracket
        constexpr char  end_object = 0x7D; // } right curly bracket
        constexpr char  name_separator = 0x3A; // : colon
        constexpr char  value_separator = 0x2C; // , comma
        constexpr char ws[] = {
                0x20, //  Space
                0x09, //  Horizontal tab
                0x0A, //  Line feed or New line
                0x0D, //  Carriage return
        };
        constexpr char  value_false[] = "false"; //false
        constexpr char  value_true[] = "true"; //true
        constexpr char  value_null[] = "null"; //null
        
        constexpr char quotation_mark = 0x22; // "quotation-mark = %x22      ; "

    }
    
    using namespace RFC7159;

    struct Exception : public std::exception {};
    struct AttributeMissing : JSON::Exception {};
    struct TypeError : public JSON::Exception {};
    struct AggregateTypeError : public JSON::Exception {};
    struct ValueError : public JSON::Exception {};
    struct OutOfRange : public JSON::Exception {};
    struct AttributeNotUnique : public JSON::Exception {};

    // TODO: introduce Mutable and Immutable objects

    struct IObject;
    struct IAggregateObject;

    using IObjectPtr = std::unique_ptr<IObject>;
    using IAggregateObjectPtr = std::unique_ptr<IAggregateObject>;

    struct IObject {

        virtual IObject& operator[](const std::string& key) = 0;
        virtual const IObject& operator[](const std::string& key) const = 0;

        virtual IObject& operator[](size_t index) = 0;
        virtual const IObject& operator[](size_t index) const = 0;

        virtual const std::string& getValue() const = 0;

        operator std::string () const {
            return getValue();
        }

        virtual ~IObject() = default;
    };

    struct IAggregateObject {
        // for arrays
        virtual void emplace(IObjectPtr&& obj) = 0;
        // for objects
        virtual void emplace(const std::string&& name,IObjectPtr&& obj) = 0;

        virtual ~IAggregateObject() = default;
    };

    class Object : public IObject, public IAggregateObject {
    public:

        using Key = std::string;
        using Value = IObjectPtr;
        using type = std::unordered_map<Key, Value>;
        using Entry = std::pair<const Key, IObjectPtr>;

        Object(){}

        template<typename... EntryT>
        Object(EntryT&&... list) {
            emplace_entries(std::forward<EntryT>(list)...);
        }

        IObject& operator[](const std::string& key) override {
            auto obj = values.find(key);
            if (obj == values.end())
                throw AttributeMissing();
            return *obj->second;
        }

        const IObject& operator[](const std::string& key) const override {
            auto obj = values.find(key);
            if(obj == values.end())
                throw AttributeMissing();
            return *obj->second;
        }

        IObject& operator[](size_t index) override {
            throw TypeError();
        }

        const IObject& operator[](size_t index) const override {
            throw TypeError();
        }

        virtual const std::string& getValue() const override {
            throw TypeError();
        }

        const type& getValues() const {
            return values;
        }

        template<typename... Entries>
        void emplace_entries(Entries&&... entries) {
            check_insert(std::forward<Entries>(entries)...);
            values.reserve(sizeof...(Entries));
            emplace_impl(std::forward<Entries>(entries)...);
        }

        void emplace(IObjectPtr&& obj) override{
            throw AggregateTypeError();
        }

        void emplace(const std::string&& name,IObjectPtr&& obj) override {
            emplace_entries(Entry(name,std::move(obj)));
        }

    private:

        void emplace_impl() {}
        void check_insert() {}

        template<typename FirstEntry, typename... Entries>
        std::enable_if_t<std::is_same<FirstEntry, Entry>::value> check_insert(FirstEntry&& e, Entries&&... rest) {
            if (values.find(e.first) != values.end())throw AttributeNotUnique();
            check_insert(std::forward<Entries>(rest)...);
        }

        template<typename FirstEntry, typename... Entries>
        std::enable_if_t<std::is_same<FirstEntry, Entry>::value> emplace_impl(FirstEntry&& e, Entries&&... rest) {
            values.emplace(e.first, std::move(e.second));
            emplace_impl(std::forward<Entries>(rest)...);
        }

        type values;
    };

    class Array : public IObject, public IAggregateObject {
    public:

        using Value = IObjectPtr;
        using type = std::vector<Value>;

        Array(){}

        template<typename... ObjT>
        Array(std::unique_ptr<ObjT>&&... objs) {
            values.reserve(sizeof...(ObjT));
            emplace_impl(std::move(objs)...);
        }


        IObject& operator[](const std::string& key) override {
            throw TypeError();
        }

        const IObject& operator[](const std::string& key) const override {
            throw TypeError();
        }

        IObject& operator[](size_t index) override {
            if (index >= values.size())
                throw OutOfRange();
            return *values[index];
        }

        const IObject& operator[](size_t index) const override {
            if (index >= values.size())
                throw OutOfRange();
            return *values[index];
        }

        virtual const std::string& getValue() const override {
            throw TypeError();
        }

        type& getValues() {
            return values;
        }

        const type& getValues() const {
            return values;
        }

        void emplace(IObjectPtr&& obj) override{
            values.emplace_back(std::move(obj));
        }

        void emplace(const std::string&& name,IObjectPtr&& obj) override {
            throw AggregateTypeError();
        }

    private:
        void emplace_impl() {}
        template<typename FirstObj, typename... Objs>
        std::enable_if_t<std::is_same<FirstObj, Value>::value> emplace_impl(FirstObj&& f, Objs&&... rest) {
            values.emplace_back(std::move(f));
            emplace_impl(std::forward<Objs>(rest)...);
        }

        type values;
    };

    namespace impl {
        template<typename T>
        struct Validator {
            const std::string& operator()(const std::string& rep) const {
                validate(rep);
                return rep;
            }

            std::string&& operator()(std::string&& rep) const {
                validate(rep);
                return std::move(rep);
            }
        private:
            void validate(const std::string& rep) const {
                T temp;
                std::stringstream ss(rep);
                ss << std::scientific;
                ss >> temp;
                if (ss.bad() || ss.fail() || ss.peek() != EOF)
                    throw ValueError();
            }
        };

        template<>
        struct Validator<nullptr_t> {
            const std::string& operator()(const std::string& rep) const {
                validate(rep);
                return rep;
            }

            std::string&& operator()(std::string&& rep) const {
                validate(rep);
                return std::move(rep);
            }
        private:
            void validate(const std::string& rep) const {
                if (rep != std::string(value_null))
                    throw ValueError();
            }
        };

        template<>
        struct Validator<bool> {
            const std::string& operator()(const std::string& rep) const {
                validate(rep);
                return rep;
            }

            std::string&& operator()(std::string&& rep) const {
                validate(rep);
                return std::move(rep);
            }
        private:
            void validate(const std::string& rep) const {
                if (rep != std::string(value_false) &&
                    rep != std::string(value_true))
                    throw ValueError();
            }
        };

        inline std::string to_string(bool b) {
            if (b)
                return std::string(value_true);
            else
                return std::string(value_false);
        }
    }

    class BuiltIn : public IObject {
    public:
        IObject& operator[](const std::string& key) override {
            throw TypeError();
        }

        const IObject& operator[](const std::string& key) const override {
            throw TypeError();
        }

        IObject& operator[](size_t index) override {
            throw TypeError();
        }

        const IObject& operator[](size_t index) const override {
            throw TypeError();
        }

        virtual const std::string& getValue() const override {
            return value;
        }
    protected:
        BuiltIn(const std::string& s) : value{ s } {}
        BuiltIn(std::string&& s) : value{ std::move(s) } {}
    private:
        std::string value;
    };

    class Bool : public BuiltIn {
    public:
        explicit Bool(bool b = false) : BuiltIn(impl::to_string(b)) {}
        explicit Bool(const std::string& s) : BuiltIn(impl::Validator<bool>{}(s)){}
        explicit Bool(std::string&& s) : BuiltIn(impl::Validator<bool>{}(std::move(s))) {}
        explicit Bool(const char * s) : Bool(std::string(s)) {}
    };

    class True : public Bool {
    public:
        True() : Bool(value_true) {}
        explicit True(const std::string& s) : Bool(s) {}
        explicit True(std::string&& s) : Bool(s) {}
    };

    class False : public Bool {
    public:
        False() : Bool(value_false) {};
        explicit False(const std::string& s) : Bool(s) {}
        explicit False(std::string&& s) : Bool(s) {}

    };


    class String : public BuiltIn {
    public:
        String() : BuiltIn(std::string{}) {}
        explicit String(const std::string& t) : BuiltIn(t) {}
        explicit String(std::string&& t) : BuiltIn(std::move(t)) {}
    };

    class Number : public BuiltIn {
    public:
        explicit Number(double d = 0.0) : BuiltIn(std::to_string(d)) {}
        explicit Number(const std::string& s) : BuiltIn(impl::Validator<double>{}(s)){}
        explicit Number(std::string&& s) : BuiltIn(impl::Validator<double>{}(std::move(s))) {}
    };

    class Null : public BuiltIn {
    public:
        explicit Null() : BuiltIn(std::string(value_null)) {}
        explicit Null(const std::string& s) : BuiltIn(impl::Validator<nullptr_t>{}(s)) {}
        explicit Null(std::string&& s) : BuiltIn(impl::Validator<nullptr_t>{}(std::move(s))) {}
    };

    template<typename T> struct is_JSON_type : public std::false_type {};
    template<> struct is_JSON_type<Object> : public std::true_type {};
    template<> struct is_JSON_type<Array> : public std::true_type {};
    template<> struct is_JSON_type<BuiltIn> : public std::true_type {};
    template<> struct is_JSON_type<Bool> : public std::true_type {};
    template<> struct is_JSON_type<True> : public std::true_type {};
    template<> struct is_JSON_type<False> : public std::true_type {};
    template<> struct is_JSON_type<String> : public std::true_type {};
    template<> struct is_JSON_type<Number> : public std::true_type {};
    template<> struct is_JSON_type<Null> : public std::true_type {};

    template<typename T,typename... Args>
    std::enable_if_t<is_JSON_type<T>::value,IObjectPtr> Create(Args&&... args) {
        return IObjectPtr(new T(std::forward<Args>(args)...));
    }

}


#endif //JSON_H_INCLUDED__
