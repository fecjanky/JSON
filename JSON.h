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
#include <array>

// TODO: Mutable and Immutable objects
namespace JSON {

    struct Exception : public std::exception {};
    struct AttributeMissing : JSON::Exception {};
    struct TypeError : public JSON::Exception {};
    struct AggregateTypeError : public JSON::Exception {};
    struct ValueError : public JSON::Exception {};
    struct OutOfRange : public JSON::Exception {};
    struct AttributeNotUnique : public JSON::Exception {};

    template<typename CharT>
    struct IObjectT;
    template<typename CharT>
    struct IAggregateObjectT;
    
    template<typename CharT>
    using IObjectPtrT = std::unique_ptr<IObjectT<CharT>>;

    template<typename CharT>
    using IAggregateObjectPtrT = std::unique_ptr<IAggregateObjectT<CharT>>;

    template<typename CharT_>
    struct IObjectT {
        using CharT = CharT_;
        using IObjectPtrT = IObjectPtrT<CharT>;
        using StringType = std::basic_string<CharT>;
        using OstreamT = std::basic_ostream<CharT>;

        virtual IObjectT& operator[](const StringType& key) = 0;
        virtual const IObjectT& operator[](const StringType& key) const = 0;

        virtual IObjectT& operator[](size_t index) = 0;
        virtual const IObjectT& operator[](size_t index) const = 0;

        virtual const StringType& getValue() const = 0;

        operator StringType () const {
            return getValue();
        }

        virtual void serialize(StringType& indentation, OstreamT&) const = 0;

        virtual ~IObjectT() = default;
    };

    template<typename CharT>
    inline std::basic_ostream<CharT>& operator << (
            std::basic_ostream<CharT>& os, const IObjectT<CharT>& obj) {
        obj.serialize(IObjectT<CharT>::StringType{},os);
        return os;
    }

    template<typename CharT>
    struct IAggregateObjectT : public IObjectT<CharT> {
        // for arrays
        virtual void emplace(IObjectPtrT&& obj) = 0;
        // for objects
        virtual void emplace(StringType&& name, IObjectPtrT&& obj) = 0;

        virtual ~IAggregateObjectT() = default;
    };

    template<typename CharT>
    struct LiteralsT;

    template<>
    struct LiteralsT<char> {
        static constexpr auto  newline = '\n';
        static constexpr auto  space = ' ';
        static constexpr auto  begin_array = '['; 
        static constexpr auto  begin_object = '{';
        static constexpr auto  end_array = ']';
        static constexpr auto  end_object = '}';
        static constexpr auto  name_separator = ':';
        static constexpr auto  value_separator = ',';
        static constexpr auto  string_escape = '\\';
        static constexpr auto  string_unicode_escape = 'u';
        static constexpr auto  quotation_mark = '"';
        static constexpr std::array<char, 4> whitespace() { 
            return {' ', '\t', '\n', '\r' }; 
        }
        static constexpr const char*const whitespace_string() {
            return " \t\n\r";
        }
        static constexpr std::array<char, 8> string_escapes() { 
            return{ '"','\\','/','b','f','n','r','t' };
        }
        static constexpr const char*const begin_number() {
            return "-0123456789";
        }
        static constexpr const char*const value_false() { return "false"; }
        static constexpr const char*const value_true() { return "true"; }
        static constexpr const char*const value_null() { return "null"; }
    };

    template<>
    struct LiteralsT<wchar_t> {
        static constexpr auto newline = L'\n';
        static constexpr auto space = L' ';
        static constexpr auto  begin_array = L'[';
        static constexpr auto  begin_object = L'{';
        static constexpr auto  end_array = L']';
        static constexpr auto  end_object = L'}';
        static constexpr auto  name_separator = L':';
        static constexpr auto  value_separator = L',';
        static constexpr auto string_escape = '\\';
        static constexpr auto string_unicode_escape = L'u';
        static constexpr auto quotation_mark = L'"';
        static constexpr std::array<wchar_t, 4> whitespace() {
            return{ L' ', L'\t', L'\n', L'\r' };
        }
        static constexpr const wchar_t*const whitespace_string() {
            return L" \t\n\r";
        }
        static constexpr std::array<wchar_t, 8> string_escapes() {
            return{ L'"',L'\\',L'/',L'b',L'f',L'n',L'r',L't' };
        }
        static constexpr const wchar_t*const begin_number() {
            return L"-0123456789";
        }
        static constexpr const wchar_t*const value_false() { return L"false"; }
        static constexpr const wchar_t*const value_true() { return L"true"; }
        static constexpr const wchar_t*const value_null() { return L"null"; }
    };

