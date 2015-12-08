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
            template<typename CharT>
            const std::basic_string<CharT>& operator()(
                const std::basic_string<CharT>& rep) const
            {
                return validate<CharT>(rep);
            }

            template<typename CharT>
            std::basic_string<CharT>&& operator()(std::basic_string<CharT>&& rep) const
            {
                return validate<CharT>(std::move(rep));
            }
        private:
            template<typename CharT, typename StringType>
            StringType&& validate(StringType&& str) const
            {
                double temp{};
                std::basic_stringstream<CharT> ss(str);
                ss << std::scientific;
                ss >> temp;
                if (ss.bad() || ss.fail()
                    || ss.peek() != std::char_traits<CharT>::eof())
                    throw ValueError();
                return std::forward<StringType>(str);
            }
        };

        template<>
        struct Validator<nullptr_t> {
            template<typename CharT>
            const std::basic_string<CharT>& operator()(
                const std::basic_string<CharT>& rep) const
            {
                return validate<CharT>(rep);
            }

            template<typename CharT>
            std::basic_string<CharT>&& operator()(std::basic_string<CharT>&& rep) const
            {
                return validate<CharT>(std::move(rep));
            }
        private:
            template<typename CharT, typename StringType>
            StringType&& validate(StringType&& str) const
            {
                if (str
                    != std::remove_reference_t<StringType>(
                        LiteralsT<CharT>::value_null()))
                    throw ValueError();
                return std::forward<StringType>(str);
            }
        };

        template<>
        struct Validator<bool> {
            template<typename CharT>
            const std::basic_string<CharT>& operator()(
                const std::basic_string<CharT>& rep) const
            {
                return validate<CharT>(rep);
            }

            template<typename CharT>
            std::basic_string<CharT>&& operator()(std::basic_string<CharT>&& rep) const
            {
                return validate<CharT>(std::move(rep));
            }
        private:
            template<typename CharT, typename StringType>
            StringType&& validate(StringType&& str) const
            {
                if (str
                    != std::basic_string < CharT
                    >(LiteralsT<CharT>::value_false()) && str
                    != std::basic_string<CharT>(LiteralsT<CharT>::value_true()))
                    throw ValueError();
                return std::forward<StringType>(str);
            }
        };

        template<typename CharT>
        std::basic_string<CharT> to_string(bool b)
        {
            if (b)
                return std::basic_string<CharT>(LiteralsT<CharT>::value_true());
            else
                return std::basic_string<CharT>(LiteralsT<CharT>::value_false());
        }
    }

    template<typename CharT>
    class ObjectT : public IAggregateObjectT<CharT> {
    public:
        using StringType = std::basic_string<CharT>;
        using OstreamT = std::basic_ostream<CharT>;
        using Key = StringType;
        using Value = JSON::IObjectPtrT<CharT>;
        using type = std::unordered_map<Key, Value>;
        using Entry = std::pair<const Key, Value>;
        using Literals = JSON::LiteralsT<CharT>;
        using IObjectT = JSON::IObjectT<CharT>;
        using IObjectPtr = JSON::IObjectPtrT<CharT>;

        ObjectT()
        {
        }

        template<typename ... EntryT>
        ObjectT(EntryT&&... list)
        {
            emplace_entries(std::forward<EntryT>(list)...);
        }

        IObjectT& operator[](const StringType& key) override
        {
            auto obj = values.find(key);
            if (obj == values.end())
                throw AttributeMissing();
            return *obj->second;
        }

        const IObjectT& operator[](const StringType& key) const override
        {
            auto obj = values.find(key);
            if (obj == values.end())
                throw AttributeMissing();
            return *obj->second;
        }

        IObjectT& operator[](size_t index) override
        {
            throw TypeError();
        }

        const IObjectT& operator[](size_t index) const override
        {
            throw TypeError();
        }

        virtual const StringType& getValue() const override
        {
            throw TypeError();
        }

        const type& getValues() const
        {
            return values;
        }

        template<typename ... Entries>
        void emplace_entries(Entries&&... entries)
        {
            check_insert(std::forward<Entries>(entries)...);
            values.reserve(sizeof...(Entries));
            emplace_impl(std::forward<Entries>(entries)...);
        }

        void emplace(IObjectPtr && obj) override
        {
            throw AggregateTypeError();
        }

        void emplace(StringType&& name, IObjectPtr && obj) override
        {
            emplace_entries(Entry(std::move(name), std::move(obj)));
        }

        void serialize(StringType&& indentation, OstreamT& os) const override
        {
            using Literals = LiteralsT<CharT>;
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
            using Literals = LiteralsT<CharT>;
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

        void emplace_impl()
        {
        }
        void check_insert()
        {
        }

        template<typename FirstEntry, typename ... Entries>
        std::enable_if_t<std::is_same<FirstEntry, Entry>::value> check_insert(
            FirstEntry&& e, Entries&&... rest)
        {
            if (values.find(e.first) != values.end())
                throw AttributeNotUnique();
            check_insert(std::forward<Entries>(rest)...);
        }

        template<typename FirstEntry, typename ... Entries>
        std::enable_if_t<std::is_same<FirstEntry, Entry>::value> emplace_impl(
            FirstEntry&& e, Entries&&... rest)
        {
            values.emplace(e.first, std::move(e.second));
            emplace_impl(std::forward<Entries>(rest)...);
        }

        type values;
    };

    template<typename CharT>
    class ArrayT : public IAggregateObjectT<CharT> {
    public:

        using IObjectT = JSON::IObjectT<CharT>;
        using IObjectPtrT = JSON::IObjectPtrT<CharT>;
        using StringType = std::basic_string<CharT>;
        using OstreamT = std::basic_ostream<CharT>;
        using Value = IObjectPtrT;
        using type = std::vector<Value>;
        using Literals = JSON::LiteralsT<CharT>;

        ArrayT()
        {
        }

        template<typename ... ObjT>
        ArrayT(std::unique_ptr<ObjT>&&... objs)
        {
            values.reserve(sizeof...(ObjT));
            emplace_impl(std::move(objs)...);
        }

        IObjectT& operator[](const StringType& key) override {
            throw TypeError();
        }

        const IObjectT& operator[](const StringType& key) const override {
            throw TypeError();
        }

        IObjectT& operator[](size_t index) override {
            if (index >= values.size())
                throw OutOfRange();
            return *values[index];
        }

        const IObjectT& operator[](size_t index) const override {
            if (index >= values.size())
                throw OutOfRange();
            return *values[index];
        }

        virtual const StringType& getValue() const override {
            throw TypeError();
        }

        type& getValues() {
            return values;
        }

        const type& getValues() const {
            return values;
        }

        void emplace(IObjectPtrT&& obj) override {
            values.emplace_back(std::move(obj));
        }

        void emplace(StringType&& name, IObjectPtrT&& obj) override {
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
        void emplace_impl() {}
        template<typename FirstObj, typename... Objs>
        std::enable_if_t<std::is_same<std::decay_t<FirstObj>, Value>::value>
            emplace_impl(FirstObj&& f, Objs&&... rest) {
            values.emplace_back(std::move(f));
            emplace_impl(std::forward<Objs>(rest)...);
        }

        type values;
    };

    template<typename CharT>
    class BuiltInT : public IObjectT<CharT> {
    public:

        using IObjectT = JSON::IObjectT<CharT>;
        using IObjectPtrT = JSON::IObjectPtrT<CharT>;
        using StringType = std::basic_string<CharT>;
        using OstreamT = std::basic_ostream<CharT>;

        IObjectT& operator[](const StringType& key) override
        {
            throw TypeError();
        }

        const IObjectT& operator[](const StringType& key) const override
        {
            throw TypeError();
        }

        IObjectT& operator[](size_t index) override
        {
            throw TypeError();
        }

        const IObjectT& operator[](size_t index) const override
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
        BuiltInT() :
            value{}
        {
        }
        BuiltInT(const StringType& s) :
            value{ s }
        {
        }
        BuiltInT(StringType&& s) :
            value{ std::move(s) }
        {
        }
        StringType value;
    };

    template<typename CharT>
    class BoolT : public BuiltInT<CharT> {
    public:
        using StringType = std::basic_string<CharT>;

        explicit BoolT(bool b = false) :
            BuiltInT<CharT>(impl::to_string<CharT>(b)), nativeValue{ b }
        {
        }
        explicit BoolT(const StringType& s) :
            BuiltInT<CharT>(impl::Validator<bool> { }(s))
        {
        }
        explicit BoolT(StringType&& s) :
            BuiltInT<CharT>(impl::Validator<bool> { }(std::move(s)))
        {
        }
        explicit BoolT(const CharT * s) :
            BoolT(StringType(s))
        {
        }
        bool getNativeValue() const noexcept
        {
            return nativeValue;
        }
    private:
        bool nativeValue;
    };

    template<typename CharT>
    class TrueT : public BoolT<CharT> {
    public:
        using StringType = std::basic_string<CharT>;

        TrueT() :
            BoolT<CharT>(LiteralsT<CharT>::value_true())
        {
        }
        explicit TrueT(const StringType& s) :
            BoolT<CharT>(s)
        {
        }
        explicit TrueT(StringType&& s) :
            BoolT<CharT>(s)
        {
        }
        static const CharT* Literal()
        {
            return LiteralsT<CharT>::value_true();
        }
    };

    template<typename CharT>
    class FalseT : public BoolT<CharT> {
    public:
        using StringType = std::basic_string<CharT>;

        FalseT() :
            BoolT<CharT>(LiteralsT<CharT>::value_false())
        {
        }
        ;
        explicit FalseT(const StringType& s) :
            BoolT<CharT>(s)
        {
        }
        explicit FalseT(StringType&& s) :
            BoolT<CharT>(s)
        {
        }
        static const CharT* Literal()
        {
            return LiteralsT<CharT>::value_false();
        }
    };

    template<typename CharT>
    class StringT : public BuiltInT<CharT> {
    public:
        using StringType = std::basic_string<CharT>;
        using OstreamT = std::basic_ostream<CharT>;

        StringT() :
            BuiltInT<CharT>(StringType{})
        {
        }
        explicit StringT(const StringType& t) :
            BuiltInT<CharT>(t)
        {
        }
        explicit StringT(StringType&& t) :
            BuiltInT<CharT>(std::move(t))
        {
        }

        void serialize(StringType&&, OstreamT& os) const override
        {
            serialize(os);
        }

        void serialize(OstreamT& os) const override
        {
            os << LiteralsT<CharT>::quotation_mark << this->getValue()
                << LiteralsT<CharT>::quotation_mark;
        }
    };

    template<typename CharT>
    class NumberT : public BuiltInT<CharT> {
    public:
        using StringType = std::basic_string<CharT>;

        explicit NumberT(double d = 0.0) :
            nativeValue{ d }
        {
            std::basic_stringstream<CharT> ss{};
            ss << std::scientific << d;
            this->value = ss.str();
        }
        explicit NumberT(const StringType& s) :
            BuiltInT<CharT>(impl::Validator<double> { }(s))
        {
        }
        explicit NumberT(StringType&& s) :
            BuiltInT<CharT>(impl::Validator<double> { }(std::move(s)))
        {
        }
        double getNativeValue() const noexcept
        {
            return nativeValue;
        }
    private:
        double nativeValue;
    };

    template<typename CharT>
    class NullT : public BuiltInT<CharT> {
    public:
        using StringType = std::basic_string<CharT>;

        explicit NullT() :
            BuiltInT<CharT>(StringType(LiteralsT<CharT>::value_null()))
        {
        }
        explicit NullT(const StringType& s) :
            BuiltInT<CharT>(impl::Validator<nullptr_t> { }(s))
        {
        }
        explicit NullT(StringType&& s) :
            BuiltInT<CharT>(impl::Validator<nullptr_t> { }(std::move(s)))
        {
        }
        nullptr_t getNativeValue() const noexcept
        {
            return nullptr;
        }
        static const CharT* Literal()
        {
            return LiteralsT<CharT>::value_null();
        }
    };

    template<typename T> struct IsJSONType : public std::false_type {
    };
    template<typename C> struct IsJSONType<ObjectT<C>> : public std::true_type {
        using CharT = C;
    };
    template<typename C> struct IsJSONType<ArrayT<C>> : public std::true_type {
        using CharT = C;
    };
    template<typename C> struct IsJSONType<BuiltInT<C>> : public std::true_type {
        using CharT = C;
    };
    template<typename C> struct IsJSONType<BoolT<C>> : public std::true_type {
        using CharT = C;
    };
    template<typename C> struct IsJSONType<TrueT<C>> : public std::true_type {
        using CharT = C;
    };
    template<typename C> struct IsJSONType<FalseT<C>> : public std::true_type {
        using CharT = C;
    };
    template<typename C> struct IsJSONType<StringT<C>> : public std::true_type {
        using CharT = C;
    };
    template<typename C> struct IsJSONType<NumberT<C>> : public std::true_type {
        using CharT = C;
    };
    template<typename C> struct IsJSONType<NullT<C>> : public std::true_type {
        using CharT = C;
    };

    template<typename T, typename ... Args>
    std::enable_if_t<IsJSONType<T>::value,
        IObjectPtrT<typename IsJSONType<T>::CharT> > Create(Args&&... args)
    {
        return IObjectPtrT<typename IsJSONType<T>::CharT>(
            new T(std::forward<Args>(args)...));
    }

    template<typename CharT>
    struct Types {
        using IObject = IObjectT<CharT>;
        using IObjectPtr = IObjectPtrT<CharT>;
        using IAggregateObject = IAggregateObjectT<CharT>;
        using Object = ObjectT<CharT>;
        using Array = ArrayT<CharT>;
        using BuiltIn = BuiltInT<CharT>;
        using Bool = BoolT<CharT>;
        using True = TrueT<CharT>;
        using False = FalseT<CharT>;
        using String = StringT<CharT>;
        using Number = NumberT<CharT>;
        using Null = NullT<CharT>;
        using Literals = LiteralsT<CharT>;
    };
}

#endif //JSONOBJECTS_H_INCLUDED__
