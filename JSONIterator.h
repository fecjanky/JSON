#ifndef JSONITERATOR_H_INCLUDED__
#define JSONITERATOR_H_INCLUDED__

#include "JSONIteratorFwd.h"
#include "JSONObjects.h"

namespace JSON {
namespace impl {

struct ObjectIteratorState : public impl::IteratorState {
    using It = Object::Container::const_iterator;
    using UPtr = std::unique_ptr<impl::IteratorState>;

    explicit ObjectIteratorState(It it_ = It{});
    ObjectIteratorState(const ObjectIteratorState&) = default;
    ObjectIteratorState(ObjectIteratorState&&) = default;
    IObject& nextObj();
    static IObject& getObj(It i);
    UPtr clone() const  override;
    bool operator==(const IteratorState& i)const override;
    bool compare(const ObjectIteratorState& o)const override;

    It it;
};

struct ArrayIteratorState : public impl::IteratorState {
    using It = Array::Container::const_iterator;
    using UPtr = std::unique_ptr<impl::IteratorState>;

    explicit ArrayIteratorState(It it_ = It{});
    ArrayIteratorState(const ArrayIteratorState&) = default;
    IObject& nextObj();
    static IObject& getObj(It i);
    UPtr clone() const override;
    bool operator==(const IteratorState& i)const override;
    bool compare(const ArrayIteratorState& o)const override;

    It it;
};

} // namespace impl

inline Iterator::Iterator() : root{} 
{
}

inline Iterator::Iterator(const IObject& o, end_t) : root{&o} {
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
    stateful_visit<impl::ObjectIteratorState>(o);
}

inline void Iterator::visit(const Array& a)
{
    stateful_visit<impl::ArrayIteratorState>(a);
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

template<typename StateT, typename Obj>
inline void Iterator::stateful_visit(const Obj& o) 
{
    if (objStack.top().second) {
        auto& s = static_cast<StateT&>(*objStack.top().second);
        if (s.it == o.getValues().end()) {
            next_elem();
        }
        else {
            objStack.emplace(&s.nextObj(), nullptr);
        }
    }
    else if (!o.getValues().empty()) {
        auto& obj = StateT::getObj(o.getValues().begin());
        objStack.top().second = StatePtr(new StateT(++o.getValues().begin()));
        objStack.emplace(&obj, nullptr);
    }
    else {
        next_elem();
    }
}

namespace impl {
            
inline ObjectIteratorState::ObjectIteratorState(It it_) : it{ it_ } {}

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

inline ArrayIteratorState::ArrayIteratorState(It it_ ) : it{ it_ } {}
        
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
