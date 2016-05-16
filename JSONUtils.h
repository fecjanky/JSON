// Copyright (c) 2015 Ferenc Nandor Janky
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef JSONUTILS_H_
#define JSONUTILS_H_

#include <memory>  // unique_ptr
#include <type_traits>
#include <utility>

#include "JSONFwd.h"

namespace JSON {
namespace Utils{

template<typename... T>
struct TypeTester;

template<bool P, typename T1, typename T2>
struct If {
    using type = T2;
};

template<typename T1, typename T2>
struct If<true, T1, T2> {
    using type = T1;
};

template<bool P, typename T1, typename T2>
using If_t = typename If<P, T1, T2>::type;

template<typename Cloneable, typename D = std::default_delete<Cloneable>>
class CloneableUniquePtr : public std::unique_ptr<Cloneable, D> {
 private:
    template<typename T>
    struct PointerCheck {
        static constexpr bool value = std::is_polymorphic<Cloneable>{} ?
            std::is_convertible<T*, Cloneable*>::value :
            std::is_same<Cloneable, T>::value;
    };

    template<typename T, typename Check>
    struct FullCheck {
        static constexpr bool value = PointerCheck<T>::value && Check::value;
    };

    struct NoExceptComparable {
        static constexpr bool value = 
            noexcept(std::declval<Cloneable>().operator==(std::declval<Cloneable>()));
    };

    template<typename DT>
    using NECopyC = std::is_nothrow_copy_constructible<DT>;

    template<typename DT>
    using NECopyA = std::is_nothrow_copy_assignable<DT>;

    template<typename DT>
    using NEMoveC = std::is_nothrow_move_constructible<DT>;

    template<typename DT>
    using NEMoveA = std::is_nothrow_move_assignable<DT>;

    template<typename DT>
    struct DNonRefCheck {
        static constexpr bool value = 
            !std::is_reference<D>::value && std::is_same<DT, D>::value;
    };

    template<typename DT>
    struct DURefCheck {
        static constexpr bool value =
            (std::is_reference<D>::value &&
                std::is_same<
                std::remove_reference_t<DT>,
                std::remove_reference_t<D>
                >::value) ||
            (!std::is_reference<D>::value && 
              std::is_same<std::decay_t<DT>, D>::value);
    };

    template<typename T>
    struct HasClone {
        template<typename U>
        static std::enable_if_t<
            std::is_same<
            std::unique_ptr<T, D>,
            decltype(std::declval<U>().clone())>::value
            , std::true_type
        > test(decltype(std::declval<U>().clone())*);
        template<typename U>
        static std::false_type test(...);
        static constexpr bool value =
            std::is_same<decltype(test<T>(nullptr)), std::true_type>::value;
    };
    static_assert(HasClone<Cloneable>::value, "Type is not Cloneable");

 public:
    using Base = std::unique_ptr<Cloneable, D>;

    explicit CloneableUniquePtr(Cloneable* ptr = nullptr) : Base{ ptr } {}

    constexpr CloneableUniquePtr(decltype(nullptr) n) : Base{ n } {};

    template<typename T, typename = std::enable_if_t<PointerCheck<T>::value>>
    explicit CloneableUniquePtr(T* ptr) : Base{ ptr } {};

    CloneableUniquePtr(const CloneableUniquePtr& other) :
        Base(other ? other->clone() : nullptr) {
    }

    CloneableUniquePtr(CloneableUniquePtr&& other) noexcept(NEMoveC<D>::value) :
        Base(std::unique_ptr<Cloneable, D>(other.release(), std::move(other.get_deleter()))) {
    }

    CloneableUniquePtr& operator=(const CloneableUniquePtr& other) {
        if (other)
            this->Base::operator=(other->clone());
        else
            this->reset();
        return *this;
    }

    CloneableUniquePtr& operator=(CloneableUniquePtr&& other) noexcept(NEMoveA<D>::value) {
        this->Base::operator=(std::unique_ptr<Cloneable>(other.release(), std::move(other.get_deleter())));
        return *this;
    }

