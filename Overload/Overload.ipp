#pragma once

#include <sharp/Overload/Overload.hpp>
#include <sharp/Traits/Traits.hpp>

#include <utility>
#include <type_traits>
#include <tuple>

namespace sharp {
namespace overload_detail {

    /**
     * return true if the type is a function pointer type
     */
    template <typename Func>
    struct IsFunctionPtr : public std::integral_constant<bool, false> {};
    template <typename ReturnType, typename... Args>
    struct IsFunctionPtr<ReturnType (*) (Args...)>
        : public std::integral_constant<bool, true> {};

    /**
     * Enable if the function is a function pointer type
     */
    template <typename Func>
    using EnableIfNotFunctionPointerType = std::enable_if_t<
        !IsFunctionPtr<std::decay_t<Func>>::value>;

    /**
     * The overload detector for function pointers
     */
    template <int current, typename... Tail>
    class FunctionOverloadDetector;
    template <int current,
              typename ReturnType, typename... Args,
              typename... Tail>
    class FunctionOverloadDetector<current, ReturnType (*) (Args...), Tail...> {
    public:

        /**
         * Declare the current impl, use the return type as a guide for which
         * overload was called
         *
         * And then import all the other impl functions as well
         */
        static std::integral_constant<int, current> impl(Args...);
        using FunctionOverloadDetector<current + 1, Tail...>::impl;
    };

    /**
     * The base overload template that is an incomplete type, this should
     * never be instantiated.  And even if it is, it should cause a hard error
     */
    template <typename... Funcs>
    class Overload;

    /**
     * The general case, in this case the Overload class imports the
     * operator() method of the current functor class, and then also imports
     * the operator() recursively on the rest of the type list
     *
     * As a result in the first case of the recursion, or in the most derived
     * class.  The most derived class will have imported all the important
     * operator() methods into the current scope
     */
    template <typename Func, typename... Funcs>
    class Overload<Func, Funcs...>
            : public Func,
              public Overload<Funcs...> {
    public:
        template <typename F, typename... Fs>
        explicit Overload(F&& f, Fs&&... fs)
            : Func{std::forward<F>(f)},
            Overload<Funcs...>{std::forward<Fs>(fs)...} {}

        /**
         * Import operator() of the current functor, whatever that may be
         * based on the type of Func and then recurse and import the
         * operator() aggregated recursively
         */
        using Func::operator();
        using Overload<Funcs...>::operator();
    };

    template <typename Func>
    class Overload<Func> : public Func {
    public:
        template <typename F>
        explicit Overload(F&& f) : Func{std::forward<F>(f)} {}

        /**
         * Import
         */
        using Func::operator();
    };

    /**
     * Base case, use the overload resolution helper to deduce the return type
     * index and then use that to call the appropriate function
     */
    template <typename ReturnType, typename... Args, typename... Tail>
    class Overload<ReturnType (*) (Args...), Tail...> {
    public:

        using Head = ReturnType (*) (Args...);

        template <typename... FPtrs>
        explicit Overload(FPtrs&&... fs) {

            // store the function pointers
            using FPtrTupleType = decltype(this->function_pointers);
            this->function_pointers = FPtrTupleType{fs...};
        }

        template <typename... Ts>
        decltype(auto) operator()(Ts&&... args) {

            // get the index with the function overload detector
            using IndexType =
                decltype(FunctionOverloadDetector<0, Head, Tail...>
                        ::impl(std::forward<Ts>(args)...));

            // and then call the appropriate function
            return std::get<IndexType::value>(this->function_pointers)(
                    std::forward<Ts>(args)...);
        }

    private:

        /**
         * All the function pointers stored here
         */
        std::tuple<Head, Tail...> function_pointers;
    };

    /**
     * A trait that gets a value list corresponding to all the functors in the
     * type list
     */
    /**
     * Base case
     */
    template <typename FunctorVList, typename FPtrVList, int current,
              typename... Funcs>
    struct SplitLists {
        static_assert(current >= 0, "");
        using type = std::pair<FunctorVList, FPtrVList>;
    };
    template <typename FunctorVList, typename FPtrVList,
              int current,
              typename Head, typename... Funcs>
    struct SplitLists<FunctorVList, FPtrVList, current, Head, Funcs...> {
        static_assert(current >= 0, "");

