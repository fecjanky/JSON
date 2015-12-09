#ifndef JSONOBJECTS_H_INCLUDED__
#define JSONOBJECTS_H_INCLUDED__

#include <unordered_map>
#include <vector>
#include <type_traits>
#include <utility>
#include <sstream>

#include "JSON.h"
#include "JSONLiterals.h"

namespace JSON {
    
    namespace impl {
        template<typename T>
        struct Validator;

        template<>
        struct Validator<double> {
            const std::string& operator()(
                const std::string& rep) const
            {
                return validate(rep);
            }

            
            std::string&& operator()(std::string&& rep) const
            {
                return validate(std::move(rep));
            }
        private:
            template<typename StringType>
            StringType&& validate(StringType&& str) const
            {
                double temp{};
                std::stringstream ss(str);
                ss << std::scientific;
                ss >> temp;
                if (ss.bad() || ss.fail()
                    || ss.peek() != std::char_traits<char>::eof())
                    throw ValueError();
                return std::forward<StringType>(str);
            }
        };

        template<>
        struct Validator<nullptr_t> {
            
            const std::string& operator()(
                const std::string& rep) const
            {
                return validate(rep);
            }

            
            std::string&& operator()(std::string&& rep) const
            {
                return validate(std::move(rep));
            }
        private:
            template<typename StringType>
            StringType&& validate(StringType&& str) const
            {
                if (str
                    != std::remove_reference_t<StringType>(
                        Literals::value_null()))
                    throw ValueError();
                return std::forward<StringType>(str);
            }
        };

        template<>
        struct Validator<bool> {
            
            const std::string& operator()(
                const std::string& rep) const
            {
                return validate(rep);
            }

            
            std::string&& operator()(std::string&& rep) const
            {
                return validate(std::move(rep));
            }
        private:
            template<typename StringType>
            StringType&& validate(StringType&& str) const
            {
                if (str
                    != std::basic_string < char
                    >(Literals::value_false()) && str
                    != std::string(Literals::value_true()))
                    throw ValueError();
                return std::forward<StringType>(str);
            }
        };

        std::string to_string(bool b)
        {
            if (b)
                return std::string(Literals::value_true());
            else
                return std::string(Literals::value_false());
        }
    }

    
    class Object : public IAggregateObject {
    public:
        using StringType = IAggregateObject::StringType;
        using OstreamT = IAggregateObject::OstreamT;
        using IObjectPtr = IAggregateObject::IObjectPtr;
        using IObject = IAggregateObject::IObject;
        using Key = StringType;
        using Value = IObjectPtr;
        using Entry = std::pair<const Key, Value>;
        using Container = std::unordered_map<Key, Value>;
        using Literals = JSON::Literals;
        

        Object()
        {
        }

        IObject& operator[](const StringType& key) override
        {
            auto obj = values.find(key);
            if (obj == values.end())
                throw AttributeMissing();
            return *obj->second;
        }

        const IObject& operator[](const StringType& key) const override
        {
            auto obj = values.find(key);
            if (obj == values.end())
                throw AttributeMissing();
            return *obj->second;
        }

        IObject& operator[](size_t index) override
        {
            throw TypeError();
        }

        const IObject& operator[](size_t index) const override
        {
            throw TypeError();
        }

        virtual const StringType& getValue() const override
        {
            throw TypeError();
        }

        const Container& getValues() const
        {
            return values;
        }


        void emplace(IObjectPtr && obj) override
        {
            throw AggregateTypeError();
        }

        void emplace(StringType&& name, IObjectPtr && obj) override
        {
            if (values.find(name) != values.end())
                throw AttributeNotUnique();
            values.emplace(std::move(name), std::move(obj));
        }

        void serialize(StringType&& indentation, OstreamT& os) const override
        {
            os << Literals::newline << indentation << Literals::begin_object
                << Literals::newline;
            indentation.push_back(Literals::space);
            indentation.push_back(Literals::space);
            for (auto i = values.begin(); i != values.end();) {
                os << indentation << Literals::quotation_mark << i->first
                    << Literals::quotation_mark << Literals::space
                    << Literals::name_separator << Literals::space;

                i->second->serialize(std::move(indentation), os);
                ++i;
                if (i != values.end())
                    os << Literals::space << Literals::value_separator
                    << Literals::newline;
                else
                    os << Literals::newline;

            }
            indentation.pop_back();
            indentation.pop_back();
            os << indentation << Literals::end_object;
        }

        void serialize(OstreamT& os) const override {
            os << Literals::begin_object;
            for (auto i = values.begin(); i != values.end();) {
                os << Literals::quotation_mark << i->first
                    << Literals::quotation_mark << Literals::name_separator;
                i->second->serialize(os);
                ++i;
                if (i != values.end())
                    os << Literals::value_separator;
            }
            os << Literals::end_object;
        }

    private:
        Container values;
    };

    
    class Array : public IAggregateObject {
    public:
        using StringType = IAggregateObject::StringType;
        using OstreamT = IAggregateObject::OstreamT;
        using IObjectPtr = IAggregateObject::IObjectPtr;
        using IObject = IAggregateObject::IObject;
        using Value = IObjectPtr;
        using Container = std::vector<Value>;
        using Literals = JSON::Literals;

        Array()
        {
        }

        IObject& operator[](const StringType& key) override {
            throw TypeError();
        }