    template<typename T, typename DT, typename = std::enable_if_t<FullCheck<T, DURefCheck<DT>>::value>>
    CloneableUniquePtr(T* ptr, DT&& d) noexcept :
    Base{ ptr,std::forward<DT>(d) } {
        static_assert(NEMoveC<D>::value, 
            "Deleter is not no-throw copy constructible");
    };

    template<typename T, typename DT, typename = std::enable_if_t<FullCheck<T, DNonRefCheck<DT>>::value>>
    CloneableUniquePtr(T* ptr, const DT& d) noexcept :
    Base{ ptr,d } {
        static_assert(NECopyC<D>::value, 
            "Deleter is not no-throw moveconstructible");
    };

    template<typename T, typename DT, 
        typename = std::enable_if_t<PointerCheck<T>::value>>
    CloneableUniquePtr(const CloneableUniquePtr<T, DT>& other) :
        Base(other ? other->clone() : nullptr) {
    }

    template<typename T, typename DT, 
        typename = std::enable_if_t<PointerCheck<T>::value>>
    CloneableUniquePtr(CloneableUniquePtr<T, DT>&& other)
        noexcept(NEMoveC<D>::value && NEMoveC<DT>::value) :
        Base(std::unique_ptr<T, DT>(other.release(), std::move(other.get_deleter()))) {
    }

    template<typename T, typename DT, 
        typename = std::enable_if_t<PointerCheck<T>::value>>
    CloneableUniquePtr(const std::unique_ptr<T, DT>& other) :
        Base(other ? other->clone() : nullptr) {
    }

    template<typename T, typename DT, 
        typename = std::enable_if_t<PointerCheck<T>::value>>
    CloneableUniquePtr(std::unique_ptr<T, DT>&& other) noexcept(NEMoveC<D>::value) :
        Base(std::move(other)) {
    }

    template<typename T, typename DT, 
        typename = std::enable_if_t<PointerCheck<T>::value>>
    CloneableUniquePtr& operator=(const CloneableUniquePtr<T, DT>& other) {
        if (other)
            this->Base::operator=(other->clone());
        else
            this->reset();
        return *this;
    }

    template<typename T, typename DT,
        typename = std::enable_if_t<PointerCheck<T>::value>>
    CloneableUniquePtr& operator=(CloneableUniquePtr<T, DT>&& other)
        noexcept(NEMoveC<DT>::value && NEMoveA<D>::value) {
        this->Base::operator=(std::unique_ptr<T, DT>(other.release(), std::move(other.get_deleter())));
        return *this;
    }

    template<typename T, typename DT,
        typename = std::enable_if_t<PointerCheck<T>::value>>
    CloneableUniquePtr& operator=(std::unique_ptr<T, DT>&& other)
        noexcept(NEMoveA<D>::value) {
        this->Base::operator=(std::move(other));
        return *this;
    }

    bool operator==(const CloneableUniquePtr& rhs) const
        noexcept(NoExceptComparable::value) {
        if (!*this && !rhs)
            return true;
        else if ((!*this && rhs) || (*this && !rhs))
            return false;
        else
            return **this == *rhs;
    }

    bool operator!=(const CloneableUniquePtr& rhs) const
        noexcept(NoExceptComparable::value)
    {
        return !(*this == rhs);
    }
};

template<typename A>
struct AllocatedType;

template<typename T, template <typename> class A>
struct AllocatedType<A<T>> {
    using type = T;
};

template<typename A>
struct DefaultDeletePolicy {
    using T = typename AllocatedType<A>::type;
    static void Delete(A& a, T* ptr) noexcept {
        a.destroy(ptr);
        a.deallocate(ptr, 1);
    }
};

template<typename A>
struct DeallocDeletePolicy {
    using T = typename AllocatedType<A>::type;
    static void Delete(A& a, T* ptr) noexcept {
        a.deallocate(ptr, 1);
    }
};

template<
    typename Allocator,
    template <typename>class DeletePolicy = DefaultDeletePolicy
>
struct DeleterOf : public Allocator {

    using T = typename AllocatedType<Allocator>::type;

