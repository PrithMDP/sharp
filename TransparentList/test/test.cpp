#include <memory>
#include <vector>

#include <gtest/gtest.h>
#include <sharp/TransparentList/TransparentList.hpp>

using std::unique_ptr;
using std::vector;
using std::make_unique;
using sharp::TransparentList;
using sharp::Node;

TEST(TransparentList, construct_test) {
    sharp::TransparentList<int>{};
}

TEST(TransparentList, simple_push_back_test) {
    auto list = sharp::TransparentList<int>{};
    auto new_node = make_unique<Node<int>>(sharp::emplace_construct::tag, 1);
    list.push_back(new_node.get());
    EXPECT_EQ(new_node.get(), &(*list.begin()));
}

TEST(TransparentList, simple_push_front_test) {
    auto list = sharp::TransparentList<int>{};
    auto node = make_unique<Node<int>>(sharp::emplace_construct::tag, 1);
    list.push_front(node.get());
    EXPECT_EQ(node.get(), &(*list.begin()));
    EXPECT_EQ(1, (*list.begin()).datum);
}

TEST(TransparentList, double_push_front_test) {

    auto list = sharp::TransparentList<int>{};

    auto node_one = make_unique<Node<int>>(sharp::emplace_construct::tag, 1);
    list.push_front(node_one.get());

    auto node_two = make_unique<Node<int>>(sharp::emplace_construct::tag, 2);
    list.push_front(node_two.get());

    auto node_three = make_unique<Node<int>>(sharp::emplace_construct::tag, 3);
    list.push_front(node_three.get());

    EXPECT_EQ(node_three.get(), &(*list.begin()));
    EXPECT_EQ(3, (*list.begin()).datum);
    EXPECT_EQ(node_two.get(), &(*(++list.begin())));
    EXPECT_EQ(2, (*(++list.begin())).datum);
    EXPECT_EQ(node_one.get(), &(*(++++list.begin())));
    EXPECT_EQ(1, (*(++++list.begin())).datum);
}

TEST(TransparentList, double_push_back_test) {

    auto list = sharp::TransparentList<int>{};

    auto node_one = make_unique<Node<int>>(sharp::emplace_construct::tag, 1);
    list.push_back(node_one.get());

    auto node_two = make_unique<Node<int>>(sharp::emplace_construct::tag, 2);
    list.push_back(node_two.get());

    auto node_three = make_unique<Node<int>>(sharp::emplace_construct::tag, 3);
    list.push_back(node_three.get());

    EXPECT_EQ(node_one.get(), &(*list.begin()));
    EXPECT_EQ(1, (*list.begin()).datum);
    EXPECT_EQ(node_two.get(), &(*(++list.begin())));
    EXPECT_EQ(2, (*(++list.begin())).datum);
    EXPECT_EQ(node_three.get(), &(*(++++list.begin())));
    EXPECT_EQ(3, (*(++++list.begin())).datum);
}

TEST(TransparentList, range_test) {

    auto list = sharp::TransparentList<int>{};
    auto vec = vector<unique_ptr<Node<int>>>{};
    vec.push_back(make_unique<Node<int>>(sharp::emplace_construct::tag, 1));
    vec.push_back(make_unique<Node<int>>(sharp::emplace_construct::tag, 2));
    vec.push_back(make_unique<Node<int>>(sharp::emplace_construct::tag, 3));
    vec.push_back(make_unique<Node<int>>(sharp::emplace_construct::tag, 4));

    // insert into the list in order
    for (const auto& node : vec) {
        list.push_back(node.get());
    }

    // assert that the ranges are equal
    EXPECT_TRUE(std::equal(vec.begin(), vec.end(), list.begin(), list.end(),
                [](const auto& node_ptr_lhs, const auto& node_ptr_rhs) {
        return node_ptr_lhs->datum == node_ptr_rhs.datum;
    }));
}

TEST(TransparentList, range_test_and_push_back_front_test) {

    auto list = sharp::TransparentList<int>{};
    auto vec = vector<unique_ptr<Node<int>>>{};
    vec.push_back(make_unique<Node<int>>(sharp::emplace_construct::tag, 1));
    vec.push_back(make_unique<Node<int>>(sharp::emplace_construct::tag, 2));
    vec.push_back(make_unique<Node<int>>(sharp::emplace_construct::tag, 3));
    vec.push_back(make_unique<Node<int>>(sharp::emplace_construct::tag, 4));

    // insert into the list in order
    for (const auto& node : vec) {
        list.push_back(node.get());
    }

    // assert that the ranges are equal
    EXPECT_TRUE(std::equal(vec.begin(), vec.end(), list.begin(), list.end(),
                [](const auto& node_ptr_lhs, const auto& node_ptr_rhs) {
        return node_ptr_lhs->datum == node_ptr_rhs.datum;
    }));

    vec.push_back(make_unique<Node<int>>(sharp::emplace_construct::tag, 5));
    list.push_back(vec.back().get());

    // assert that the ranges are equal
    EXPECT_TRUE(std::equal(vec.begin(), vec.end(), list.begin(), list.end(),
                [](const auto& node_ptr_lhs, const auto& node_ptr_rhs) {
        return node_ptr_lhs->datum == node_ptr_rhs.datum;
    }));
}
