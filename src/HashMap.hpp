//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#ifndef HASHMAP_HPP
#define HASHMAP_HPP

#include <utility>
#include <functional>

template <typename K, typename V>
class HashMap;

template <typename K, typename V>
void swap(HashMap<K, V>& a, HashMap<K, V>& b);

/// A custom hash map implementation. The types K and V must both be
/// default-initializable and trivially copyable. Additionally, K
/// should be hashable and ordered. Note that the "default" value
/// should not be stored in the data structure, as it is treated
/// specially for efficiency reasons.
template <typename K, typename V>
class HashMap {
private:
    std::pair<K, V>* arr;
    std::size_t bucketCount;
    std::hash<K> hash;
    std::pair<K, V> defaulted;
    std::size_t elems;
    void rehash();
public:

    /// Default initializes a hash map containing no elements.
    HashMap();

    /// Destructs the hash map.
    ~HashMap();

    HashMap(const HashMap& map);

    HashMap(HashMap&& map);

    friend void swap<K, V>(HashMap& a, HashMap& b);

    HashMap& operator=(HashMap other);

    HashMap& operator=(HashMap&& other);

    /// Gets the element, or returns nullptr if it is not found.
    ///
    /// \param key The key
    /// \return the value, or nullptr
    V* get(K key);

    /// Adds a value to the map, or updates an existing value if the key
    /// already exists in the map. Note that adding a new value may
    /// trigger a rehash.
    ///
    /// \param key the key
    /// \param value the value
    void put(K key, V value);

    std::size_t size();

}; ///// Delete, iterator, then use this data structure

// ------------

constexpr std::size_t TREE_DEPTH = 4;
constexpr std::size_t TREE_LEN = (1 << TREE_DEPTH) - 1;
constexpr std::size_t DEFAULT_BUCKETS = 101;

template <typename K, typename V>
void swap(HashMap<K, V>& a, HashMap<K, V>& b) {
    using std::swap;
    swap(a.arr, b.arr);
    swap(a.bucket_count, b.bucket_count);
    swap(a.elems, b.elems);
}

template <typename K, typename V>
void HashMap<K, V>::rehash() {
    auto oldArr = arr;
    auto oldBC = bucketCount;
    bucketCount *= 2;
    arr = new std::pair<K, V>[TREE_LEN * bucketCount]();
    for (unsigned int i = 0; i < TREE_LEN * oldBC; i++) {
        if (oldArr[i] != defaulted) {
            put(oldArr[i].first, oldArr[i].second);
        }
    }
    delete[] oldArr;
}

template <typename K, typename V>
HashMap<K, V>::HashMap()
    : arr(new std::pair<K, V>[TREE_LEN * DEFAULT_BUCKETS]())
    , bucketCount(DEFAULT_BUCKETS)
    , elems(0) {};

template <typename K, typename V>
HashMap<K, V>::~HashMap() {
    delete[] arr;
}

template <typename K, typename V>
HashMap<K, V>::HashMap(const HashMap& map)
    : arr(new std::pair<K, V>[TREE_LEN * map.bucketCount])
    , bucketCount(map.bucketCount)
    , elems(map.elems) {
    std::copy(&map.arr[0], &map.arr[TREE_LEN * map.bucketCount], &arr[0]);
}

template <typename K, typename V>
HashMap<K, V>::HashMap(HashMap&& map)
    : arr(map.arr)
    , bucketCount(map.bucketCount)
    , elems(map.elems) {
    map.arr = nullptr;
}

template <typename K, typename V>
auto HashMap<K, V>::operator=(HashMap other) -> HashMap& {
    swap(*this, other);
    return *this;
}

template <typename K, typename V>
auto HashMap<K, V>::operator=(HashMap&& other) -> HashMap& {
    arr = other.arr;
    bucketCount = other.bucketCount;
    elems = other.elems;
    other.arr = nullptr;
}

template <typename K, typename V>
V* HashMap<K, V>::get(K key) {
    std::size_t base = (hash(key) % bucketCount) * TREE_LEN;
    for (unsigned int i = 0; i < TREE_DEPTH; i++) {
        auto& pair = arr[base];
        if (pair == defaulted) {
            return nullptr;
        } else if (pair.first == key) {
            return &pair.second;
        } else if (pair.first < key) {
            base++;
        } else {
            base += 1 << (TREE_DEPTH - i - 1);
        }
    }
    return nullptr;
}

template <typename K, typename V>
void HashMap<K, V>::put(K key, V value) {
    std::size_t base = (hash(key) % bucketCount) * TREE_LEN;
    for (unsigned int i = 0; i < TREE_DEPTH; i++) {
        auto& pair = arr[base];
        if (pair == defaulted) {
            pair = std::make_pair(key, value);
            return;
        } else if (pair.first == key) {
            pair.second = value;
            return;
        } else if (pair.first < key) {
            base++;
        } else {
            base += 1 << (TREE_DEPTH - i - 1);
        }
    }
    rehash();
    put(key, value);
}

template <typename K, typename V>
std::size_t HashMap<K, V>::size() {
    return elems;
}

#endif // HASHMAP_HPP