    static constexpr bool NECC = std::is_nothrow_copy_constructible<Allocator>::value;
    static constexpr bool NEMC = std::is_nothrow_move_constructible<Allocator>::value;
    static constexpr bool NECA = std::is_nothrow_copy_assignable<Allocator>::value;
    static constexpr bool NEMA = std::is_nothrow_move_assignable<Allocator>::value;
    static constexpr bool NEDC = std::is_nothrow_default_constructible<Allocator>::value;

    DeleterOf() noexcept(NEDC) = default;
    DeleterOf(const DeleterOf&) noexcept(NECC) = default;

    DeleterOf(DeleterOf&& d) noexcept : Allocator(std::move(d)) {
        static_assert(NEMC, "Allocator is not noexcept move constructible");
    }

    DeleterOf& operator=(const DeleterOf&) noexcept(NECA) = default;
    DeleterOf& operator=(DeleterOf&& d) noexcept {
        static_assert(NEMA, "Allocator is not noexcept move assignable");
        return this->Allocator::operator=(std::move(d));
    }

    explicit DeleterOf(const Allocator& a) noexcept(NECC)
        : Allocator(a) {}

    explicit DeleterOf(Allocator&& a) noexcept
        : Allocator(std::move(a)) {
        static_assert(NEMC, "Allocator is not noexcept move constructible");
    }

    // Consturction from other Deleter policies if they have
    // the same allocator type
    template<template <typename>class OtherDPolicy>
    DeleterOf(const DeleterOf<Allocator, OtherDPolicy>& d) noexcept(NECC)
        : Allocator(d)
    {
    }

    template<template <typename>class OtherDPolicy>
    DeleterOf(DeleterOf<Allocator, OtherDPolicy>&& d) noexcept
        : Allocator(std::move(d))
    {
        static_assert(NEMC, "Allocator is not noexcept move constructible");
    }

    // Assignment from other Deleter policies if they have
    // the same allocator type
    template<template <typename>class OtherDPolicy>
    DeleterOf& operator=(const DeleterOf<Allocator, OtherDPolicy>& d) noexcept(NECA)
    {
        return this->Allocator::operator=(d);
    }

    template<template <typename>class OtherDPolicy>
    DeleterOf& operator=(DeleterOf<Allocator, OtherDPolicy>&& d) noexcept
    {
        static_assert(NEMA, "Allocator is not noexcept move assignable");
        return this->Allocator::operator=(std::move(d));
    }

    //required operator()
    void operator()(T* ptr) {
        DeletePolicy<Allocator>::Delete(*this, ptr);
    }

};

template<
    template <typename T_, class D_>class SmartPtr = std::unique_ptr,
    template <typename> class Allocator = std::allocator
>
struct SmartPtrCreatorT {
    template<typename T>
    using DAllocator = DeleterOf<Allocator<T>>;

    template<typename T>
    using Ptr = SmartPtr<T, DAllocator<T>>;

    template<typename T>
    using DelAllocator = DeleterOf<Allocator<T>, DeallocDeletePolicy>;

    template<typename T>
    using SmartPtrDel = SmartPtr<T, DelAllocator<T>>;

    template<typename T>
    struct SmartPointerIsSafe {
        static constexpr bool value =
            std::is_nothrow_constructible<
            SmartPtrDel<T>, typename DelAllocator<T>::pointer, DelAllocator<T>&&
            >::value &&
            std::is_nothrow_constructible<
            Ptr<T>, typename DAllocator<T>::pointer, DAllocator<T>&&
            >::value &&
            noexcept(std::declval<SmartPtrDel<T>>().release()) &&
            noexcept(std::declval<SmartPtrDel<T>>().get_deleter());
    };

    template<typename T, typename A, typename... Args>
    static std::enable_if_t<
        std::is_same<std::decay_t<A>, Allocator<T>>::value,
        Ptr<T>
    > Create(A&& a, Args&&... args) {
        static_assert(SmartPointerIsSafe<T>::value, "SmartPtr is unsafe");
        DelAllocator<T> da{ std::forward<A>(a) };
        SmartPtrDel<T> p(da.allocate(1), std::move(da));
        p.get_deleter().construct(p.get(), std::forward<Args>(args)...);
        return Ptr<T>(p.release(), std::move(p.get_deleter()));
    }