        // in the default case the head is a functor so concatenate the empty
        // value list with 0
        using NewFunctorVList = Concatenate_t<FunctorVList, ValueList<current>>;
        using NewFPtrVList = FPtrVList;

        using Next = SplitLists<NewFunctorVList, NewFPtrVList, current + 1,
                                Funcs...>;
        using type = typename Next::type;
    };
    /**
     * Specialization for function pointers
     */
    template <typename FunctorVList, typename FPtrVList,
              int current,
              typename ReturnType, typename... Args, typename... Funcs>
    struct SplitLists<FunctorVList, FPtrVList, current,
                      ReturnType (*) (Args...), Funcs...> {
        static_assert(current >= 0, "");

        // in the default case the head is a functor so concatenate the empty
        // value list with 0
        using NewFunctorVList = FunctorVList;
        using NewFPtrVList = Concatenate_t<FPtrVList, ValueList<current>>;

        using Next = SplitLists<NewFunctorVList, NewFPtrVList, current + 1,
                                Funcs...>;
        using type = typename Next::type;
    };

    /**
     * A trait that gets a value list corresponding to all the function
     * pointers in the type list
     */
    template <typename...> struct WhichType;
    template <typename TupleType>
    struct SplitFunctorAndFunctionPointers;
    template <typename... Args>
    struct SplitFunctorAndFunctionPointers<std::tuple<Args...>> {

        /**
         * split the lists into functor value lists and function pointer value
         * lists
         */
        using SplitValueLists = typename SplitLists<
            ValueList<>, ValueList<>, 0, std::decay_t<Args>...>::type;

        // get the individual value lists from the aggregate type returned
        using FunctorVList = typename SplitValueLists::first_type;
        using FPtrVList = typename SplitValueLists::second_type;

        template <typename TupleType>
        static auto impl(TupleType&& tup) {
            return impl(std::forward<TupleType>(tup), FunctorVList{},
                    FPtrVList{});
        }

    private:
        template <typename TupleType, int... IndicesFunctor, int... IndicesFPtr>
        static auto impl(TupleType&& tup,
                  ValueList<IndicesFunctor...>,
                  ValueList<IndicesFPtr...>) {

            using TupleTypeDecayed = std::decay_t<TupleType>;
            using TupleTypeToReturn = std::tuple<
                std::tuple_element_t<IndicesFunctor, TupleTypeDecayed>...,
                std::tuple_element_t<IndicesFPtr, TupleTypeDecayed>...>;

            // use std::move to aid in reference collapsing and keep the
            // referencesness of the tuple element type
            //
            // this works because the && transformation is a identity
            // transformation, i.e.
            //  int&& && -> int&&,
            //  int& && -> int&,
            return TupleTypeToReturn{
                std::get<IndicesFunctor>(std::move(tup))...,
                std::get<IndicesFPtr>(std::move(tup))...};
        }
    };

    template <typename... FuncArgs, int... Indices>
    auto make_overload_impl(std::tuple<FuncArgs...>& args,
                            std::integer_sequence<int, Indices...>) {
        return overload_detail::Overload<std::decay_t<FuncArgs>...>{
            std::get<Indices>(std::move(args))...};
    }

} // namespace overload_detail

template <typename... Funcs>
auto make_overload(Funcs&&... funcs) {
    // pack the arguments in a tuple
    auto args = std::forward_as_tuple(std::forward<Funcs>(funcs)...);
    using FuncTypes = decltype(args);

    // split the arguments based on whether they are function pointers or
    // functors, with function pointers being at the end
    auto functor_function_ptrs =
        overload_detail::SplitFunctorAndFunctionPointers<FuncTypes>::impl(args);

    constexpr auto size = std::tuple_size<FuncTypes>::value;
    return overload_detail::make_overload_impl(functor_function_ptrs,
            std::make_integer_sequence<int, size>{});
}

} // namespace sharp
