/**
 * @file Utility.hpp
 * @author Aaryaman Sagar
 *
 * Functions and traits that do not fit in any of the files in the module
 * above, so they are dumped here
 */

#pragma once

#include <type_traits>
#include <tuple>

#include <sharp/Traits/detail/IsInstantiationOf.hpp>

namespace sharp {

/**
 * @class ValueList
 *
 * A constexpr value container, this can be used to store values at compile
 * time.  At the moment before C++17 this only stores integer values
 */
template <int... values>
struct ValueList {};

/**
 * @class End
 *
 * A tag that denotes the end of a type list range, similar to std::end() this
 * marks the end of a type list range.  This is used in cases where an
 * algorithm returns past the end of a range to denote that a value could not
 * be found.
 *
 * For example if the predicate passed to FindIf returns true for none of the
 * types then the algorithm returns an End tag to denote failure
 */
struct End {};

namespace detail {

    /**
     * Implemenation for the concatenate trait
     */
    template <typename TypesContainerOne, typename TypesContainerTwo>
    struct ConcatenateImpl;
    template <typename... TypesOne, typename... TypesTwo>
    struct ConcatenateImpl<std::tuple<TypesOne...>, std::tuple<TypesTwo...>> {
        using type = std::tuple<TypesOne..., TypesTwo...>;
    };
    template <int... integers_one, int... integers_two>
    struct ConcatenateImpl<ValueList<integers_one...>,
                           ValueList<integers_two...>> {
        using type = ValueList<integers_one..., integers_two...>;
    };

    /**
     * Implementation for the PopFront trait
     */
    template <typename TypesContainer>
    struct PopFrontImpl;
    template <typename Head, typename... Tail>
    struct PopFrontImpl<std::tuple<Head, Tail...>> {
        using type = std::tuple<Tail...>;
    };
    template <>
    struct PopFrontImpl<std::tuple<>> {
        using type = std::tuple<>;
    };
    template <>
    struct PopFrontImpl<End> {
        using type = std::tuple<>;
    };

    /**
     * Implementation for the erase trait
     */
    template <int to_erase, typename TypesContainer>
    struct EraseImpl;
    template <int to_erase, typename... Types>
    struct EraseImpl<to_erase, std::tuple<Types...>> {
        using type = std::tuple<Types...>;
    };
    template <int to_erase, typename Head, typename... Types>
    struct EraseImpl<to_erase, std::tuple<Head, Types...>> {

        static_assert(to_erase > 0, "Something went wrong in the "
                "implementation of the Erase trait");

        using type = typename ConcatenateImpl<
            std::tuple<Head>,
            typename EraseImpl<to_erase - 1, std::tuple<Types...>>::type>::type;
    };
    template <typename Head, typename... Types>
    struct EraseImpl<0, std::tuple<Head, Types...>> {
        using type = std::tuple<Types...>;
    };

    /**
     * Implemenation for the ConcatenateN trait
     */
    template <typename TypeToRepeat, int n>
    struct ConcatenateNImpl {
        using type = typename ConcatenateImpl<
            std::tuple<TypeToRepeat>,
            typename ConcatenateNImpl<TypeToRepeat, n - 1>::type>::type;
    };
    template <typename TypeToRepeat>
    struct ConcatenateNImpl<TypeToRepeat, 0> {
        using type = std::tuple<>;
    };

