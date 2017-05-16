/**
 * @file FutureImpl.hpp
 * @author Aaryaman Sagar
 *
 * This file contains the main code for the FutureImpl class.  Which is the
 * implementation of the futures code, this contains all the synchronization
 * for when a value is set and stuff like that
 *
 * The Promise and Future classes are simply wrappers around this.  Both hook
 * into methods in this when one wants to set state and when the other waits
 * for the promise to be fulfilled.
 *
 * The Promise class is responsible for creating this shared state but the
 * Future class which in most cases extends beyond the Promise class is
 * responsible for destroying it
 */

#pragma once

#include <exception>
#include <condition_variable>
#include <mutex>
#include <initializer_list>
#include <system_error>
#include <functional>

#include <sharp/Traits/Traits.hpp>

namespace sharp {

namespace detail {

    /**
     * A future impl represents the shared state that a future/promise pair
     * share among them.  This contains all the main code for the futures
     * library
     */
    template <typename Type>
    class FutureImpl {
    public:

        /**
         * The wait function blocks until there is a value or an exception in
         * the shared state, this uses a condition variable to wait internally
         */
        void wait() const;

        /**
         * wait_and_get() first calls wait() to wait for shared state to be
         * ready and then returns the object in the shared state as if by
         * std::move(instance)
         */
        Type get();

        /**
         * Forwarding reference implementations of the set_value function
         */
        template <typename... Args>
        void set_value(Args&&...);
        template <typename U, typename... Args>
        void set_value(std::initializer_list<U> il, Args&&...);

        /**
         * Set an exception in the shared state that will be rethrown on a get
         */
        void set_exception(std::exception_ptr ptr);

        /**
         * Returns true if the future contains either a value or an exception
         */
        bool is_ready() const noexcept;

        /**
         * Sets the atomic flag to be set to indicate that a future has
         * already been retrieved from the promise, if the flag had been set
         * before this a future exception with the error code
         * future_already_retrieved is thrown
         */
        void test_and_set_retrieved_flag();

        /**
         * Add a continuation to be executed on the shared state when the
         * shared state has been fulfilled
         *
         * The continuation closure will be executed inline and will either be
         * executed immediately if there is a value present in the shared
         * state or will be packed up and stored in a std::function<> object
         * to be executed later when the need arises, this way this function
         * presents reusable code without causing unnecesary allocation if not
         * needed
         *
         * The continuation functor must accept a FutureImpl<Type> object by
         * reference
         */
        template <typename Func>
        void add_callback(Func&& func);

        /**
         * Returns the current exception_ptr or value assuming there is an
         * exception or value in this
         */
        std::exception_ptr& get_exception_ptr();
        const std::exception_ptr& get_exception_ptr() const;
        Type& get_value();

        /**
         * Returns true if the shared state contains an exception
         *
         * In order to be thread safe, this function first acquires a lock and
         * then makes the check
         */
        bool contains_exception() const;

    private:

        /**
         * The states the future can have, these should be self explanatory
         */
        enum class FutureState : int {
            NotFulfilled,
            ContainsValue,
            ContainsException,
        };

        /**
         * fixes the internal bookkeeping for the future, including setting
         * the state variable to the appropriate value and signalling any
         * waiting threads to wake up
         */
        void after_set_value();
        void after_set_exception();

        /**
         * Checking functions that make sure everything is okay in the future
         * and if something is wrong they throw the appropriate exception
         */
        void check_get() const;
        void check_set_value() const;

        /**
         * Executes the callback on this if there is a callback to execute
         */
        void execute_callback(std::unique_lock<std::mutex>& lck);

        /**
         * Synchronizy locking things
         */
        std::atomic_flag retrieved = ATOMIC_FLAG_INIT;
        mutable std::atomic<FutureState> state{FutureState::NotFulfilled};
        mutable std::mutex mtx;
        mutable std::condition_variable cv;

        /**
         * A union containing either an exception_ptr or a value, this should
         * be replaced with a better std::variant once that has been
         * standardized and most compilers support that
         */
        std::aligned_union_t<0, std::exception_ptr, Type> storage;

        /**
         * A callback functor to be called when the shared state has a value
         */
        std::function<void(FutureImpl<Type>&)> callback;
    };

} // namespace detail

} // namespace sharp

#include <sharp/Future/detail/FutureImpl.ipp>
