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

    namespace impl {
        template<typename T>
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
            void validate(const std::string& rep) const {
                T temp;
                std::stringstream ss(rep);
                ss >> temp;
                if (std::to_string(temp).find(rep) != 0)
                    throw ValueError();
            }
        };

        template<>
        struct Validator<nullptr_t> {
            const std::string& operator()(const std::string& rep) const {
                validate(rep);
                return rep;
            }

            std::string operator()(std::string&& rep) const {
                validate(rep);
                return std::move(rep);
            }
        private:
            void validate(const std::string& rep) const {
                if (rep != value_null)
                    throw ValueError();
            }
        };

        template<>
        struct Validator<bool> {
            const std::string& operator()(const std::string& rep) const {
                validate(rep);
                return rep;
            }

            std::string operator()(std::string&& rep) const {
                validate(rep);
                return std::move(rep);
            }
        private:
            void validate(const std::string& rep) const {
                if (rep != value_false || rep != value_true)
                    throw ValueError();
            }
        };

        inline std::string to_string(bool b) {
            if (b)
                return JSON::value_true;
            else
                return JSON::value_false;
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
        explicit Bool(const char * s) : BuiltIn(std::string(s)) {}
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
        explicit Null() : BuiltIn(value_null) {}
        explicit Null(const std::string& s) : BuiltIn(impl::Validator<nullptr_t>{}(s)) {}
        explicit Null(std::string&& s) : BuiltIn(impl::Validator<nullptr_t>{}(std::move(s))) {}
    };

    template<typename T>
    struct is_JSON_type : public std::false_type {};
    
    template<>
    struct is_JSON_type<BuiltIn> : public std::true_type {};

    template<>
    struct is_JSON_type<Bool> : public std::true_type {};

    template<>
    struct is_JSON_type<Number> : public std::true_type {};

    template<>
    struct is_JSON_type<String> : public std::true_type {};

    template<>
    struct is_JSON_type<Array> : public std::true_type {};

    template<>
    struct is_JSON_type<Object> : public std::true_type {};



    template<typename T,typename... Args>
    std::enable_if_t<is_JSON_type<T>::value,IObject::ptr> Create(Args&&... args) {
        return IObject::ptr(new T(std::forward<Args>(args)...));
    }

}


#endif //JSON_H_INCLUDED__