    /**
     * Implementation for the MatchForwardingReference trait
     */
    template <typename TypeToMatch, typename Type>
    struct MatchForwardingReferenceImpl;
    template <typename TypeToMatch, typename Type>
    struct MatchForwardingReferenceImpl<TypeToMatch&, Type> {
        using type = Type&;
    };
    template <typename TypeToMatch, typename Type>
    struct MatchForwardingReferenceImpl<const TypeToMatch&, Type> {
        using type = const Type&;
    };
    template <typename TypeToMatch, typename Type>
    struct MatchForwardingReferenceImpl<TypeToMatch&&, Type> {
        using type = Type&&;
    };
    template <typename TypeToMatch, typename Type>
    struct MatchForwardingReferenceImpl<const TypeToMatch&&, Type> {
        using type = const Type&&;
    };

} // namespace detail

/**
 * @class Concatenate
 *
 * concatenates two type lists or two value lists, type lists are supported as
 * std::tuples and value lists are supported as sharp::ValueList
 */
template <typename TypesContainerOne, typename TypesContainerTwo>
struct Concatenate {
    using type = typename detail::ConcatenateImpl<TypesContainerOne,
                                                  TypesContainerTwo>::type;
};

/**
 * @class ConcatenateN
 *
 * Concatenates the same type n types to result in a tuple of length n with
 * the same type repeated n times
 */
template <typename TypeToRepeat, int n>
struct ConcatenateN {
    using type = typename detail::ConcatenateNImpl<TypeToRepeat, n>::type;
};

/**
 * @class PopFront
 *
 * Pops the first type out of the type list container and returns the rest of
 * the type container
 */
template <typename TypesContainer>
struct PopFront {
    using type = typename detail::PopFrontImpl<TypesContainer>::type;
};

/**
 * @class Erase
 *
 * Erases the element at the given index from the type list and returns the
 * type list wrapped in a std::tuple
 */
template <int to_erase, typename TypesContainer>
struct Erase {
    using type = typename detail::EraseImpl<to_erase, TypesContainer>::type;
};

/**
 * @function for_each_tuple
 *
 * Iterates through all the values in a tuple and executes the passed in
 * functor on each value
 *
 * The tuple argument is passed in by forwarding reference to reduce the code
 * duplication that would have resulted in attempted rvalue generalization of
 * this function
 *
 * The types of functions that can be used include one having a just a
 * polymorphic first type (i.e.  templated so that it can accept tuple type
 * instances) and another accepting an int as the second parameter
 */
template <typename TupleType, typename Func>
Func for_each_tuple(TupleType&& tup, Func func);

/**
 * @class Identity
 *
 * An empty class with nothing but a member typedef, this is used to simplify
 * passing type parameters around to lambdas, objects of this type can be
 * passed to polymorphic lambdas as parameters and that can be used to deduce
 * the types passed to the lambda
 */
template <typename Type>
struct Identity {
    using type = Type;
};

/**
 * @class MatchForwardingReference
 *
 * This class can be used to match the reference-ness of a forwarding
 * reference on another type.  For example if the tuple is && and you want to
 * return the tuple's member, you would want the && to match in std::get()'s
 * return value, as a result, this has slightly different usage semantics as
 * compared to std::forward.  For safety.  You should only call this with
 * with the full type of the forwarding reference, and include the two &&
 */
template <typename TypeToMatch, typename Type>
struct MatchForwardingReference {
    using type = typename detail::MatchForwardingReferenceImpl<TypeToMatch,
                                                               Type>::type;
};

/**
 * Conventional typedefs, these end in the suffix _t, this is keeping in
 * convention with the C++ standard library features post and including C++17
 */
template <typename TypesContainerOne, typename TypesContainerTwo>
using Concatenate_t = typename Concatenate<TypesContainerOne, TypesContainerTwo>
    ::type;
template <typename TypeToRepeat, int n>
using ConcatenateN_t = typename ConcatenateN<TypeToRepeat, n>::type;
template <typename TypesContainer>
using PopFront_t = typename PopFront<TypesContainer>::type;
template <int to_erase, typename... Types>
using Erase_t = typename Erase<to_erase, Types...>::type;
template <typename TypeToMatch, typename Type>
using MatchForwardingReference_t = typename MatchForwardingReference<
    TypeToMatch, Type>::type;

/**
 * Tests for Concatenate
 */
static_assert(std::is_same<Concatenate_t<std::tuple<int>, std::tuple<double>>,
                                         std::tuple<int, double>>::value,
        "sharp::Concatenate tests failed!");
static_assert(std::is_same<Concatenate_t<ValueList<0>, ValueList<1>>,
                                         ValueList<0, 1>>::value,
        "sharp::Concatenate tests failed!");

/**
 * Tests for PopFront
 */
static_assert(std::is_same<PopFront_t<std::tuple<int, double>>,
                           std::tuple<double>>::value,
    "sharp::PopFront tests failed!");
static_assert(std::is_same<PopFront_t<std::tuple<double>>,
                           std::tuple<>>::value,
    "sharp::PopFront tests failed!");
static_assert(std::is_same<PopFront_t<std::tuple<>>,
                           std::tuple<>>::value,
    "sharp::PopFront tests failed!");

/**
 * Tests for Erase
 */
static_assert(std::is_same<Erase_t<0, std::tuple<int, double, char>>,
                           std::tuple<double, char>>::value,
    "sharp::Erase tests failed");
static_assert(std::is_same<Erase_t<1, std::tuple<int, double, char>>,
                           std::tuple<int, char>>::value,
    "sharp::Erase tests failed");
static_assert(std::is_same<Erase_t<2, std::tuple<int, double, char>>,
                           std::tuple<int, double>>::value,
    "sharp::Erase tests failed");
static_assert(std::is_same<Erase_t<0, std::tuple<>>, std::tuple<>>::value,
    "sharp::Erase tests failed");
static_assert(std::is_same<Erase_t<1, std::tuple<>>, std::tuple<>>::value,
    "sharp::Erase tests failed");

/**
 * Tests for ConcatenateN
 */
static_assert(std::is_same<ConcatenateN_t<int, 3>,
                           std::tuple<int, int, int>>::value,
    "sharp::detail::RepeatN tests failed");
static_assert(std::is_same<ConcatenateN_t<int, 0>,
                           std::tuple<>>::value,
    "sharp::detail::RepeatN tests failed");

static_assert(std::is_same<ConcatenateN_t<int, 1>,
                           std::tuple<int>>::value,
    "sharp::detail::RepeatN tests failed");

/**
 * Tests for MatchForwardingReference
 */
static_assert(std::is_same<MatchForwardingReference_t<int&, double>, double&>
        ::value, "sharp::MatchForwardingReference tests failed");
static_assert(std::is_same<MatchForwardingReference_t<const int&, double>,
                           const double&>
        ::value, "sharp::MatchForwardingReference tests failed");
static_assert(std::is_same<MatchForwardingReference_t<int&&, double>, double&&>
        ::value, "sharp::MatchForwardingReference tests failed");
static_assert(std::is_same<MatchForwardingReference_t<const int&&, double>,
                           const double&&>
        ::value, "sharp::MatchForwardingReference tests failed");


} // namespace sharp

#include <sharp/Traits/detail/Utility.ipp>
