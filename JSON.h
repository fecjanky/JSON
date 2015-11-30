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
#include <functional>
#include <algorithm>

// TODO: Mutable and Immutable objects
// TODO: template objects, template param Char (for wchar_t and wstring support)
namespace JSON {

    namespace RFC7159 {

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
                0x0D  //  Carriage return
        };
        constexpr char string_escape = 0x5c;
        constexpr char string_unicode_escape = 0x75;

        constexpr char string_escapes[] = {
            0x22 , // "    quotation mark  U+0022
            0x5C , // \    reverse solidus U+005C
            0x2F , // /    solidus         U+002F
            0x62 , // b    backspace       U+0008
            0x66 , // f    form feed       U+000C
            0x6E , // n    line feed       U+000A
            0x72 , // r    carriage return U+000D
            0x74  // t    tab             U+0009
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

        virtual void serialize(std::string& indentation,std::ostream&) const = 0;

        virtual ~IObject() = default;
    };

    inline std::ostream& operator << (std::ostream& os, const IObject& obj) {
        obj.serialize(std::string{},os);
        return os;
    }

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

        void serialize(std::string& indentation, std::ostream& os) const override {
            os << begin_object << "\n";
            indentation.push_back(' ');
            indentation.push_back(' ');
            for (auto i = values.begin(); i != values.end();) {
                os << indentation << quotation_mark << i->first << quotation_mark
                    << " " << name_separator << " ";
                
                i->second->serialize(indentation,os);
                ++i;
                if (i != values.end()) os << " " << value_separator << "\n";
                else os << "\n";
                
            }
            indentation.pop_back();
            indentation.pop_back();
            os << indentation << end_object;
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

        void serialize(std::string& indentation,std::ostream& os) const  override {
            os << begin_array << " ";
            for (auto i = values.begin(); i != values.end();) {
                (*i)->serialize(indentation,os);
                ++i;
                if (i != values.end()) os << " " << value_separator << " ";
            }
            os << " " << end_array;
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

        void serialize(std::string&,std::ostream& os) const override {
            os << value;
        }

    protected:
        BuiltIn(const std::string& s) : value{ s } {}
        BuiltIn(std::string&& s) : value{ std::move(s) } {}
    private:
        std::string value;
    };

    class Bool : public BuiltIn {
    public:
        explicit Bool(bool b = false) : BuiltIn(impl::to_string(b)), nativeValue{b} {}
        explicit Bool(const std::string& s) : BuiltIn(impl::Validator<bool>{}(s)){}
        explicit Bool(std::string&& s) : BuiltIn(impl::Validator<bool>{}(std::move(s))) {}
        explicit Bool(const char * s) : Bool(std::string(s)) {}
        bool getNativeValue()const noexcept {
            return nativeValue;
        }
    private:
        bool nativeValue;
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
        
        void serialize(std::string&,std::ostream& os) const override {
            os << quotation_mark << getValue() << quotation_mark;
        }
    };

    class Number : public BuiltIn {
    public:
        explicit Number(double d = 0.0) : BuiltIn(std::to_string(d)), nativeValue{d} {}
        explicit Number(const std::string& s) : BuiltIn(impl::Validator<double>{}(s)){}
        explicit Number(std::string&& s) : BuiltIn(impl::Validator<double>{}(std::move(s))) {}
        double getNativeValue()const noexcept {
            return nativeValue;
        }
    private:
        double nativeValue;
    };

    class Null : public BuiltIn {
    public:
        explicit Null() : BuiltIn(std::string(value_null)) {}
        explicit Null(const std::string& s) : BuiltIn(impl::Validator<nullptr_t>{}(s)) {}
        explicit Null(std::string&& s) : BuiltIn(impl::Validator<nullptr_t>{}(std::move(s))) {}
        nullptr_t getNativeValue()const noexcept {
            return nullptr;
        }
    };

    template<typename T> struct IsJSONType : public std::false_type {};
    template<> struct IsJSONType<Object> : public std::true_type {};
    template<> struct IsJSONType<Array> : public std::true_type {};
    template<> struct IsJSONType<BuiltIn> : public std::true_type {};
    template<> struct IsJSONType<Bool> : public std::true_type {};
    template<> struct IsJSONType<True> : public std::true_type {};
    template<> struct IsJSONType<False> : public std::true_type {};
    template<> struct IsJSONType<String> : public std::true_type {};
    template<> struct IsJSONType<Number> : public std::true_type {};
    template<> struct IsJSONType<Null> : public std::true_type {};

    template<typename T,typename... Args>
    std::enable_if_t<IsJSONType<T>::value,IObjectPtr> Create(Args&&... args) {
        return IObjectPtr(new T(std::forward<Args>(args)...));
    }

}


#endif //JSON_H_INCLUDED__
