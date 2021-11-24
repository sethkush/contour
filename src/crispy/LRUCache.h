/**
 * This file is part of the "libterminal" project
 *   Copyright (c) 2021 Christian Parpart <christian@parpart.family>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <cassert>
#include <list>
#include <ostream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <fmt/format.h>

#include <iostream>

namespace crispy
{

/// Implements LRU (Least recently used) cache.
template <typename Key, typename Value>
class LRUCache
{
public:
    size_t static constexpr inline npos = size_t(-1);

    struct Item
    {
        Key key = {};
        Value value = {};
        size_t prev = npos;
        size_t next = npos;

        void clear()
        {
            *this = Item{};
        }

        std::ostream& repr(std::ostream& os) const
        {
            os << '[';
            if (prev != npos) os << prev; else os << "npos";
            os << '(' << key << ", " << value << ')';
            if (next != npos) os << next; else os << "npos";
            os << ']';
            return os;
            //return os << fmt::format("[{}<-({}, {})->{}]", prev, key, value, next);
        }
    };
    using ItemList = std::vector<Item>;

    using iterator = typename ItemList::iterator;
    using const_iterator = typename ItemList::const_iterator;

    explicit LRUCache(std::size_t _capacity): capacity_{_capacity}
    {
        items_.resize(_capacity);
    }

    std::size_t size() const noexcept { return itemByKeyMapping_.size(); }
    std::size_t capacity() const noexcept { return capacity_; }

    void clear()
    {
        for (size_t i = 0; i < size(); ++i)
            items_[i].clear();
        itemByKeyMapping_.clear();
    }

    void touch(Key _key) noexcept
    {
        (void) try_get(_key);
    }

    [[nodiscard]] bool contains(Key _key) const noexcept
    {
        return try_get(_key) != nullptr;
    }

    [[nodiscard]] Value* try_get(Key _key) const
    {
        return const_cast<LRUCache*>(this)->try_get(_key);
    }

    [[nodiscard]] Value* try_get(Key _key)
    {
        if (auto i = itemByKeyMapping_.find(_key); i != itemByKeyMapping_.end())
        {
            // move it to the front by swapping, and return it
            auto const newHead = i->second;
            std::cout << fmt::format(
                "try_get: {} -> {}\n",
                _key,
                items_[i->second].value
            );
            if (newHead != head_)
            {
                // std::swap(items_[head_].key, items_[newHead].key);
                // std::swap(items_[head_].value, items_[newHead].value);
                items_[head_].prev = newHead;
                items_[head_].next = items_[newHead].next;
                items_[newHead].next = head_;
                items_[newHead].prev = npos;
                head_ = newHead;
                assert(head_ != npos);
            }
            return &items_[newHead].value;
        }
        std::cout << fmt::format("try_get: {} -> npos\n", _key);

        return nullptr;
    }

    [[nodiscard]] Value& at(Key _key)
    {
        if (Value* p = try_get(_key))
            return *p;

        throw std::out_of_range("_key");
    }

    [[nodiscard]] Value const& at(Key _key) const
    {
        if (Value const* p = try_get(_key))
            return *p;

        throw std::out_of_range("_key");
    }

    /// Returns the value for the given key, default-constructing it in case
    /// if it wasn't in the cache just yet.
    [[nodiscard]] Value& operator[](Key _key)
    {
        if (Value* p = try_get(_key))
            return *p;

        return emplace(_key, {});
    }

    /// Conditionally creates a new item to the LRU-Cache iff its key was not present yet.
    ///
    /// @retval true the key did not exist in cache yet, a new value was constructed.
    /// @retval false The key is already in the cache, no entry was constructed.
    template <typename ValueConstructFn>
    [[nodiscard]] bool try_emplace(Key _key, ValueConstructFn _constructValue)
    {
        if (Value* p = try_get(_key))
            return false;

        if (itemByKeyMapping_.size() == capacity_)
            evict_one_and_push_front(_key).value = _constructValue();
        else
            prepend_internal(_key, _constructValue());

        return true;
    }

    template <typename ValueConstructFn>
    [[nodiscard]] Value& get_or_emplace(Key _key, ValueConstructFn _constructValue)
    {
        if (Value* p = try_get(_key))
            return *p;
        Value& v = emplace(_key, _constructValue());
        repr(std::cout << "get_or_emplace: ") << '\n';
        return v;
    }

    Value& emplace(Key _key, Value _value)
    {
        assert(!contains(_key));
        std::cout << fmt::format("emplace: {} -> {}\n", _key, _value);

        if (size() == capacity_)
            return evict_one_and_push_front(_key).value = std::move(_value);

        prepend_internal(_key, std::move(_value));
        return items_[head_].value;
    }

    [[nodiscard]] iterator begin() { return items_.begin(); }
    [[nodiscard]] iterator end() { return std::next(items_.begin(), size()); }

    [[nodiscard]] const_iterator begin() const { return items_.cbegin(); }
    [[nodiscard]] const_iterator end() const { return std::next(items_.cbegin(), size()); }

    [[nodiscard]] const_iterator cbegin() const { return items_.cbegin(); }
    [[nodiscard]] const_iterator cend() const { return std::next(items_.cbegin(), size()); }

    [[nodiscard]] std::vector<Key> keys() const
    {
        std::vector<Key> result;
        result.resize(size());
        size_t i = 0;
        for (size_t pos = head_; pos != npos; pos = items_[pos].next)
            result[i++] = items_[pos].key;
        return result;
    }

    // void erase(iterator _iter)
    // {
    //     items_.erase(_iter);
    //     itemByKeyMapping_.erase(_iter->first);
    // }
    //
    // void erase(Key const& _key)
    // {
    //     if (auto i = itemByKeyMapping_.find(_key); i != itemByKeyMapping_.end())
    //         erase(i->second);
    // }

    std::ostream& repr(std::ostream& os) const
    {
        os << "LRUCache: head=";
        if (head_ != npos) os << head_; else os << "npos";
        os << ", tail=";
        if (tail_ != npos) os << tail_; else os << "npos";
        os << ":";
        for (size_t pos = head_; pos != npos; pos = items_[pos].next)
        {
            os << fmt::format(" ({}: ", pos);
            items_[pos].repr(os);
            os << ")";
        }
        return os;
    }

private:
    void prepend_internal(Key _key, Value _value)
    {
        assert(size() < capacity_);
        auto const newHead = size();
        items_[newHead].key = std::move(_key);
        items_[newHead].value = std::move(_value);
        items_[newHead].next = head_;
        if (head_ != npos)
            items_[head_].prev = newHead;
        head_ = newHead;
        if (tail_ == npos) // first item
            tail_ = head_;

        assert(head_ != npos);
        assert(tail_ != npos);

        itemByKeyMapping_.emplace(_key, head_);
    }

    /// Evicts least recently used item and prepares (/reuses) its storage for a new item.
    [[nodiscard]] Item& evict_one_and_push_front(Key _newKey)
    {
        auto i = itemByKeyMapping_.find(items_[tail_].key);
        assert(i != itemByKeyMapping_.end());
        auto const newHead = i->second;
        auto const newTail = items_[tail_].prev;

        itemByKeyMapping_.erase(i);
        itemByKeyMapping_.emplace(_newKey, newHead);

        items_[head_].prev = newHead;
        items_[newHead].prev = npos;
        items_[newHead].key = _newKey;
        items_[newHead].next = head_;
        items_[newTail].next = npos;

        head_ = newHead;
        tail_ = newTail;

        assert(head_ != npos);
        assert(tail_ != npos);

        return items_[head_];
    }

    // private data
    //
    ItemList items_;
    size_t head_ = npos;
    size_t tail_ = npos;
    std::unordered_map<Key, size_t> itemByKeyMapping_;
    std::size_t capacity_;
};

}
