#pragma once

#include <sharp/ForEach/ForEach.hpp>
#include <sharp/Traits/Traits.hpp>

#include <cstdint>
#include <tuple>
#include <type_traits>
#include <utility>
#include <iterator>
#include <array>

namespace sharp {

namespace for_each_detail {

/**
 * An internal namespace that contains functions that do ADL lookups, for this
 * reason those functions that are looked up via ADL must be imported in the
 * namespace to provide backup in case the passed type does not contain
 * ADL defined functions
 */
namespace adl {

    /**
     * Import the 3 things that can be ADL defined for this library
     */
    using std::get;
    using std::begin;
    using std::end;

    /**
     * Function to simulate adl lookup on the begin, end and get<> functions
     */
    template <typename Range>
    auto adl_begin(Range&& range) -> decltype(begin(std::declval<Range>())) {
        return begin(std::forward<Range>(range));
    }
    template <typename Range>
    auto adl_end(Range&& range) -> decltype(end(std::declval<Range>())) {
        return end(range);
    }
    template <std::size_t Index, typename Range>
    auto adl_get(Range&& range) -> decltype(get<Index>(std::declval<Range>())) {
        return get<Index>(std::forward<Range>(range));
    }

} // namespace adl

    /**
     * Concepts to check if the range has a member get<> or if it has a free
     * function get
     */
    template <typename Range>
    using EnableIfNonMemberGetWorks = sharp::void_t<
        decltype(adl::adl_get<0>(std::declval<Range>()))>;
    template <typename Range>
    using EnableIfMemberGetWorks = sharp::void_t<
        decltype(std::declval<Range>().template get<0>())>;

    /**
     * Helper to either retrieve the value through a member get method or a
     * free get function
     */
    template <std::size_t Index, typename Range, typename = sharp::void_t<>>
    class Get {
    public:
        template <typename R>
        static auto impl(R&& range)
                -> decltype(adl::adl_get<Index>(std::declval<R>())) {
            return adl::adl_get<Index>(std::forward<R>(range));
        }
    };
    template <std::size_t Index, typename Range>
    class Get<Index, Range, EnableIfMemberGetWorks<Range>> {
    public:
        template <typename R>
        static auto impl(R&& range)
                -> decltype(adl::adl_get<Index>(std::declval<R>())) {
            return std::forward<R>(range).template get<Index>();
        }
    };

    /**
     * Concepts to check if the range is a compile time range or a runtime
     * range
     */
    template <typename Range>
    using EnableIfCompileTimeRange = sharp::void_t<
        decltype(Get<0, Range>::impl(std::declval<Range>())),
        decltype(std::tuple_size<Range>::value)>;
    template <typename Range>
    using EnableIfRuntimeRange = sharp::void_t<
        decltype(adl::adl_begin(std::declval<Range>())),
        decltype(adl::adl_end(std::declval<Range>()))>;

    /**
     * A simple overloaded trait to simulate the result of declval on the
     * range, this works both for compile time ranges (tuple like) and on
     * runtime ranges
     */
    template <typename Range, typename = sharp::void_t<>>
    class DeclvalSequence {
    public:
        static auto impl()
            -> decltype(*(adl::adl_begin(std::declval<Range>())));
    };
    template <typename Range>
    class DeclvalSequence<Range, EnableIfCompileTimeRange<Range>> {
    public:
        static auto impl()
            -> decltype(Get<0, Range>::impl(std::declval<Range>()));
    };

    /**
     * Enables if the function type can accept a tuple element along with a
     * integral_constant type for the second argument
     */
    template <typename Range, typename Func>
    using EnableIfAcceptsOneArg = sharp::void_t<
        decltype(std::declval<Func>()(
                    DeclvalSequence<Range>::impl()))>;
    /**
     * Enables if the function accepts two arguments, one of the range type
     * and another of type std::integral_constant
     *
     * For simplicity I am not adding in the possibility of accepting an int
     * as the second argument
     */
    template <typename Range, typename Func>
    using EnableIfAcceptsTwoArgs = sharp::void_t<
        decltype(std::declval<Func>()(
                    DeclvalSequence<Range>::impl(),
                    std::integral_constant<int, 0>{}))>;

    /**
     * Implementation of the for_each runtime algorithm, this has two cases,
     * one for the case where the function accepts one argument and another
     * for the case when the function accepts two arguments
     */
    template <typename Range, typename Func,
              EnableIfAcceptsTwoArgs<Range, Func>* = nullptr>
    void for_each_runtime_impl(Range&& range, Func& func) {

        // iterate through the range, not using a range based for loop because
        // in the future this algorithm should pass iterators to the functor
        // as well
        auto first = adl::adl_begin(std::forward<Range>(range));
        auto last = adl::adl_end(std::forward<Range>(range));
        for (auto index = 0; first != last; ++first, ++index) {
            func(*first, index);
        }
    }
    template <typename Range, typename Func,
              EnableIfAcceptsOneArg<Range, Func>* = nullptr>
    void for_each_runtime_impl(Range&& range, Func& func) {

        // construct an adaptor that makes the function passed in a two
        // argument function and then pass that to the implementation function
        auto two_arg_adaptor = [&func](auto&& ele, auto) {
            func(std::forward<decltype(ele)>(ele));
        };
        for_each_runtime_impl(std::forward<Range>(range), two_arg_adaptor);
    }

    template <typename Range, typename Func>
    void for_each_compile_time_impl(Range&&, Func&) {}

    template <typename Range, typename = sharp::void_t<>>
    class ForEachImpl {
    public:
        template <typename R, typename Func>
        static void impl(R&& range, Func& func) {
            for_each_compile_time_impl(std::forward<R>(range), func);
        }
    };
    template <typename Range>
    class ForEachImpl<Range, EnableIfRuntimeRange<Range>> {
    public:
        template <typename R, typename Func>
        static void impl(R&& range, Func& func) {
            for_each_runtime_impl(std::forward<R>(range), func);
        }
    };


} // namespace detail

template <typename Range, typename Func>
constexpr Func for_each(Range&& tup, Func func) {
    for_each_detail::ForEachImpl<Range>::impl(std::forward<Range>(tup), func);
    return func;
}

} // namespace sharp