    template<typename T, typename... Args>
    static Ptr<T> Create(Args&&... args) {
        using DelAllocator = DeleterOf<Allocator<T>, DeallocDeletePolicy>;
        return Create<T>(Allocator<T>{}, std::forward<Args>(args)...);
    }

};

using SmartPtrCreator = SmartPtrCreatorT<>;

namespace impl {
    template<class Impl, class... Ts>
    struct ForTypesMatchVisitorImpl;

    template<class Impl, class... Ts>
    struct ForTypesNoMatchVisitorImpl;

    template <class Impl, class VisitorIF >
    struct ForTypesNoMatchVisitorImplHelper;

    template<class Impl, class T>
    struct ForTypesNoMatchVisitorImpl<Impl, T> : public IVisitor {
        using TT = std::decay_t<T>;
        void visit(TT&) override { static_cast<Impl*>(this)->no_match(); };
        void visit(const TT&) override { static_cast<Impl*>(this)->no_match(); };
        void visit(TT&) const override { static_cast<const Impl*>(this)->no_match(); };
        void visit(const TT&) const override { static_cast<const Impl*>(this)->no_match(); };
    };

    template<class Impl, class T, class... Ts>
    struct ForTypesNoMatchVisitorImpl<Impl, T, Ts...> : public ForTypesNoMatchVisitorImpl<Impl, Ts...> {
        using TT = std::decay_t<T>;
        void visit(TT&) override { static_cast<Impl*>(this)->no_match(); };
        void visit(const TT&) override { static_cast<Impl*>(this)->no_match(); };
        void visit(TT&) const override { static_cast<const Impl*>(this)->no_match(); };
        void visit(const TT&) const override { static_cast<const Impl*>(this)->no_match(); };
    };

    template<class Impl, class T, class... Ts>
    struct ForTypesNoMatchVisitorImplHelper<Impl, JSON::impl::VisitorIF_Gen<T, Ts...>> : public
        ForTypesNoMatchVisitorImpl<Impl, T, Ts...>
    {
    };

    template<class Impl, class T>
    struct ForTypesMatchVisitorImpl<Impl, T> : public ForTypesNoMatchVisitorImplHelper<Impl, IVisitor> {
        using TT = std::decay_t<T>;
        void visit(TT&) override { static_cast<Impl*>(this)->match(); };
        void visit(const TT&) override { static_cast<Impl*>(this)->match(); };
        void visit(TT&) const override { static_cast<const Impl*>(this)->match(); };
        void visit(const TT&) const override { static_cast<const Impl*>(this)->match(); };
    };

    template<class Impl, class T, class... Ts>
    struct ForTypesMatchVisitorImpl<Impl, T, Ts...> : public ForTypesMatchVisitorImpl<Impl, Ts...> {
        using TT = std::decay_t<T>;
        void visit(TT&) override { static_cast<Impl*>(this)->match(); };
        void visit(const TT&) override { static_cast<Impl*>(this)->match(); };
        void visit(TT&) const override { static_cast<const Impl*>(this)->match(); };
        void visit(const TT&) const override { static_cast<const Impl*>(this)->match(); };
    };

}  //namespace impl

template<typename... Ts>
struct ForTypes : public impl::ForTypesMatchVisitorImpl<ForTypes<Ts...>,Ts...> {

    explicit ForTypes(const IObject& o_) : o{ o_ }
    {}
    void match() const {
        if(match_function)match_function();
    }
    void no_match() const {
        if(nomatch_function)nomatch_function();
    }

    template<typename MatchFunction, typename NoMatchFunction>
    void operator()(MatchFunction&& mf, NoMatchFunction&& nmf) && {
        match_function = std::forward<MatchFunction>(mf);
        nomatch_function = std::forward<NoMatchFunction>(nmf);
        o.accept(*this);
    }

    template<typename MatchFunction>
    void operator()(MatchFunction&& mf) && {
        match_function = std::forward<MatchFunction>(mf);
        o.accept(*this);
    }

private:
    const IObject& o;
    std::function<void(void)> match_function, nomatch_function;
};


}  // namespace Utils
}  // namespace JSON


#endif  // JSONUTILS_H_
