#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

#include <sharp/Traits/detail/Utility.hpp>

namespace sharp {

namespace detail {

    /**
     * Implementation of the for_each_tuple function
     */
    template <int current, int last>
    struct ForEachTupleImpl {

        template <typename TupleType, typename Func>
        static void impl(TupleType&& tup, Func& func) {

            // call the object at the given index
            func(std::get<current>(std::forward<TupleType>(tup)));

            // and then recurse
            ForEachTupleImpl<current + 1, last>::impl(
                    std::forward<TupleType>(tup), func);
        }
    };
    /**
     * No-op on last
     */
    template <int last>
    struct ForEachTupleImpl<last, last> {
        template <typename TupleType, typename Func>
        static void impl(TupleType&&, Func&) {}
    };

} // namespace detail

template <typename TupleType, typename Func>
Func for_each_tuple(TupleType&& tup, Func func) {

    // call the implementation function and then return the functor, similar
    // to std::for_each
    constexpr auto length = std::tuple_size<std::decay_t<TupleType>>::value;
    detail::ForEachTupleImpl<0, length>
        ::impl(std::forward<TupleType>(tup), func);

    return func;
}

template <typename TypeToMatch, typename Type>
decltype(auto) match_forward(std::remove_reference_t<Type>& instance) {


    // is the type TypeToMatch a const reference (or just const) type?
    constexpr auto is_const = std::is_const<
        std::remove_reference_t<TypeToMatch>>::value;

    // get the type to cast to based on the reference category of the
    // TypeToMatch
    using TypeToCast = std::conditional_t<
        std::is_lvalue_reference<TypeToMatch>::value,
        std::conditional_t<
            is_const,
            const std::remove_reference_t<Type>&,
            std::remove_reference_t<Type>&>,
        std::conditional_t<
            is_const,
            const std::remove_reference_t<Type>&&,
            std::remove_reference_t<Type>&&>>;

    // then cast to that type and then return
    return static_cast<TypeToCast>(instance);
}

template <typename TypeToMatch, typename Type>
decltype(auto) match_forward(std::remove_reference_t<Type>&& instance) {

    // if the instance is an rvalue reference then casting that to an lvalue
    // might cause a dangling reference based on the value category of the
    // original expression so assert against that
    static_assert(!std::is_lvalue_reference<TypeToMatch>::value,
            "Can not forward an rvalue as an lvalue");

    // get the type to cast to with the right const-ness
    using TypeToCast = std::conditional_t<
        std::is_const<std::remove_reference_t<TypeToMatch>>::value,
        const std::remove_reference_t<Type>&&,
        std::remove_reference_t<Type>&&>;

    // then cast the expression to an rvalue and return
    return static_cast<TypeToCast>(instance);
}

} // namespace sharp
