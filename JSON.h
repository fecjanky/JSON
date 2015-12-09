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

struct IObject;
using IObjectPtr = std::unique_ptr<IObject>;

struct IObject {
    using IObjectPtr = JSON::IObjectPtr;
    using StringType = std::string;
    using OstreamT = std::ostream;

    virtual IObject& operator[](const StringType& key) = 0;
    virtual const IObject& operator[](const StringType& key) const = 0;

    virtual IObject& operator[](size_t index) = 0;
    virtual const IObject& operator[](size_t index) const = 0;

    virtual const StringType& getValue() const = 0;

    operator StringType() const
    {
        return getValue();
    }

    // Human readable
    virtual void serialize(StringType&& indentation, OstreamT&) const = 0;
    // Minimal whitespace (transmission syntax)
    virtual void serialize(OstreamT&) const = 0;

    virtual ~IObject() = default;
};


inline std::ostream& operator <<(std::ostream& os,const IObject& obj)
{
    obj.serialize(std::string { }, os);
    return os;
}


struct IAggregateObject: public IObject {
    using StringType = IObject::StringType;
    using IObjectPtr = IObject::IObjectPtr;
    // for arrays
    virtual void emplace(IObjectPtr && obj) = 0;
    // for objects
    virtual void emplace(StringType&& name, IObjectPtr && obj) = 0;

    virtual ~IAggregateObject() = default;
};

}

#endif //JSON_H_INCLUDED__
