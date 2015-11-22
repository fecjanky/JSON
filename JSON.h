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

namespace JSON {
    struct Boolean {
        bool val;
        Boolean(bool b = false) :val{ b } {}
        operator bool()const noexcept {
            return val;
        }
    };
}

std::istream& operator>>(std::istream& is, JSON::Boolean);

namespace std {
    std::string to_string(JSON::Boolean b) {
        if (b)
            return "true";
        else
            return "false";
    }
}

namespace JSON {

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

        constexpr char  begin_array = 0x5B; // [ left square bracket
        constexpr char  begin_object = 0x7B; // { left curly bracket
        constexpr char  end_array = 0x5D; // ] right square bracket
        constexpr char  end_object = 0x7D; // } right curly bracket
        constexpr char  name_separator = 0x3A; // : colon
        constexpr char  value_separator = 0x2C; // , comma
        constexpr const char ws[] = {
                0x20, //  Space
                0x09, //  Horizontal tab
                0x0A, //  Line feed or New line
                0x0D, //  Carriage return
        };
        constexpr const char value_false[] = { 0x66,0x61,0x6c,0x73,0x65 }; //false
        constexpr const char value_true[] = { 0x74,0x72,0x75,0x65 }; //true
        constexpr const char value_null[] = { 0x6e,0x75,0x6c,0x6c }; //null


    }
    

    using namespace RFC7159;

    struct Exception : public std::exception {};
    struct AttributeMissing : Exception {};
    struct TypeError : public Exception {};
    struct ValueError : public Exception {};
    struct OutOfRange : public Exception {};
    struct AttributeNotUnique : public Exception {};

    struct IObject {
        using ptr = std::unique_ptr<IObject>;
        
        virtual IObject& operator[](const std::string& key) = 0;
        virtual const IObject& operator[](const std::string& key) const = 0;
        
        virtual IObject& operator[](size_t index) = 0;
        virtual const IObject& operator[](size_t index) const = 0;
        
        virtual const std::string& getValue() const = 0;
        
        virtual ~IObject() = default;
    };


    class Object : public IObject {
    public:

        using Key = std::string;
        using Value = ptr;
        using type = std::unordered_map<Key, Value>;
        using Entry = std::pair<const Key, ptr>;

        template<typename... EntryT>
        Object(EntryT&&... list) {
            emplace(std::forward<EntryT>(list)...);
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
        void emplace(Entries&&... entries) {
            check_insert(std::forward<Entries>(entries)...);
            values.reserve(sizeof...(Entries));
            emplace_impl(std::forward<Entries>(entries)...);
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
            emplace(std::forward<Entries>(rest)...);
        }

        type values;
    };

    class Array : public IObject {
    public:

        using Value = ptr;
        using type = std::vector<Value>;

        template<typename... ObjT>
        Array(std::unique_ptr<ObjT>&&... objs) {
            values.reserve(sizeof...(ObjT));
            emplace(std::move(objs)...);
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

    private:
        void emplace() {}
        template<typename FirstObj, typename... Objs>
        std::enable_if_t<std::is_same<FirstObj, Value>::value> emplace(FirstObj&& f, Objs&&... rest) {
            values.emplace(std::move(f));
            emplace(std::forward<Objs>(rest)...);
        }

        type values;
    };

    template<typename T>
    class BuiltInValue : public IObject {
    public:
        struct Validator {
            const std::string& operator()(const std::string& rep) const {
                validate(rep);
                return rep;
            }

            std::string operator()(std::string&& rep) const {
                validate(rep);
                return std::move(rep);
            }
        private:
            void validate(const std::string& rep) const  {
                T temp;
                std::stringstream ss(rep);
                ss << std::boolalpha;
                ss >> temp;
                if (std::to_string(temp).find(rep) != 0)
                    throw ValueError();
            }
        };

        explicit BuiltInValue(T t) : value{std::to_string(t)} {}
        explicit BuiltInValue(const std::string& s) : value{ Validator{}(s) }{}
        explicit BuiltInValue(std::string&& s) : value{ Validator{}(std::move(s)) } {}

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

    private:
        std::string value;
    };


    template<>
    class BuiltInValue<std::string> : public IObject {
    public:
        
        explicit BuiltInValue(const std::string& t) : value{ t } {}
        explicit BuiltInValue(std::string&& t) : value{ std::move(t) } {}

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

    private:
        std::string value;
    };

    using Number = BuiltInValue<double>;
    using Bool = BuiltInValue<Boolean>;
    using Null = BuiltInValue<nullptr_t>;
    using String = BuiltInValue<std::string>;

    template<typename T>
    struct is_JSON_type : public std::false_type {};
    
    template<typename T>
    struct is_JSON_type<BuiltInValue<T>> : public std::true_type {};

    template<>
    struct is_JSON_type<Array> : public std::true_type {};

    template<>
    struct is_JSON_type<Object> : public std::true_type {};



    template<typename T,typename... Args>
    std::enable_if_t<is_JSON_type<T>::value,IObject::ptr> Create(Args&&... args) {
        return IObject::ptr(new T(std::forward<Args>(args)...));
    }

    inline std::istream& operator>>(std::istream& is, JSON::Boolean& b) {
        std::string s;
        is >> s;
        if (s == RFC7159::value_false)
            b = false;
        else if (s == RFC7159::value_true)
            b = true;
        else
            //is.failbit 
            // TODO: indicate error
            ;
        return is;
    }

    inline std::ostream& operator<<(std::ostream& os, JSON::Boolean b) {
        if (b)
            os << RFC7159::value_true;
        else
            os << RFC7159::value_false;
        return os;
    }

}




#endif //JSON_H_INCLUDED__
