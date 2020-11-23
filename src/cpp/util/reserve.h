#pragma once

#include <robin_hood.h>

#include <map>
#include <unordered_map>

namespace util {

template <typename K, typename V>
void reserve(std::unordered_map<K, V>& map, size_t s) {
    map.reserve(s);
}

template <typename K, typename V>
void reserve(robin_hood::unordered_flat_map<K, V>& map, size_t s) {
    map.reserve(s);
}

template <typename K, typename V>
void reserve(robin_hood::unordered_node_map<K, V>& map, size_t s) {
    map.reserve(s);
}

template <typename K, typename V>
void reserve(std::map<K, V>& /*map*/, size_t /*s*/) {
    // does nothing
}

} // namespace util
