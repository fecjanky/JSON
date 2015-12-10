#ifndef JSONOBJECTSFWD_H_INCLUDED__
#define JSONOBJECTSFWD_H_INCLUDED__

#include <unordered_map>
#include <utility> // pair
#include <vector> 

#include "JSON.h"

namespace JSON {
namespace impl {

template<class IF>
class IteratorIFImpl : public IF {
public:
    Iterator begin() const override;
    Iterator end() const override;
};

} //namespace impl

class Object : public impl::IteratorIFImpl<IAggregateObject> {
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

    Object();
    IObject& operator[](const StringType& key) override;
    const IObject& operator[](const StringType& key) const;
    IObject& operator[](size_t index) override;
    const IObject& operator[](size_t index) const;
    virtual const StringType& getValue() const;
    const Container& getValues() const;
    void emplace(IObjectPtr && obj) override;
    void emplace(StringType&& name, IObjectPtr && obj) override;
    void serialize(StringType&& indentation, OstreamT& os) const override;
    void serialize(OstreamT& os) const override;
    void accept(IVisitor& v) override;
    void accept(IVisitor& v)const override;
    bool operator==(const IObject&) const override;
    bool compare(const Object&)  const override;

private:
    Container values;
};

class Array : public impl::IteratorIFImpl<IAggregateObject> {
public:
    using StringType = IAggregateObject::StringType;
    using OstreamT = IAggregateObject::OstreamT;
    using IObjectPtr = IAggregateObject::IObjectPtr;
    using IObject = IAggregateObject::IObject;
    using Value = IObjectPtr;
    using Container = std::vector<Value>;
    using Literals = JSON::Literals;

    Array();
    IObject& operator[](const StringType& key) override;
    const IObject& operator[](const StringType& key) const override;
    IObject& operator[](size_t index) override;
    const IObject& operator[](size_t index) const override;
    virtual const StringType& getValue() const override;
    Container& getValues();
    const Container& getValues() const;
    void emplace(IObjectPtr&& obj) override;
    void emplace(StringType&& name, IObjectPtr&& obj) override;
    void serialize(StringType&& indentation, OstreamT& os) const override;
    void serialize(OstreamT& os) const override;
    void accept(IVisitor& v) override;
    void accept(IVisitor& v)const override;
    bool operator==(const IObject&) const override;
    bool compare(const Array&)  const override;

private:
    Container values;
};

class BuiltIn : public impl::IteratorIFImpl<IObject> {
public:

    using StringType = IObject::StringType;
    using OstreamT = IObject::OstreamT;
    using IObjectPtr = IObject::IObjectPtr;
    using Literals = JSON::Literals;

    IObject& operator[](const StringType& key) override;
    const IObject& operator[](const StringType& key) const override;
    IObject& operator[](size_t index) override;
    const IObject& operator[](size_t index) const override;
    virtual const StringType& getValue() const override;
    void serialize(StringType&&, OstreamT& os) const override;
    void serialize(OstreamT& os) const override;
protected:
    BuiltIn();
    BuiltIn(const StringType& s);
    BuiltIn(StringType&& s);
    StringType value;
};

class Bool : public BuiltIn {
public:

    using StringType = BuiltIn::StringType;
    using Literals = JSON::Literals;

    bool getNativeValue() const noexcept;

    bool operator==(const IObject&) const override;
    bool compare(const Bool&)  const override;

protected:
    explicit Bool(bool b = false);
    explicit Bool(const StringType& s);
    explicit Bool(StringType&& s);
    explicit Bool(const char * s);
private:
    bool nativeValue;
};


class True : public Bool {
public:
    using StringType = Bool::StringType;

    True();
    explicit True(const StringType& s);
    explicit True(StringType&& s);
    static const char* Literal();
    void accept(IVisitor& v) override;
    void accept(IVisitor& v)const override;

};

class False : public Bool {
public:
    using StringType = Bool::StringType;

    False();
    explicit False(const StringType& s);
    explicit False(StringType&& s);
    static const char* Literal();
    void accept(IVisitor& v) override;
    void accept(IVisitor& v)const override;
};

class String : public BuiltIn {
public:
    using StringType = BuiltIn::StringType;
    using OstreamT = BuiltIn::OstreamT;

    String();
    explicit String(const StringType& t);
    explicit String(StringType&& t);
    void serialize(StringType&&, OstreamT& os) const override;
    void serialize(OstreamT& os) const override;
    void accept(IVisitor& v) override;
    void accept(IVisitor& v)const override;

    bool operator==(const IObject&) const override;
    bool compare(const String&)  const override;

};

class Number : public BuiltIn {
public:
    using StringType = BuiltIn::StringType;

    explicit Number(double d = 0.0);
    explicit Number(const StringType& s);
    explicit Number(StringType&& s);
    double getNativeValue() const noexcept;
    void accept(IVisitor& v) override;
    void accept(IVisitor& v)const override;
    bool operator==(const IObject&) const override;
    bool compare(const Number&)  const override;

private:
    double nativeValue;
};

class Null : public BuiltIn {
public:
    using StringType = BuiltIn::StringType;

    explicit Null();
    explicit Null(const StringType& s);
    explicit Null(StringType&& s);
    nullptr_t getNativeValue() const noexcept;
    static const char* Literal();
    void accept(IVisitor& v) override;
    void accept(IVisitor& v)const override;
    bool operator==(const IObject&) const override;
    bool compare(const Null&)  const override;
};

template<typename T, typename B = void> struct IsJSONType : public std::false_type {
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
    
} //namespace JSON

#endif //JSONOBJECTSFWD_H_INCLUDED__