    template<>
    struct LiteralsT<char16_t> {
        static constexpr auto newline = u'\n';
        static constexpr auto space = u' ';
        static constexpr auto  begin_array = u'[';
        static constexpr auto  begin_object = u'{';
        static constexpr auto  end_array = u']';
        static constexpr auto  end_object = u'}';
        static constexpr auto  name_separator = u':';
        static constexpr auto  value_separator = u',';
        static constexpr auto  string_escape = '\\';
        static constexpr auto  string_unicode_escape = u'u';
        static constexpr auto  quotation_mark = u'"';
        static constexpr std::array<char16_t, 4> whitespace() {
            return{ u' ', u'\t', u'\n', u'\r' };
        }
        static constexpr const char16_t*const whitespace_string() {
            return u" \t\n\r";
        }
        static constexpr std::array<char16_t, 8> string_escapes() {
            return{ u'"',u'\\',u'/',u'b',u'f',u'n',u'r',u't' };
        }
        static constexpr const char16_t*const begin_number() {
            return u"-0123456789";
        }
        static constexpr const char16_t*const value_false() { return u"false"; }
        static constexpr const char16_t*const value_true() { return u"true"; }
        static constexpr const char16_t*const value_null() { return u"null"; }
    };

    template<>
    struct LiteralsT<char32_t> {
        static constexpr auto newline = U'\n';
        static constexpr auto space = U' ';
        static constexpr auto  begin_array = U'[';
        static constexpr auto  begin_object = U'{';
        static constexpr auto  end_array = U']';
        static constexpr auto  end_object = U'}';
        static constexpr auto  name_separator = U':';
        static constexpr auto  value_separator = U',';
        static constexpr auto  string_escape = '\\';
        static constexpr auto  string_unicode_escape = U'u';
        static constexpr auto  quotation_mark = U'"';
        static constexpr std::array<char32_t, 4> whitespace() {
            return{ U' ', U'\t', U'\n', U'\r' };
        }
        static constexpr const char32_t*const whitespace_string() {
            return U" \t\n\r";
        }
        static constexpr std::array<char32_t, 8> string_escapes() {
            return{ U'"',U'\\',U'/',U'b',U'f',U'n',U'r',U't' };
        }
        static constexpr const char32_t*const begin_number() {
            return U"-0123456789";
        }
        static constexpr const char32_t*const value_false() { return U"false"; }
        static constexpr const char32_t*const value_true() { return U"true"; }
        static constexpr const char32_t*const value_null() { return U"null"; }
    };

    
    template<typename CharT>
    class ObjectT : public IAggregateObjectT<CharT> {
    public:

        using Key = StringType;
        using Value = IObjectPtrT;
        using type = std::unordered_map<Key, Value>;
        using Entry = std::pair<const Key, IObjectPtrT>;
        using Literals = JSON::LiteralsT<CharT>;
         

        ObjectT(){}

        template<typename... EntryT>
        ObjectT(EntryT&&... list) {
            emplace_entries(std::forward<EntryT>(list)...);
        }

        IObjectT& operator[](const StringType& key) override {
            auto obj = values.find(key);
            if (obj == values.end())
                throw AttributeMissing();
            return *obj->second;
        }

        const IObjectT& operator[](const StringType& key) const override {
            auto obj = values.find(key);
            if(obj == values.end())
                throw AttributeMissing();
            return *obj->second;
        }

        IObjectT& operator[](size_t index) override {
            throw TypeError();
        }

        const IObjectT& operator[](size_t index) const override {
            throw TypeError();
        }

