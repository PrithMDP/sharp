/**
 * @file TypeSet.hpp
 * @author Aaryaman Sagar
 */

#pragma once

#include <type_traits>

#include <sharp/Traits/Traits.hpp>

namespace sharp {

namespace detail {

    /**
     * Template higher order function to transform a type into its
     * corresponding untyped storage
     */
    template <typename Type>
    struct AlignedStorageFor {
        using type = std::aligned_storage_t<sizeof(Type), alignof(Type)>;
    };

} // namespace detail

/**
 * Forward declaration for the TypeSet class
 */
template <typename... Args>
class TypeSet {
public:

    /**
     * Default constructs the arguments provided to the type set into the
     * aligned storage, one by one
     */
    TypeSet();

    /**
     * Destroys all the objects stored in the type set
     */
    ~TypeSet();

private:

    /**
     * The data item that is of type std::tuple, the Transform_t algorithm
     * iterates through the type list provided as an argument and transforms
     * it into a type list that contains the results of applying the
     * transformation higher order function to each type in the input type
     * list.  The result is returned as a std::tuple, reference
     * sharp/Traits/detail/Algorithm.hpp
     */
    Transform_t<detail::AlignedStorageFor, Args...> aligned_tuple;
    static_assert(IsInstantiationOf_v<decltype(aligned_tuple), std::tuple>,
            "sharp::Transform_t returned a non std::tuple type list");
};

/**
 * @function get returns the instance of the given type from the TypeSet, this
 * function is similar in functionality to std::get<> that operates on a
 * std::tuple and returns a type from that tuple
 *
 * This function only participates in overload resolution if the type passed
 * to the function is an instantiation of type TypeSet
 *
 * The three overloads deal with const and movability with TypeSet instances
 *
 * The reason these are provided as non member functions is the same as the
 * reason the C++ standard library provides non member get<> functions for
 * std::tuple - If these were member functions, and code depended on a
 * template parameter, the member function would have to be prepended with the
 * "template" keyword.  Which is a nuisance.
 */
template <typename Type, typename... Types>
Type& get(sharp::TypeSet<Types...>&);
template <typename Type, typename... Types>
const Type& get(const sharp::TypeSet<Types>&);
template <typename Type, typename... Types>
Type&& get(sharp::TypeSet<Types>&&);

/**
 * @function collect_types Collects the arguments given into the type list
 * provided to the function, if there are missing arguments in the type list
 * provided as arguments they are default constructed into the type set.  The
 * others are moved into the type set
 *
 * A consequence of the above description is that the objects used with the
 * type set must be move constructible and are required to be default
 * constructible if the user does not intend to pass in an argument of that
 * type to the function
 */
template <typename... Types, typename... Args>
TypeSet<Types...> collect_into_type_set(Args&& args...);

} // namespace sharp
