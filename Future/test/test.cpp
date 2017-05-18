#include <thread>
#include <chrono>
#include <utility>
#include <iostream>

#include <sharp/Future/Future.hpp>
#include <sharp/Threads/Threads.hpp>
#include <gtest/gtest.h>

TEST(Future, FutureBasic) {
    auto promise = sharp::Promise<int>{};
    auto future = promise.get_future();
    promise.set_value(1);
    auto value = future.get();
    EXPECT_EQ(value, 1);
}

TEST(Future, FutureBasicThreaded) {
    auto promise = sharp::Promise<int>{};
    auto future = promise.get_future();
    auto th = std::thread{[&]() {
        promise.set_value(10);
    }};
    EXPECT_EQ(future.get(), 10);
    th.join();
}

TEST(Future, FutureMove) {
    auto promise = sharp::Promise<int>{};
    auto future = promise.get_future();
    EXPECT_TRUE(future.valid());
    auto another_future = std::move(future);
    EXPECT_TRUE(another_future.valid());
    EXPECT_FALSE(future.valid());

    future = std::move(another_future);
    EXPECT_TRUE(future.valid());
    EXPECT_FALSE(another_future.valid());
}

TEST(Future, FutureExceptionSend) {
    auto promise = sharp::Promise<int>{};
    auto future = promise.get_future();
    promise.set_exception(std::make_exception_ptr(std::logic_error{""}));
    try {
        future.get();
        EXPECT_TRUE(false);
    } catch (std::logic_error& err) {
    } catch (...) {
        EXPECT_TRUE(false);
    }
}

TEST(Future, FutureAlreadyRetrieved) {
    auto promise = sharp::Promise<int>{};
    auto future = promise.get_future();
    try {
        auto future = promise.get_future();
        EXPECT_TRUE(false);
    } catch (sharp::FutureError& err) {
        EXPECT_EQ(err.code().value(), static_cast<int>(
                    sharp::FutureErrorCode::future_already_retrieved));
    }
}

TEST(Future, PromiseAlreadySatisfied) {
    auto promise = sharp::Promise<int>{};
    promise.set_value(1);
    try {
        promise.set_value(1);
        EXPECT_TRUE(false);
    } catch(sharp::FutureError& err) {
        EXPECT_EQ(err.code().value(), static_cast<int>(
                    sharp::FutureErrorCode::promise_already_satisfied));
    }
}

TEST(Future, NoState) {
    auto future = sharp::Future<int>{};

    try {
        future.get();
        EXPECT_TRUE(false);
    } catch(sharp::FutureError& err) {
        EXPECT_EQ(err.code().value(), static_cast<int>(
                    sharp::FutureErrorCode::no_state));
    }

    try {
        future.wait();
        EXPECT_TRUE(false);
    } catch(sharp::FutureError& err) {
        EXPECT_EQ(err.code().value(), static_cast<int>(
                    sharp::FutureErrorCode::no_state));
    }

}

TEST(Future, DoubleGet) {
    auto promise = sharp::Promise<int>{};
    auto future = promise.get_future();
    promise.set_value(1);
    future.get();
    try {
        future.get();
        EXPECT_TRUE(false);
    } catch(sharp::FutureError& err) {
        EXPECT_EQ(err.code().value(), static_cast<int>(
                    sharp::FutureErrorCode::no_state));
    }
}

TEST(Future, BrokenPromise) {
    auto future = sharp::Future<int>{};
    {
        auto promise = sharp::Promise<int>{};
        auto future_two = promise.get_future();
        future = std::move(future_two);
    }
    try {
        future.get();
        EXPECT_TRUE(false);
    } catch (sharp::FutureError& err) {
        EXPECT_EQ(err.code().value(), static_cast<int>(
                    sharp::FutureErrorCode::broken_promise));
    }
}

TEST(Future, UnwrapConstructBasic) {
    for (auto i = 0; i < 100; ++i) {
        sharp::ThreadTest::reset();
        auto promise = sharp::Promise<sharp::Future<int>>{};
        auto future_unwrapped = sharp::Future<int>{promise.get_future()};

        std::thread{[promise = std::move(promise)]() mutable {
            sharp::ThreadTest::mark(1);
            auto promise_inner = sharp::Promise<int>{};
            auto future_inner = promise_inner.get_future();
            promise.set_value(std::move(future_inner));
            promise_inner.set_value(1);
        }}.detach();

        sharp::ThreadTest::mark(0);
        EXPECT_EQ(future_unwrapped.get(), 1);
    }
}

TEST(Future, UnwrapConstructOtherInvalid) {
    try {
        auto future = sharp::Future<sharp::Future<int>>{};
        auto future_unwrapped = sharp::Future<int>{std::move(future)};
        EXPECT_TRUE(false);
    } catch (sharp::FutureError& err) {
        EXPECT_EQ(err.code().value(), static_cast<int>(
                    sharp::FutureErrorCode::no_state));
    }
}

TEST(Future, UnwrapConstructOtherContainsException) {
    try {
        auto promise = sharp::Promise<sharp::Future<int>>{};
        auto future = promise.get_future();
        auto future_unwrapped = sharp::Future<int>{std::move(future)};
        promise.set_exception(std::make_exception_ptr(std::logic_error{""}));
        future_unwrapped.get();
        EXPECT_TRUE(false);
    } catch (std::logic_error& err) {}
}

TEST(Future, UnwrapConstructOtherContainsInvalid) {
    try {
        auto promise = sharp::Promise<sharp::Future<int>>{};
        auto future = promise.get_future();
        auto future_unwrapped = sharp::Future<int>{std::move(future)};
        promise.set_value(sharp::Future<int>{});
        future_unwrapped.get();
        EXPECT_TRUE(false);
    } catch (sharp::FutureError& err) {
        EXPECT_EQ(err.code().value(), static_cast<int>(
                    sharp::FutureErrorCode::broken_promise));
    }
}

TEST(Future, UnwrapConstructOtherContainsValidWithException) {
    auto promise = sharp::Promise<sharp::Future<int>>{};
    auto future = promise.get_future();
    auto promise_inner = sharp::Promise<int>{};
    auto future_inner = promise_inner.get_future();
    auto future_unwrapped = sharp::Future<int>{std::move(future)};

    try {
        promise_inner.set_exception(
                std::make_exception_ptr(std::logic_error{""}));
        promise.set_value(std::move(future_inner));
        future_unwrapped.get();
        EXPECT_TRUE(false);
    } catch (std::logic_error& err) {}
}

TEST(Future, FutureThenBasicTest) {
    auto promise = sharp::Promise<int>{};
    auto future = promise.get_future();
    auto thened_future = future.then([](auto future) {
        return future.get() * 5;
    });
    promise.set_value(10);
    EXPECT_EQ(thened_future.get(), 50);
}

TEST(Future, ThreadedThenTest) {
    for (auto i = 0; i < 100; ++i) {
        auto promise = sharp::Promise<int>{};
        auto future = promise.get_future();
        std::thread{[&]() {
            promise.set_value(10);
        }}.detach();
        auto thened_future = future.then([](auto future) {
            return future.get() * 5;
        });
        EXPECT_EQ(thened_future.get(), 50);
    }
}
