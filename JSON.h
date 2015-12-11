#ifndef JSON_H_INCLUDED__
#define JSON_H_INCLUDED__

#include <exception>
#include <memory>
#include <iostream> // std::ostream
#include <string>
#include <sstream>

#include "JSONFwd.h"

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
    // no whitespace (compact,transmission syntax)
    virtual void serialize(OstreamT&) const = 0;
    
    virtual bool compare(const Object&)  const {return false;}
    virtual bool compare(const Array&)  const {return false;}
    virtual bool compare(const Bool&)  const {return false;}
    virtual bool compare(const Null&) const {return false;}
    virtual bool compare(const Number&) const {return false;}
    virtual bool compare(const String&) const { return false; }
    virtual bool compare(const ObjectEntry&) const { return false; }
    virtual bool compare(const ArrayEntry&) const {return false;}
    virtual bool operator==(const IObject&) const = 0;
    bool operator!=(const IObject& o) const {
        return !(this->operator==(o));
    }

    struct IVisitor {
        virtual void visit(const Object&) { }
        virtual void visit(const ObjectEntry&) { }
        virtual void visit(const Array&) { }
        virtual void visit(const ArrayEntry&) { }
        virtual void visit(const True&) { }
        virtual void visit(const False&) { }
        virtual void visit(const Null&) { }
        virtual void visit(const Number&) { }
        virtual void visit(const String&) { }
        virtual ~IVisitor() = default;
    };

    virtual void accept(IVisitor&) = 0;
    virtual void accept(IVisitor&)const = 0;
    using iterator = Iterator;
    virtual iterator begin() const = 0;
    virtual iterator end() const = 0;

    virtual ~IObject() = default;
};

inline std::ostream& operator <<(std::ostream& os, const IObject& obj)
{
    obj.serialize(std::string{}, os);
    return os;
}

} //namespace JSON

#endif //JSON_H_INCLUDED__
