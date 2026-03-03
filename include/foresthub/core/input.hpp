#ifndef FORESTHUB_CORE_INPUT_HPP
#define FORESTHUB_CORE_INPUT_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "foresthub/util/json.hpp"

namespace foresthub {
namespace core {

using json = nlohmann::json;

/// Type discriminator for Input subclasses.
enum class InputType : uint8_t {
    kString,  ///< Single text string.
    kItems    ///< Ordered list of conversation items.
};

/// Abstract base for all input types.
class Input {
public:
    virtual ~Input() = default;

    /// Returns the type discriminator for this input.
    virtual InputType GetInputType() const = 0;

    /// Serialize this input to JSON.
    /// @param j Output JSON object, populated by the method.
    virtual void ToJson(json& j) const = 0;
};

/// Type discriminator for InputItem subclasses.
enum class InputItemType : uint8_t {
    kString,     ///< Plain text (InputString).
    kToolCall,   ///< Tool call request from the model (ToolCallRequest).
    kToolResult  ///< Tool execution result (ToolResult).
};

/// Abstract base for items inside an InputItems list.
class InputItem {
public:
    virtual ~InputItem() = default;

    /// Returns the type discriminator for safe downcasting via static_pointer_cast.
    virtual InputItemType GetItemType() const = 0;

    /// Human-readable text representation of this item.
    virtual std::string ToString() const = 0;

    /// Serialize this item to JSON.
    /// @param j Output JSON object, populated by the method.
    virtual void ToJson(json& j) const = 0;
};

/// A string that acts as both top-level Input and InputItem.
class InputString : public Input, public InputItem {
public:
    std::string text;  ///< The raw text content.

    /// Construct from a text string.
    explicit InputString(std::string text) : text(std::move(text)) {}

    /// Returns the stored text directly.
    std::string ToString() const override { return text; }
    /// Identifies this item as a plain text string.
    InputItemType GetItemType() const override { return InputItemType::kString; }
    /// Identifies this input as a string.
    InputType GetInputType() const override { return InputType::kString; }
    /// Serialize as JSON object with "value" key.
    void ToJson(json& j) const override { j = json{{"value", text}}; }
};

/// An ordered list of InputItem elements.
class InputItems : public Input {
public:
    std::vector<std::shared_ptr<InputItem>> items;  ///< Ordered conversation items.

    /// Construct an empty item list.
    InputItems() = default;

    /// Construct from a single item.
    explicit InputItems(const std::shared_ptr<InputItem>& item) { items.push_back(item); }

    /// Append an item to the list.
    void PushBack(const std::shared_ptr<InputItem>& item) { items.push_back(item); }

    /// Identifies this input as an item list.
    InputType GetInputType() const override { return InputType::kItems; }

    /// Serialize all items as a JSON array.
    void ToJson(json& j) const override {
        j = json::array();
        for (const auto& item : items) {
            json item_j;
            if (item) {
                item->ToJson(item_j);
            } else {
                item_j = nullptr;
            }
            j.push_back(std::move(item_j));
        }
    }

    /// Mutable iterator over items.
    using Iterator = std::vector<std::shared_ptr<InputItem>>::iterator;
    /// Const iterator over items.
    using ConstIterator = std::vector<std::shared_ptr<InputItem>>::const_iterator;

    /// Returns a mutable iterator to the first item.
    Iterator Begin() { return items.begin(); }
    /// Returns a mutable past-the-end iterator.
    Iterator End() { return items.end(); }
    /// Returns a const iterator to the first item.
    ConstIterator Begin() const { return items.begin(); }
    /// Returns a const past-the-end iterator.
    ConstIterator End() const { return items.end(); }

    /// Forwards all arguments to the underlying vector's insert().
    ///
    /// @param args Arguments forwarded to std::vector::insert (position iterator, then value(s) or range).
    /// @code
    /// items->Insert(items->End(), other.Begin(), other.End());
    /// @endcode
    template <typename... Args>
    void Insert(Args&&... args) {
        items.insert(std::forward<Args>(args)...);
    }
};

/// Normalize input to InputItems.
/// @param input Input to normalize (string or items); nullptr returns an empty InputItems.
/// @return InputItems containing the input content.
inline std::shared_ptr<InputItems> AsInputItems(const std::shared_ptr<Input>& input) {
    if (!input) {
        return std::make_shared<InputItems>();
    }

    switch (input->GetInputType()) {
        case InputType::kItems:
            return std::static_pointer_cast<InputItems>(input);
        case InputType::kString: {
            auto new_list = std::make_shared<InputItems>();
            new_list->PushBack(std::static_pointer_cast<InputString>(input));
            return new_list;
        }
    }

    return std::make_shared<InputItems>();
}

}  // namespace core
}  // namespace foresthub

#endif  // FORESTHUB_CORE_INPUT_HPP
