/**
 * This file is part of the "contour" project.
 *   Copyright (c) 2020 Christian Parpart <christian@parpart.family>
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
#include <crispy/LRUCache.h>

#include <functional>
#include <iostream>

#include <catch2/catch_all.hpp>
#include <fmt/format.h>

using namespace std;
using namespace std::string_view_literals;

// template <typename A, typename B>
// void dump_it(crispy::LRUCache<A, B> const& cache, std::string_view _header)
// {
//     std::ostream& out = std::cout;
//
//     out << fmt::format("LRUCache({}/{}): {}\n", cache.size(), cache.capacity(), _header);
//     for (typename crispy::LRUCache<A, B>::Item const& item: cache)
//     {
//         out << fmt::format("{}: {}\n", item.first, item.second);
//     }
//     out << "\n";
// }

template <typename T>
static std::string join(std::vector<T> const& _list, std::string_view _delimiter = " "sv)
{
    std::string s;
    for (size_t i = 0; i < _list.size(); ++i)
    {
        if (i)
            s += _delimiter;
        s += fmt::format("{}", _list[i]);
    }
    return s;
}

TEST_CASE("LRUCache.ctor", "[lrucache]")
{
    auto cache = crispy::LRUCache<int, int>(4);
    CHECK(cache.size() == 0);
    CHECK(cache.capacity() == 4);
}

TEST_CASE("LRUCache.at", "[lrucache]")
{
    printf("a\n");
    auto cache = crispy::LRUCache<int, int>(2);
    printf("b\n");

    try {
        printf("c\n");
        cache.at(2);
        printf("d\n");
        CHECK(false);
    }
    catch (std::out_of_range)
    {
        printf("e ok\n");
        CHECK(true);
    }
    printf("f\n");
    //CHECK_THROWS_AS(cache.at(2), std::out_of_range);
    cache[2] = 4;
    CHECK_NOTHROW(cache.at(2));
}

TEST_CASE("LRUCache.get_or_emplace", "[lrucache]")
{
    auto cache = crispy::LRUCache<int, int>(2);

    // add first pair
    int& a = cache.get_or_emplace(2, []() { return 4; });
    CHECK(a == 4);
    CHECK(cache.contains(2));
    CHECK(cache.at(2) == 4);
    CHECK(cache.size() == 1);
    CHECK(join(cache.keys()) == "2"sv);

    // int& a2 = cache.get_or_emplace(2, []() { return -4; });
    // std::cout << fmt::format("keys: {}\n", join(cache.keys()));
    // CHECK(a2 == 4);
    // CHECK(cache.contains(2));
    // CHECK(cache.at(2) == 4);
    // CHECK(cache.size() == 1);
    // CHECK(join(cache.keys()) == "2"sv);

    // add second pair
    int& b = cache.get_or_emplace(3, []() { return 6; });
    std::cout << fmt::format("keys: {}\n", join(cache.keys()));
    CHECK(b == 6);
    CHECK(cache.contains(3));
    CHECK(cache.at(3) == 6);
    CHECK(cache.size() == 2);
    CHECK(join(cache.keys()) == "3 2"sv);

    // add third pair, evicting first.
    int& c = cache.get_or_emplace(4, []() { return 8; });
    std::cout << fmt::format("keys: {}\n", join(cache.keys()));
    CHECK(join(cache.keys()) == "4 3"sv);
    CHECK(c == 8);
    cout << "repr1: "; cache.repr(std::cout); cout << endl;
    CHECK(cache.at(4) == 8);
    CHECK(cache.size() == 2);
    cout << "repr2: "; cache.repr(std::cout); cout << endl;
    CHECK(cache.contains(3));
    cout << "repr3: "; cache.repr(std::cout); cout << endl;
    CHECK_FALSE(cache.contains(2)); // thrown out

    cout << "repr4: "; cache.repr(std::cout); cout << endl;

    // int& b2 = cache.get_or_emplace(3, []() { return -3; });
    //
    // CHECK(b2 == 6);
    // CHECK(join(cache.keys()) == "3 4"sv);
    // CHECK(cache.at(3) == 6);
    // CHECK(cache.size() == 2);
}

TEST_CASE("LRUCache.operator[]", "[lrucache]")
{
    auto cache = crispy::LRUCache<int, int>(2);

    (void) cache[2];
    CHECK(join(cache.keys()) == "2"sv);
    CHECK(cache[2] == 0);
    cache[2] = 4;
    CHECK(cache[2] == 4);
    CHECK(cache.size() == 1);

    cache[3] = 6;
    CHECK(join(cache.keys()) == "3 2"sv);
    CHECK(cache[3] == 6);
    CHECK(cache.size() == 2);

    cache[4] = 8;
    CHECK(join(cache.keys()) == "4 3"sv);
    CHECK(cache[4] == 8);
    CHECK(cache.size() == 2);
    cout << "repr4: "; cache.repr(std::cout); cout << endl;
    (void) cache.contains(3);
    CHECK(cache.contains(3));
    cout << "repr3: "; cache.repr(std::cout); cout << endl;
    CHECK_FALSE(cache.contains(2)); // thrown out

    cout << "repr2: "; cache.repr(std::cout); cout << endl;
    (void) cache[3]; // move 3 to the front (currently at the back)
    CHECK(join(cache.keys()) == "3 4"sv);
    cout << "repr1: "; cache.repr(std::cout); cout << endl;
    cache[5] = 10;
    CHECK(join(cache.keys()) == "5 3"sv);
    CHECK(cache.at(5) == 10);
    CHECK(cache.size() == 2);
    CHECK(cache.contains(5));
    CHECK(cache.contains(3));
    CHECK_FALSE(cache.contains(4)); // thrown out
}

TEST_CASE("LRUCache.clear", "[lrucache]")
{
    auto cache = crispy::LRUCache<int, int>(4);
    cache[2] = 4;
    cache[3] = 6;
    CHECK(cache.size() == 2);
    cache.clear();
    CHECK(cache.size() == 0);
}

TEST_CASE("LRUCache.try_emplace", "[lrucache]")
{
    auto cache = crispy::LRUCache<int, int>(2);
    auto rv = cache.try_emplace(2, []() { return 4; });
    CHECK(rv);
    CHECK(join(cache.keys()) == "2");
    CHECK(cache.at(2) == 4);

    rv = cache.try_emplace(3, []() { return 6; });
    CHECK(rv);
    CHECK(join(cache.keys()) == "3 2");
    CHECK(cache.at(2) == 4);
    CHECK(cache.at(3) == 6);

    rv = cache.try_emplace(2, []() { return -1; });
    CHECK_FALSE(rv);
    CHECK(join(cache.keys()) == "2 3");
    CHECK(cache.at(2) == 4);
    CHECK(cache.at(3) == 6);
}