        const IObject& operator[](const StringType& key) const override {
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

        virtual const StringType& getValue() const override {
            throw TypeError();
        }

        Container& getValues() {
            return values;
        }

        const Container& getValues() const {
            return values;
        }

        void emplace(IObjectPtr&& obj) override {
            values.emplace_back(std::move(obj));
        }

        void emplace(StringType&& name, IObjectPtr&& obj) override {
            throw AggregateTypeError();
        }

        void serialize(StringType&& indentation, OstreamT& os) const override {
            os << Literals::begin_array << Literals::space;
            for (auto i = values.begin(); i != values.end();) {
                (*i)->serialize(std::move(indentation), os);
                ++i;
                if (i != values.end())
                    os << Literals::space <<
                    Literals::value_separator << Literals::space;
            }
            os << Literals::space << Literals::end_array;
        }

        void serialize(OstreamT& os) const override {
            os << Literals::begin_array;
            for (auto i = values.begin(); i != values.end();) {
                (*i)->serialize(os);
                ++i;
                if (i != values.end()) os << Literals::value_separator;
            }
            os << Literals::end_array;
        }

    private:
        Container values;
    };

    class BuiltIn : public IObject {
    public:

        using StringType = IObject::StringType;
        using OstreamT = IObject::OstreamT;
        using IObjectPtr = IObject::IObjectPtr;
        using Literals = JSON::Literals;

        IObject& operator[](const StringType& key) override
        {
            throw TypeError();
        }

        const IObject& operator[](const StringType& key) const override
        {
            throw TypeError();
        }

        IObject& operator[](size_t index) override
        {
            throw TypeError();
        }

        const IObject& operator[](size_t index) const override
        {
            throw TypeError();
        }

        virtual const StringType& getValue() const override
        {
            return value;
        }

        void serialize(StringType&&, OstreamT& os) const override
        {
            os << value;
        }
        void serialize(OstreamT& os) const override {
            os << value;
        }

    protected:
        BuiltIn() :
            value{}
        {
        }
        BuiltIn(const StringType& s) :
            value{ s }
        {
        }
        BuiltIn(StringType&& s) :
            value{ std::move(s) }
        {
        }
        StringType value;
    };


    class Bool : public BuiltIn {
    public:

        using StringType = BuiltIn::StringType;
        using Literals = JSON::Literals;

        explicit Bool(bool b = false) :
            BuiltIn(impl::to_string(b)), nativeValue{ b }
        {
        }
        explicit Bool(const StringType& s) :
            BuiltIn(impl::Validator<bool> { }(s))
        {
        }
        explicit Bool(StringType&& s) :
            BuiltIn(impl::Validator<bool> { }(std::move(s)))
        {
        }
        explicit Bool(const char * s) :
            Bool(StringType(s))
        {
        }
        bool getNativeValue() const noexcept
        {
            return nativeValue;
        }
    private:
        bool nativeValue;
    };

    
    class True : public Bool {
    public:
        using StringType = Bool::StringType;

        True() :
            Bool(Literals::value_true())
        {
        }
        explicit True(const StringType& s) :
            Bool(s)
        {
        }
        explicit True(StringType&& s) :
            Bool(std::move(s))
        {
        }
        static const char* Literal()
        {
            return Literals::value_true();
        }
    };

    class False : public Bool{
    public:
        using StringType = Bool::StringType;

        False() :
            Bool(Literals::value_false())
        {
        }
        ;
        explicit False(const StringType& s) :
            Bool(s)
        {
        }
        explicit False(StringType&& s) :
            Bool(std::move(s))
        {
        }
        static const char* Literal()
        {
            return Literals::value_false();
        }
    };

    class String : public BuiltIn {
    public:
        using StringType = BuiltIn::StringType;
        using OstreamT = BuiltIn::OstreamT;

        String() :
            BuiltIn(StringType{})
        {
        }
        explicit String(const StringType& t) :
            BuiltIn(t)
        {
        }
        explicit String(StringType&& t) :
            BuiltIn(std::move(t))
        {
        }

        void serialize(StringType&&, OstreamT& os) const override
        {
            serialize(os);
        }

        void serialize(OstreamT& os) const override
        {
            os << Literals::quotation_mark << this->getValue()
                << Literals::quotation_mark;
        }
    };

    class Number : public BuiltIn {
    public:
        using StringType = BuiltIn::StringType;

        explicit Number(double d = 0.0) :
            nativeValue{ d }
        {
            std::stringstream ss{};
            ss << std::scientific << d;
            this->value = ss.str();
        }
        explicit Number(const StringType& s) :
            BuiltIn(impl::Validator<double> { }(s))
        {
        }
        explicit Number(StringType&& s) :
            BuiltIn(impl::Validator<double> { }(std::move(s)))
        {
        }
        double getNativeValue() const noexcept
        {
            return nativeValue;
        }
    private:
        double nativeValue;
    };

    class Null : public BuiltIn {
    public:
        using StringType = BuiltIn::StringType;

        explicit Null() :
            BuiltIn(StringType(Literals::value_null()))
        {
        }
        explicit Null(const StringType& s) :
            BuiltIn(impl::Validator<nullptr_t> { }(s))
        {
        }
        explicit Null(StringType&& s) :
            BuiltIn(impl::Validator<nullptr_t> { }(std::move(s)))
        {
        }
        nullptr_t getNativeValue() const noexcept
        {
            return nullptr;
        }
        static const char* Literal()
        {
            return Literals::value_null();
        }
    };


    template<typename T,typename B = void> struct IsJSONType : public std::false_type {
    };

    template<typename T>
    struct IsJSONType<
        T, 
        std::enable_if_t<std::is_convertible<T*, IObject*>::value>
         > : public std::true_type {
    };

    template<typename T, typename ... Args>
    std::enable_if_t<
        IsJSONType<T>::value,
        IObjectPtr> Create(Args&&... args)
    {
        return IObjectPtr(new T(std::forward<Args>(args)...));
    }


}

#endif //JSONOBJECTS_H_INCLUDED__
