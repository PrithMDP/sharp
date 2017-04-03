#pragma once

#include <tuple>
#include <type_traits>

#include <sharp/Traits/Traits.hpp>
#include <sharp/TypeSet/TypeSet.hpp>

namespace sharp {

namespace detail {

    /**
     * Executes the passed in function on the appropriate element in the
     * tuple.  This is determined by the context passed to the function in
     * combination with the type of the tuple.
     *
     * The ForEach trait calls a function with a context variable as the
     * argument.  This context variable stores the type of the thing the
     * iteration is on currently as a static member typedef, so it can be
     * fetched with (typename decltype(context)::type), after that the
     * location of this type is inspected in the tuple type and then
     * std::get<> is called with the appropriate index in the tuple and that
     * element is passed to the function passed
     */
    template <typename Context, typename TupleType, typename Func>
    decltype(auto) execute_on_appropriate_tuple_element(Context context,
                                                        TupleType& tup,
                                                        Func&& func) {
        // get the type that the current iteration is over
        using Type = typename decltype(context)::type;
        using TransformedType = typename AlignedStorageFor<Type>::type;

        // get the type of the tuple stored internally
        using Tuple = std::decay_t<decltype(tup)>;

        // then get the index of that type in the current tuple
        constexpr auto index_type = sharp::FindIndex_v<TransformedType, Tuple>;
        static_assert(index_type < std::tuple_size<Tuple>::value,
            "index_type out of bounds");

        // call the function on the storage, see header file for what the
        // storage is
        return std::forward<Func>(func)(std::get<index_type>(tup));
    }

} // namespace detail

template <typename... Types>
TypeSet<Types...>::TypeSet() {
    sharp::ForEach<std::tuple<Types...>>{}([&](auto context) mutable {
        // execute the function below on the right storage item, this storage
        // item will match the type context (which contains the type as a
        // typedef) passed
        detail::execute_on_appropriate_tuple_element(context,
                this->aligned_tuple, [&](auto& storage) {
            new (&storage) typename decltype(context)::type{};
        });
    });
}

template <typename... Types>
TypeSet<Types...>::~TypeSet() {
    sharp::ForEach<std::tuple<Types...>>{}([&](auto context) mutable {
        // execute the function below on the right storage item, this storage
        // item will match the type context (which contains the type as a
        // typedef) passed
        detail::execute_on_appropriate_tuple_element(context,
                this->aligned_tuple, [&](auto& storage) {
            using Type = typename decltype(context)::type;
            reinterpret_cast<Type*>(&storage)->~Type();
        });
    });
}

template <typename Type, typename... Types>
Type& get(TypeSet<Types...>& type_set) {
    static_assert(!std::is_same<Find_t<Type, std::tuple<Types...>>,
                               std::tuple<>>::value,
        "The type getting from should exist in the TypeSet");

    auto context = sharp::Identity<Type>{};
    return detail::execute_on_appropriate_tuple_element(context,
            type_set.aligned_tuple, [&](auto& storage) {
        using type = typename decltype(context)::type;
        return reinterpret_cast<type*>(&storage);
    });
}

template <typename Type, typename... Types>
const Type& get(const sharp::TypeSet<Types...>& type_set) {
    static_assert(!std::is_same<Find_t<Type, std::tuple<Types...>>,
                               std::tuple<>>::value,
        "The type getting from should exist in the TypeSet");

    auto context = sharp::Identity<Type>{};
    return detail::execute_on_appropriate_tuple_element(context,
            type_set.aligned_tuple, [&](auto& storage) {
        using type = typename decltype(context)::type;
        return reinterpret_cast<type*>(&storage);
    });
}

template <typename Type, typename... Types>
Type&& get(sharp::TypeSet<Types...>&& type_set) {
    static_assert(!std::is_same<Find_t<Type, std::tuple<Types...>>,
                               std::tuple<>>::value,
        "The type getting from should exist in the TypeSet");

    auto context = sharp::Identity<Type>{};
    return detail::execute_on_appropriate_tuple_element(context,
            type_set.aligned_tuple, [&](auto& storage) {
        using type = typename decltype(context)::type;
        return reinterpret_cast<type*>(&storage);
    });
}

template <typename Type, typename... Types>
const Type&& get(const sharp::TypeSet<Types...>&& type_set) {
    static_assert(!std::is_same<Find_t<Type, std::tuple<Types...>>,
                               std::tuple<>>::value,
        "The type getting from should exist in the TypeSet");

    auto context = sharp::Identity<Type>{};
    return detail::execute_on_appropriate_tuple_element(context,
            type_set.aligned_tuple, [&](auto& storage) {
        using type = typename decltype(context)::type;
        return reinterpret_cast<type*>(&storage);
    });
}

} // namespace sharp
