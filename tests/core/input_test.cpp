// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include "foresthub/core/input.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

using namespace foresthub::core;

// ==========================================
// 1. Tests for InputString
// ==========================================

TEST(InputStringTest, ConstructorAndMemberAccess) {
    std::string raw_text = "Hello World";
    auto input_str = std::make_shared<InputString>(raw_text);

    EXPECT_EQ(input_str->text, raw_text);
}

TEST(InputStringTest, ToStringInterface) {
    std::string raw_text = "Test String";
    auto input_str = std::make_shared<InputString>(raw_text);

    EXPECT_EQ(input_str->ToString(), raw_text);
}

TEST(InputStringTest, InheritanceCheck) {
    auto input_str = std::make_shared<InputString>("Inheritance Test");

    std::shared_ptr<Input> as_input = input_str;
    ASSERT_NE(as_input, nullptr) << "InputString should be castable to Input";

    std::shared_ptr<InputItem> as_item = input_str;
    ASSERT_NE(as_item, nullptr) << "InputString should be castable to InputItem";
    EXPECT_EQ(as_item->ToString(), "Inheritance Test");
}

// ==========================================
// 2. Tests for InputItems
// ==========================================

TEST(InputItemsTest, DefaultConstructor) {
    auto list = std::make_shared<InputItems>();

    EXPECT_TRUE(list->items.empty());
    EXPECT_EQ(list->items.size(), 0);
}

TEST(InputItemsTest, AddMethod) {
    auto list = std::make_shared<InputItems>();

    auto item1 = std::make_shared<InputString>("First");
    auto item2 = std::make_shared<InputString>("Second");

    list->PushBack(item1);
    ASSERT_EQ(list->items.size(), 1);
    EXPECT_EQ(list->items[0]->ToString(), "First");

    list->PushBack(item2);
    ASSERT_EQ(list->items.size(), 2);
    EXPECT_EQ(list->items[1]->ToString(), "Second");
}

TEST(InputItemsTest, SingleItemConstructor) {
    auto item = std::make_shared<InputString>("Single Item Constructor");
    auto list = std::make_shared<InputItems>(item);

    ASSERT_EQ(list->items.size(), 1);
    EXPECT_EQ(list->items[0]->ToString(), "Single Item Constructor");
}

TEST(InputItemsTest, IsInputButNotInputItem) {
    auto list = std::make_shared<InputItems>();

    std::shared_ptr<Input> as_input = list;
    ASSERT_NE(as_input, nullptr);

    EXPECT_EQ(as_input->GetInputType(), InputType::kItems);
}

// ==========================================
// 3. Tests for AsInputItems (Helper Function)
// ==========================================

TEST(AsInputItemsTest, HandleNullptr) {
    std::shared_ptr<Input> null_input = nullptr;

    std::shared_ptr<InputItems> result = AsInputItems(null_input);

    ASSERT_NE(result, nullptr) << "Should never return nullptr";
    EXPECT_TRUE(result->items.empty()) << "Null input should produce an empty list";
}

TEST(AsInputItemsTest, HandleInputString) {
    std::shared_ptr<Input> str_input = std::make_shared<InputString>("Wrap me");

    std::shared_ptr<InputItems> result = AsInputItems(str_input);

    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->items.size(), 1) << "String should be wrapped into a list of length 1";
    EXPECT_EQ(result->items[0]->ToString(), "Wrap me");
}

TEST(AsInputItemsTest, HandleExistingInputItems) {
    auto original_list = std::make_shared<InputItems>();
    original_list->PushBack(std::make_shared<InputString>("Already in list"));

    std::shared_ptr<Input> input = original_list;

    std::shared_ptr<InputItems> result = AsInputItems(input);

    ASSERT_NE(result, nullptr);
    // Pointer comparison: It must return exactly the same object, not a copy
    EXPECT_EQ(result, original_list) << "Should return the original object, not a copy";
    ASSERT_EQ(result->items.size(), 1);
    EXPECT_EQ(result->items[0]->ToString(), "Already in list");
}

TEST(InputStringTest, GetItemType) {
    auto input_str = std::make_shared<InputString>("test");
    std::shared_ptr<InputItem> as_item = input_str;
    EXPECT_EQ(as_item->GetItemType(), InputItemType::kString);
}

// Note: HandleUnknownInputType test removed — InputType is a closed enum (String, Items).
// With enum-based dispatch, unknown types cannot exist at runtime.

// ==========================================
// 4. Tests for InputItems ToJson & Iterators
// ==========================================

TEST(InputItemsTest, ToJsonDirect) {
    auto list = std::make_shared<InputItems>();
    list->PushBack(std::make_shared<InputString>("First"));
    list->PushBack(std::make_shared<InputString>("Second"));

    json j;
    list->ToJson(j);

    ASSERT_TRUE(j.is_array());
    ASSERT_EQ(j.size(), 2);
    EXPECT_EQ(j[0]["value"], "First");
    EXPECT_EQ(j[1]["value"], "Second");
}

TEST(InputItemsTest, ToJsonWithNullItem) {
    auto list = std::make_shared<InputItems>();
    list->PushBack(std::make_shared<InputString>("Valid"));
    list->PushBack(nullptr);

    json j;
    list->ToJson(j);

    ASSERT_TRUE(j.is_array());
    ASSERT_EQ(j.size(), 2);
    EXPECT_EQ(j[0]["value"], "Valid");
    EXPECT_TRUE(j[1].is_null());
}

TEST(InputItemsTest, BeginEndIterators) {
    auto list = std::make_shared<InputItems>();
    list->PushBack(std::make_shared<InputString>("A"));
    list->PushBack(std::make_shared<InputString>("B"));
    list->PushBack(std::make_shared<InputString>("C"));

    int count = 0;
    for (auto it = list->Begin(); it != list->End(); ++it) {
        ++count;
    }
    EXPECT_EQ(count, 3);
}

TEST(InputItemsTest, ConstBeginEndIterators) {
    auto list = std::make_shared<InputItems>();
    list->PushBack(std::make_shared<InputString>("X"));
    list->PushBack(std::make_shared<InputString>("Y"));

    const InputItems& const_ref = *list;
    int count = 0;
    for (auto it = const_ref.Begin(); it != const_ref.End(); ++it) {
        ++count;
    }
    EXPECT_EQ(count, 2);
}

TEST(InputItemsTest, Insert) {
    auto list = std::make_shared<InputItems>();
    list->PushBack(std::make_shared<InputString>("A"));
    list->PushBack(std::make_shared<InputString>("C"));

    auto other = std::make_shared<InputItems>();
    other->PushBack(std::make_shared<InputString>("B"));

    // Insert other's items before index 1 (between A and C)
    list->Insert(list->Begin() + 1, other->Begin(), other->End());

    ASSERT_EQ(list->items.size(), 3);
    EXPECT_EQ(list->items[0]->ToString(), "A");
    EXPECT_EQ(list->items[1]->ToString(), "B");
    EXPECT_EQ(list->items[2]->ToString(), "C");
}
