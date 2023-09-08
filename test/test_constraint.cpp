#include "dbm/constraints.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <set>
#include <vector>

TEST_CASE("Constraint operators")
{
    const auto vec = std::vector<constraint_t>{{2, 1, 2}, {1, 2, -2}, {1, 0, 1}, {0, 1, -2}};
    CHECK(vec[0] != vec[1]);
    CHECK(!std::is_sorted(vec.begin(), vec.end()));
    auto vec_sort = vec;
    std::sort(vec_sort.begin(), vec_sort.end());
    REQUIRE(std::is_sorted(vec_sort.begin(), vec_sort.end()));
    auto vec_sort2 = vec_sort;
    CHECK(std::includes(vec_sort2.begin(), vec_sort2.end(), vec_sort.begin(), vec_sort.end()));
    CHECK(std::includes(vec_sort.begin(), vec_sort.end(), vec_sort2.begin(), vec_sort2.end()));
    vec_sort2.push_back({2, 0, 3});
    vec_sort2.push_back({0, 2, -3});
    std::sort(vec_sort2.begin(), vec_sort2.end());
    REQUIRE(std::is_sorted(vec_sort2.begin(), vec_sort2.end()));
    CHECK(std::includes(vec_sort2.begin(), vec_sort2.end(), vec_sort.begin(), vec_sort.end()));
    CHECK(!std::includes(vec_sort.begin(), vec_sort.end(), vec_sort2.begin(), vec_sort2.end()));

    const auto set = std::set(vec.begin(), vec.end());
    const auto set1 = std::set(vec_sort.begin(), vec_sort.end());
    const auto set2 = std::set(vec_sort2.begin(), vec_sort2.end());
    CHECK(std::includes(set.begin(), set.end(), set1.begin(), set1.end()));
    CHECK(std::includes(set1.begin(), set1.end(), set.begin(), set.end()));
    CHECK(!std::includes(set.begin(), set.end(), set2.begin(), set2.end()));
    CHECK(std::includes(set2.begin(), set2.end(), set.begin(), set.end()));
    CHECK(!std::includes(set1.begin(), set1.end(), set2.begin(), set2.end()));
    CHECK(std::includes(set2.begin(), set2.end(), set1.begin(), set1.end()));
}