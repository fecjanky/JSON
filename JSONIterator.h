#ifndef JSONITERATOR_H_INCLUDED__
#define JSONITERATOR_H_INCLUDED__

#include "JSONIteratorFwd.h"
#include "JSONObjects.h"

namespace JSON {
namespace impl {

struct ObjectIteratorState : public impl::IteratorState, public ObjectEntry {
    using It = Object::Container::const_iterator;
    using UPtr = std::unique_ptr<impl::IteratorState>;

    ObjectIteratorState(It it_,const Object&);
    ObjectIteratorState(const ObjectIteratorState&) = default;
    ObjectIteratorState(ObjectIteratorState&&) = default;
    IObject& nextObj();
    static IObject& getObj(It i);
    UPtr clone() const  override;
    bool operator==(const IteratorState& i)const override;
    bool compare(const ObjectIteratorState& o)const override;

};

struct ArrayIteratorState : public impl::IteratorState, public ArrayEntry {
    using It = Array::Container::const_iterator;
    using UPtr = std::unique_ptr<impl::IteratorState>;

    ArrayIteratorState(It it,const Array&);
    ArrayIteratorState(const ArrayIteratorState&) = default;
    IObject& nextObj();
    static IObject& getObj(It i);
    UPtr clone() const override;
    bool operator==(const IteratorState& i)const override;
    bool compare(const ArrayIteratorState& o)const override;
};

struct IsAggregateObject : public IObject::IVisitor {
    IsAggregateObject(const IObject& o_) : o{ o_ }, b {}
    {
        o.accept(*this);
    }
    operator bool()const noexcept
    {
        return b;
    }
private:
    void visit(const Object&o) override
    {
        b = true;
    }
    void visit(const Array& a) override
    {
        b = true;
    }
    const IObject& o;
    bool b;
};

} // namespace impl

inline Iterator::Iterator() : root{} 
{
}

inline Iterator::Iterator(const IObject& o, end_t) : root{&o} 
{
}

inline Iterator::Iterator(const IObject& o) : root{&o}
{
    objStack.emplace(&o, nullptr);
}

inline const IObject& Iterator::operator*()
{
    return *objStack.top().first;
}
    
inline Iterator& Iterator::operator++()
{
    objStack.top().first->accept(*this);
    return *this;
}
    
inline Iterator Iterator::operator++(int)
{
    Iterator temp = *this;
    ++*this;
    return temp;
}

inline const IObject* Iterator::operator->()
{
    return objStack.top().first;
}
    
inline bool Iterator::operator==(const Iterator& rhs)const
{
    return  root == rhs.root && objStack == rhs.objStack;
}
    
inline bool Iterator::operator!=(const Iterator& rhs)const
{
    return !(*this == rhs);
}

    
inline void Iterator::visit(const Object& o)
{
    statefulVisit(o);
}

inline void Iterator::visit(const ObjectEntry& o)
{
    statefulVisitEntry(o);
}

inline void Iterator::visit(const Array& a)
{
    statefulVisit(a);
}

inline void Iterator::visit(const ArrayEntry& o)
{
    statefulVisitEntry(o);
}

inline void Iterator::visit(const True&)
{
    next_elem();
}

inline void Iterator::visit(const False&)
{
    next_elem();
}

inline void Iterator::visit(const Null&)
{
    next_elem();
}

inline void Iterator::visit(const Number&)
{
    next_elem();
}

inline void Iterator::visit(const String&)
{
    next_elem();
}

inline void Iterator::next_elem() 
{
    objStack.pop();
    if (!objStack.empty()) {
        objStack.top().first->accept(*this);
    }
}


template< typename Obj>
inline void Iterator::statefulVisit(const Obj& o)
{
    using StateT = typename ObjTraits<Obj>::State;
    if (!objStack.top().second && !o.getValues().empty()) {
        objStack.top().second =
            StatePtr(new StateT(o.getValues().begin(), o));
        auto& next_obj =
            static_cast<StateT&>(*objStack.top().second);
        objStack.emplace(&next_obj, nullptr);
    } else
        next_elem();

}

template<typename Obj>
inline void Iterator::statefulVisitEntry(const Obj& o)
{
    if (o.it == o.parent.getValues().end()) {
        objStack.pop();
    } else {
        auto& obj = o.obj();
        ++o.it;
        if (impl::IsAggregateObject(obj))
            objStack.emplace(&obj, nullptr);
        else if (o.it == o.parent.getValues().end()) {
            objStack.pop();
        }
    }
}



namespace impl {
            
inline ObjectIteratorState::ObjectIteratorState(It it,const Object& o) : 
    ObjectEntry(it,o) 
{
}

inline IObject& ObjectIteratorState::nextObj()
{
    return *(it++->second);
}
        
inline IObject& ObjectIteratorState::getObj(It i)
{
    return *i->second;
}
        
inline ObjectIteratorState::UPtr ObjectIteratorState::clone() const
{
    return UPtr(new ObjectIteratorState(*this));
}
        
inline bool ObjectIteratorState::operator==(const IteratorState& i)const
{
    return i.compare(*this);
}
            
inline bool ObjectIteratorState::compare(const ObjectIteratorState& o)const
{
    return it == o.it;
}

inline ArrayIteratorState::ArrayIteratorState(It it, const Array& a) 
    : ArrayEntry(it,a) 
{
}
        
inline IObject& ArrayIteratorState::nextObj()
{
    return **(it++);
}
        
inline IObject& ArrayIteratorState::getObj(It i)
{
    return **(i);
}
        
inline ArrayIteratorState::UPtr ArrayIteratorState::clone() const
{
    return UPtr(new ArrayIteratorState(*this));
}

inline bool ArrayIteratorState::operator==(const IteratorState& i)const
{
    return i.compare(*this);
}

inline bool ArrayIteratorState::compare(const ArrayIteratorState& o)const
{
    return it == o.it;
}

inline bool IteratorState::compare(const ObjectIteratorState&)const {
    return false;
}

inline bool IteratorState::compare(const ArrayIteratorState&)const {
    return false;
}

}// namepsace impl
}// namespace JSON

#endif //JSONITERATOR_H_INCLUDED__