        virtual const StringType& getValue() const override {
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

        void emplace(IObjectPtrT&& obj) override{
            throw AggregateTypeError();
        }

        void emplace(StringType&& name, IObjectPtrT&& obj) override {
            emplace_entries(Entry(std::move(name),std::move(obj)));
        }

        void serialize(StringType& indentation, OstreamT& os) const override {
            os << Literals::newline << indentation << 
                Literals::begin_object << Literals::newline;
            indentation.push_back(Literals::space);
            indentation.push_back(Literals::space);
            for (auto i = values.begin(); i != values.end();) {
                os << indentation << Literals::quotation_mark << 
                    i->first << Literals::quotation_mark <<
                        Literals::space << Literals::name_separator << 
                            Literals::space;
                
                i->second->serialize(indentation,os);
                ++i;
                if (i != values.end()) 
                    os << Literals::space << 
                        Literals::value_separator << Literals::newline;
                else os << Literals::newline;
                
            }
            indentation.pop_back();
            indentation.pop_back();
            os << indentation << Literals::end_object;
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

    template<typename CharT>
    class ArrayT : public IAggregateObjectT<CharT> {
    public:

        using Value = IObjectPtrT;
        using type = std::vector<Value>;
        using Literals = JSON::LiteralsT<CharT>;

        ArrayT(){}

        template<typename... ObjT>
        ArrayT(std::unique_ptr<ObjT>&&... objs) {
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

        void emplace(IObjectPtrT&& obj) override{
            values.emplace_back(std::move(obj));
        }

        void emplace(StringType&& name, IObjectPtrT&& obj) override {
            throw AggregateTypeError();
        }

        void serialize(StringType& indentation,OstreamT& os) const  override {
            os << Literals::begin_array << Literals::space;
            for (auto i = values.begin(); i != values.end();) {
                (*i)->serialize(indentation,os);
                ++i;
                if (i != values.end()) 
                    os << Literals::space << 
                        Literals::value_separator << Literals::space;
            }
            os << Literals::space << Literals::end_array;
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

    namespace impl {
        template<typename T>
        struct Validator {
            template<typename CharT>
            const std::basic_string<CharT>& operator()(
                const std::basic_string<CharT>& rep) const {
                return validate<CharT>(rep);
            }

            template<typename CharT>
            std::basic_string<CharT>&& operator()(std::basic_string<CharT>&& rep) const {
                return validate<CharT>(std::move(rep));
            }
        private:
            template<typename CharT, typename StringType>
            StringType&& validate(StringType&& str) const {
                T temp{};
                std::basic_stringstream<CharT> ss(str);
                ss << std::scientific;
                ss >> temp;
                if (ss.bad() || ss.fail() || ss.peek() != std::char_traits<CharT>::eof())
                    throw ValueError();
                return std::forward<StringType>(str);
            }
        };

        template<>
        struct Validator<nullptr_t> {
            template<typename CharT>
            const std::basic_string<CharT>& operator()(
                const std::basic_string<CharT>& rep) const {
                return validate<CharT>(rep);
            }

            template<typename CharT>
            std::basic_string<CharT>&& operator()(std::basic_string<CharT>&& rep) const {
                return validate<CharT>(std::move(rep));
            }
        private:
            template<typename CharT, typename StringType>
            StringType&& validate(StringType&& str) const {
                if (str != std::remove_reference_t<StringType>(Literals::value_null()))
                    throw ValueError();
                return std::forward<StringType>(str);
            }
        };

        template<>
        struct Validator<bool> {
            template<typename CharT>
            const std::basic_string<CharT>& operator()(
                const std::basic_string<CharT>& rep) const {
                return validate<CharT>(rep);
            }

            template<typename CharT>
            std::basic_string<CharT>&& operator()(std::basic_string<CharT>&& rep) const {
                return validate<CharT>(std::move(rep));
            }
        private:
            template<typename CharT, typename StringType>
            StringType&& validate(StringType&& str) const {
                if (str != std::basic_string<CharT>(LiteralsT<CharT>::value_false()) &&
                    str != std::basic_string<CharT>(LiteralsT<CharT>::value_true()))
                    throw ValueError();
                return std::forward<StringType>(str);
            }
        };

        template<typename CharT>
        std::basic_string<CharT> to_string(bool b) {
            if (b)
                return std::basic_string<CharT>(value_true);
            else
                return std::basic_string<CharT>(value_false);
        }
    }

    template<typename CharT>
    class BuiltInT : public IObjectT<CharT>{
    public:
        IObjectT& operator[](const StringType& key) override {
            throw TypeError();
        }

        const IObjectT& operator[](const StringType& key) const override {
            throw TypeError();
        }

        IObjectT& operator[](size_t index) override {
            throw TypeError();
        }

        const IObjectT& operator[](size_t index) const override {
            throw TypeError();
        }

        virtual const StringType& getValue() const override {
            return value;
        }

        void serialize(StringType&,OstreamT& os) const override {
            os << value;
        }

    protected:
        BuiltInT() : value{} {}
        BuiltInT(const StringType& s) : value{ s } {}
        BuiltInT(StringType&& s) : value{ std::move(s) } {}
        StringType value;
    };

    template<typename CharT>
    class BoolT : public BuiltInT<CharT>{
    public:
        explicit BoolT(bool b = false) : BuiltInT(impl::to_string<CharT>(b)), nativeValue{b} {}
        explicit BoolT(const StringType& s) : BuiltInT(impl::Validator<bool>{}(s)){}
        explicit BoolT(StringType&& s) : BuiltInT(impl::Validator<bool>{}(std::move(s))) {}
        explicit BoolT(const CharT * s) : BoolT(StringType(s)) {}
        bool getNativeValue()const noexcept {
            return nativeValue;
        }
    private:
        bool nativeValue;
    };

    template<typename CharT>
    class TrueT : public BoolT<CharT>{
    public:
        TrueT() : BoolT(LiteralsT<CharT>::value_true()) {}
        explicit TrueT(const StringType& s) : BoolT(s) {}
        explicit TrueT(StringType&& s) : BoolT(s) {}
        static const CharT* Literal() {
            return LiteralsT<CharT>::value_true();
        }
    };

    template<typename CharT>
    class FalseT : public BoolT<CharT> {
    public:
        FalseT() : BoolT(LiteralsT<CharT>::value_false()) {};
        explicit FalseT(const StringType& s) : BoolT(s) {}
        explicit FalseT(StringType&& s) : BoolT(s) {}
        static const CharT* Literal() {
            return LiteralsT<CharT>::value_false();
        }
    };


    template<typename CharT>
    class StringT : public BuiltInT<CharT>{
    public:
        StringT() : BuiltInT(StringType{}) {}
        explicit StringT(const StringType& t) : BuiltInT(t) {}
        explicit StringT(StringType&& t) : BuiltInT(std::move(t)) {}
        
        void serialize(StringType&,OstreamT& os) const override {
            os << LiteralsT<CharT>::quotation_mark << getValue() 
                << LiteralsT<CharT>::quotation_mark;
        }
    };

    template<typename CharT>
    class NumberT : public BuiltInT<CharT> {
    public:
        explicit NumberT(double d = 0.0) : nativeValue{ d } {
            std::basic_stringstream<CharT> ss{};
            ss << std::scientific << d;
            value = ss.str();
        }
        explicit NumberT(const StringType& s) : BuiltInT(impl::Validator<double>{}(s)){}
        explicit NumberT(StringType&& s) : BuiltInT(impl::Validator<double>{}(std::move(s))) {}
        double getNativeValue()const noexcept {
            return nativeValue;
        }
    private:
        double nativeValue;
    };

    template<typename CharT>
    class NullT : public BuiltInT<CharT> {
    public:
        explicit NullT() : BuiltInT<CharT>(StringType(LiteralsT<CharT>::value_null())) {}
        explicit NullT(const StringType& s) : BuiltInT(impl::Validator<nullptr_t>{}(s)) {}
        explicit NullT(StringType&& s) : BuiltInT(impl::Validator<nullptr_t>{}(std::move(s))) {}
        nullptr_t getNativeValue()const noexcept {
            return nullptr;
        }
        static const CharT* Literal() {
            return LiteralsT<CharT>::value_null();
        }
    };

    template<typename T> struct IsJSONType : public std::false_type {};
    template<typename C> struct IsJSONType<ObjectT<C>> : public std::true_type { using CharT = C; };
    template<typename C> struct IsJSONType<ArrayT<C>> : public std::true_type { using CharT = C; };
    template<typename C> struct IsJSONType<BuiltInT<C>> : public std::true_type { using CharT = C; };
    template<typename C> struct IsJSONType<BoolT<C>> : public std::true_type { using CharT = C; };
    template<typename C> struct IsJSONType<TrueT<C>> : public std::true_type { using CharT = C; };
    template<typename C> struct IsJSONType<FalseT<C>> : public std::true_type { using CharT = C; };
    template<typename C> struct IsJSONType<StringT<C>> : public std::true_type { using CharT = C; };
    template<typename C> struct IsJSONType<NumberT<C>> : public std::true_type { using CharT = C; };
    template<typename C> struct IsJSONType<NullT<C>> : public std::true_type { using CharT = C; };

    template<typename T,typename... Args>
    std::enable_if_t<
        IsJSONType<T>::value, 
        IObjectPtrT<typename IsJSONType<T>::CharT>
    > Create(Args&&... args) {
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


#endif //JSON_H_INCLUDED__
