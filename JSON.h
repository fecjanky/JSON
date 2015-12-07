#ifndef JSON_H_INCLUDED__
#define JSON_H_INCLUDED__

#include <exception>
#include <memory>
#include <iostream>
#include <sstream>

// TODO: Mutable and Immutable objects
namespace JSON {

struct Exception: public std::exception {
};
struct AttributeMissing: JSON::Exception {
};
struct TypeError: public JSON::Exception {
};
struct AggregateTypeError: public JSON::Exception {
};
struct ValueError: public JSON::Exception {
};
struct OutOfRange: public JSON::Exception {
};
struct AttributeNotUnique: public JSON::Exception {
};

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
    using IObjectPtrT = JSON::IObjectPtrT<CharT>;
    using StringType = std::basic_string<CharT>;
    using OstreamT = std::basic_ostream<CharT>;

    virtual IObjectT& operator[](const StringType& key) = 0;
    virtual const IObjectT& operator[](const StringType& key) const = 0;

    virtual IObjectT& operator[](size_t index) = 0;
    virtual const IObjectT& operator[](size_t index) const = 0;

    virtual const StringType& getValue() const = 0;

    operator StringType() const
    {
        return getValue();
    }

    // Human readable
    virtual void serialize(StringType&& indentation, OstreamT&) const = 0;
    // Minimal whitespace (transmission syntax)
    virtual void serialize(OstreamT&) const = 0;

    virtual ~IObjectT() = default;
};

template<typename CharT>
inline std::basic_ostream<CharT>& operator <<(std::basic_ostream<CharT>& os,
        const IObjectT<CharT>& obj)
{
    obj.serialize(std::basic_string<CharT> { }, os);
    return os;
}

template<typename CharT>
struct IAggregateObjectT: public IObjectT<CharT> {
    using StringType = std::basic_string<CharT>;
    using IObjectPtr = JSON::IObjectPtrT<CharT>;
    // for arrays
    virtual void emplace(IObjectPtr && obj) = 0;
    // for objects
    virtual void emplace(StringType&& name, IObjectPtr && obj) = 0;

    virtual ~IAggregateObjectT() = default;
};

}

#endif //JSON_H_INCLUDED__
