#include "ArabicCollections.h"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <numeric>

namespace ArabicLanguage::StdLib {

    ArabicCollections& ArabicCollections::getInstance() {
        static ArabicCollections instance;
        return instance;
    }

    // ══════════════════════════════════════════════════════════════
    // 📋 تنفيذ القائمة الديناميكية (List)
    // ══════════════════════════════════════════════════════════════

template<typename T>
ArabicCollections::List<T>::List() {
    // استخدام إدارة الذاكرة القياسية
}

template<typename T>
ArabicCollections::List<T>::List(const List<T>& other) : data(other.data) {
    // استخدام إدارة الذاكرة القياسية
}

template<typename T>
ArabicCollections::List<T>::List(List<T>&& other) noexcept : data(std::move(other.data)) {
    // استخدام إدارة الذاكرة القياسية
}

    template<typename T>
    void ArabicCollections::List<T>::add(const T& item) {
        data.push_back(item);
    }

    template<typename T>
    void ArabicCollections::List<T>::add(T&& item) {
        data.push_back(std::move(item));
    }

    template<typename T>
    void ArabicCollections::List<T>::insert(size_t index, const T& item) {
        if (index > data.size()) index = data.size();
        data.insert(data.begin() + index, item);
    }

    template<typename T>
    void ArabicCollections::List<T>::insert(size_t index, T&& item) {
        if (index > data.size()) index = data.size();
        data.insert(data.begin() + index, std::move(item));
    }

    template<typename T>
    void ArabicCollections::List<T>::remove(size_t index) {
        if (index < data.size()) {
            data.erase(data.begin() + index);
        }
    }

    template<typename T>
    void ArabicCollections::List<T>::remove(const T& item) {
        auto it = std::find(data.begin(), data.end(), item);
        if (it != data.end()) {
            data.erase(it);
        }
    }

    template<typename T>
    void ArabicCollections::List<T>::clear() {
        data.clear();
    }

    template<typename T>
    T& ArabicCollections::List<T>::get(size_t index) {
        return data.at(index);
    }

    template<typename T>
    const T& ArabicCollections::List<T>::get(size_t index) const {
        return data.at(index);
    }

    template<typename T>
    T& ArabicCollections::List<T>::operator[](size_t index) {
        return data[index];
    }

    template<typename T>
    const T& ArabicCollections::List<T>::operator[](size_t index) const {
        return data[index];
    }

    template<typename T>
    T& ArabicCollections::List<T>::first() {
        return data.front();
    }

    template<typename T>
    const T& ArabicCollections::List<T>::first() const {
        return data.front();
    }

    template<typename T>
    T& ArabicCollections::List<T>::last() {
        return data.back();
    }

    template<typename T>
    const T& ArabicCollections::List<T>::last() const {
        return data.back();
    }

    template<typename T>
    bool ArabicCollections::List<T>::contains(const T& item) const {
        return std::find(data.begin(), data.end(), item) != data.end();
    }

    template<typename T>
    size_t ArabicCollections::List<T>::indexOf(const T& item) const {
        auto it = std::find(data.begin(), data.end(), item);
        return (it != data.end()) ? std::distance(data.begin(), it) : static_cast<size_t>(-1);
    }

    template<typename T>
    size_t ArabicCollections::List<T>::lastIndexOf(const T& item) const {
        auto it = std::find(data.rbegin(), data.rend(), item);
        return (it != data.rend()) ? std::distance(it, data.rend()) - 1 : static_cast<size_t>(-1);
    }

    template<typename T>
    bool ArabicCollections::List<T>::isEmpty() const {
        return data.empty();
    }

    template<typename T>
    size_t ArabicCollections::List<T>::size() const {
        return data.size();
    }

    template<typename T>
    size_t ArabicCollections::List<T>::capacity() const {
        return data.capacity();
    }

    template<typename T>
    void ArabicCollections::List<T>::reserve(size_t capacity) {
        data.reserve(capacity);
    }

    template<typename T>
    void ArabicCollections::List<T>::shrinkToFit() {
        data.shrink_to_fit();
    }

    template<typename T>
    void ArabicCollections::List<T>::sort() {
        std::sort(data.begin(), data.end());
    }

    template<typename T>
    void ArabicCollections::List<T>::sort(std::function<bool(const T&, const T&)> comparator) {
        std::sort(data.begin(), data.end(), comparator);
    }

    template<typename T>
    void ArabicCollections::List<T>::reverse() {
        std::reverse(data.begin(), data.end());
    }

    template<typename T>
    void ArabicCollections::List<T>::shuffle() {
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(data.begin(), data.end(), g);
    }

    template<typename T>
    template<typename Predicate>
    void ArabicCollections::List<T>::removeIf(Predicate pred) {
        data.erase(std::remove_if(data.begin(), data.end(), pred), data.end());
    }

    template<typename T>
    template<typename UnaryOp>
    void ArabicCollections::List<T>::transform(UnaryOp op) {
        std::transform(data.begin(), data.end(), data.begin(), op);
    }

    template<typename T>
    ArabicCollections::List<T> ArabicCollections::List<T>::subList(size_t fromIndex, size_t toIndex) const {
        List<T> result;
        if (fromIndex < toIndex && toIndex <= data.size()) {
            result.data.assign(data.begin() + fromIndex, data.begin() + toIndex);
        }
        return result;
    }

    template<typename T>
    void ArabicCollections::List<T>::addAll(const List<T>& other) {
        data.insert(data.end(), other.data.begin(), other.data.end());
    }

    template<typename T>
    void ArabicCollections::List<T>::addAll(List<T>&& other) {
        data.insert(data.end(),
                   std::make_move_iterator(other.data.begin()),
                   std::make_move_iterator(other.data.end()));
        other.data.clear();
    }

    template<typename T>
    ArabicCollections::List<T>& ArabicCollections::List<T>::operator=(const List<T>& other) {
        if (this != &other) {
            data = other.data;
        }
        return *this;
    }

    template<typename T>
    ArabicCollections::List<T>& ArabicCollections::List<T>::operator=(List<T>&& other) noexcept {
        if (this != &other) {
            data = std::move(other.data);
        }
        return *this;
    }

    template<typename T>
    bool ArabicCollections::List<T>::operator==(const List<T>& other) const {
        return data == other.data;
    }

    template<typename T>
    bool ArabicCollections::List<T>::operator!=(const List<T>& other) const {
        return data != other.data;
    }

    template<typename T>
    std::string ArabicCollections::List<T>::toString() const {
        std::stringstream ss;
        ss << "[";
        for (size_t i = 0; i < data.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << data[i];
        }
        ss << "]";
        return ss.str();
    }

    template<typename T>
    void ArabicCollections::List<T>::forEach(std::function<void(T&)> action) {
        for (auto& item : data) {
            action(item);
        }
    }

    template<typename T>
    void ArabicCollections::List<T>::forEach(std::function<void(const T&)> action) const {
        for (const auto& item : data) {
            action(item);
        }
    }

    // ══════════════════════════════════════════════════════════════
    // 🗺️ تنفيذ الخريطة (Map)
    // ══════════════════════════════════════════════════════════════

template<typename K, typename V>
ArabicCollections::Map<K, V>::Map() {
    // استخدام إدارة الذاكرة القياسية
}

template<typename K, typename V>
ArabicCollections::Map<K, V>::Map(const Map<K, V>& other) : data(other.data) {
    // استخدام إدارة الذاكرة القياسية
}

template<typename K, typename V>
ArabicCollections::Map<K, V>::Map(Map<K, V>&& other) noexcept : data(std::move(other.data)) {
    // استخدام إدارة الذاكرة القياسية
}

    template<typename K, typename V>
    void ArabicCollections::Map<K, V>::put(const K& key, const V& value) {
        data[key] = value;
    }

    template<typename K, typename V>
    void ArabicCollections::Map<K, V>::put(const K& key, V&& value) {
        data[key] = std::move(value);
    }

    template<typename K, typename V>
    void ArabicCollections::Map<K, V>::put(K&& key, const V& value) {
        data[std::move(key)] = value;
    }

    template<typename K, typename V>
    void ArabicCollections::Map<K, V>::put(K&& key, V&& value) {
        data[std::move(key)] = std::move(value);
    }

    template<typename K, typename V>
    void ArabicCollections::Map<K, V>::remove(const K& key) {
        data.erase(key);
    }

    template<typename K, typename V>
    void ArabicCollections::Map<K, V>::clear() {
        data.clear();
    }

    template<typename K, typename V>
    V& ArabicCollections::Map<K, V>::get(const K& key) {
        return data.at(key);
    }

    template<typename K, typename V>
    const V& ArabicCollections::Map<K, V>::get(const K& key) const {
        return data.at(key);
    }

    template<typename K, typename V>
    V& ArabicCollections::Map<K, V>::operator[](const K& key) {
        return data[key];
    }

    template<typename K, typename V>
    V& ArabicCollections::Map<K, V>::operator[](K&& key) {
        return data[std::move(key)];
    }

    template<typename K, typename V>
    bool ArabicCollections::Map<K, V>::containsKey(const K& key) const {
        return data.find(key) != data.end();
    }

    template<typename K, typename V>
    bool ArabicCollections::Map<K, V>::containsValue(const V& value) const {
        for (const auto& pair : data) {
            if (pair.second == value) return true;
        }
        return false;
    }

    template<typename K, typename V>
    bool ArabicCollections::Map<K, V>::isEmpty() const {
        return data.empty();
    }

    template<typename K, typename V>
    size_t ArabicCollections::Map<K, V>::size() const {
        return data.size();
    }

    template<typename K, typename V>
    ArabicCollections::List<K> ArabicCollections::Map<K, V>::keys() const {
        List<K> result;
        for (const auto& pair : data) {
            result.add(pair.first);
        }
        return result;
    }

    template<typename K, typename V>
    ArabicCollections::List<V> ArabicCollections::Map<K, V>::values() const {
        List<V> result;
        for (const auto& pair : data) {
            result.add(pair.second);
        }
        return result;
    }

    template<typename K, typename V>
    ArabicCollections::List<std::pair<K, V>> ArabicCollections::Map<K, V>::entries() const {
        List<std::pair<K, V>> result;
        for (const auto& pair : data) {
            result.add(pair);
        }
        return result;
    }

    template<typename K, typename V>
    V ArabicCollections::Map<K, V>::getOrDefault(const K& key, const V& defaultValue) const {
        auto it = data.find(key);
        return (it != data.end()) ? it->second : defaultValue;
    }

    template<typename K, typename V>
    V ArabicCollections::Map<K, V>::getOrDefault(const K& key, V&& defaultValue) const {
        auto it = data.find(key);
        return (it != data.end()) ? it->second : std::move(defaultValue);
    }

    template<typename K, typename V>
    template<typename Function>
    V ArabicCollections::Map<K, V>::computeIfAbsent(const K& key, Function mappingFunction) {
        auto it = data.find(key);
        if (it == data.end()) {
            V value = mappingFunction(key);
            data[key] = value;
            return value;
        }
        return it->second;
    }

    template<typename K, typename V>
    template<typename Function>
    V ArabicCollections::Map<K, V>::computeIfPresent(const K& key, Function mappingFunction) {
        auto it = data.find(key);
        if (it != data.end()) {
            V newValue = mappingFunction(key, it->second);
            it->second = newValue;
            return newValue;
        }
        return V{};
    }

    template<typename K, typename V>
    ArabicCollections::Map<K, V>& ArabicCollections::Map<K, V>::operator=(const Map<K, V>& other) {
        if (this != &other) {
            data = other.data;
        }
        return *this;
    }

    template<typename K, typename V>
    ArabicCollections::Map<K, V>& ArabicCollections::Map<K, V>::operator=(Map<K, V>&& other) noexcept {
        if (this != &other) {
            data = std::move(other.data);
        }
        return *this;
    }

    template<typename K, typename V>
    bool ArabicCollections::Map<K, V>::operator==(const Map<K, V>& other) const {
        return data == other.data;
    }

    template<typename K, typename V>
    bool ArabicCollections::Map<K, V>::operator!=(const Map<K, V>& other) const {
        return data != other.data;
    }

    template<typename K, typename V>
    std::string ArabicCollections::Map<K, V>::toString() const {
        std::stringstream ss;
        ss << "{";
        bool first = true;
        for (const auto& pair : data) {
            if (!first) ss << ", ";
            ss << pair.first << ": " << pair.second;
            first = false;
        }
        ss << "}";
        return ss.str();
    }

    template<typename K, typename V>
    void ArabicCollections::Map<K, V>::forEach(std::function<void(const K&, V&)> action) {
        for (auto& pair : data) {
            action(pair.first, pair.second);
        }
    }

    template<typename K, typename V>
    void ArabicCollections::Map<K, V>::forEach(std::function<void(const K&, const V&)> action) const {
        for (const auto& pair : data) {
            action(pair.first, pair.second);
        }
    }

    // ══════════════════════════════════════════════════════════════
    // 🔗 تنفيذ المجموعة (Set)
    // ══════════════════════════════════════════════════════════════

template<typename T>
ArabicCollections::Set<T>::Set() {
    // استخدام إدارة الذاكرة القياسية
}

template<typename T>
ArabicCollections::Set<T>::Set(const Set<T>& other) : data(other.data) {
    // استخدام إدارة الذاكرة القياسية
}

template<typename T>
ArabicCollections::Set<T>::Set(Set<T>&& other) noexcept : data(std::move(other.data)) {
    // استخدام إدارة الذاكرة القياسية
}

    template<typename T>
    void ArabicCollections::Set<T>::add(const T& item) {
        data.insert(item);
    }

    template<typename T>
    void ArabicCollections::Set<T>::add(T&& item) {
        data.insert(std::move(item));
    }

    template<typename T>
    void ArabicCollections::Set<T>::remove(const T& item) {
        data.erase(item);
    }

    template<typename T>
    void ArabicCollections::Set<T>::clear() {
        data.clear();
    }

    template<typename T>
    bool ArabicCollections::Set<T>::contains(const T& item) const {
        return data.find(item) != data.end();
    }

    template<typename T>
    bool ArabicCollections::Set<T>::isEmpty() const {
        return data.empty();
    }

    template<typename T>
    size_t ArabicCollections::Set<T>::size() const {
        return data.size();
    }

    template<typename T>
    void ArabicCollections::Set<T>::addAll(const Set<T>& other) {
        data.insert(other.data.begin(), other.data.end());
    }

    template<typename T>
    void ArabicCollections::Set<T>::addAll(Set<T>&& other) {
        data.insert(std::make_move_iterator(other.data.begin()),
                   std::make_move_iterator(other.data.end()));
        other.data.clear();
    }

    template<typename T>
    void ArabicCollections::Set<T>::removeAll(const Set<T>& other) {
        for (const auto& item : other.data) {
            data.erase(item);
        }
    }

    template<typename T>
    void ArabicCollections::Set<T>::retainAll(const Set<T>& other) {
        for (auto it = data.begin(); it != data.end();) {
            if (other.data.find(*it) == other.data.end()) {
                it = data.erase(it);
            } else {
                ++it;
            }
        }
    }

    template<typename T>
    ArabicCollections::Set<T> ArabicCollections::Set<T>::unionWith(const Set<T>& other) const {
        Set<T> result = *this;
        result.addAll(other);
        return result;
    }

    template<typename T>
    ArabicCollections::Set<T> ArabicCollections::Set<T>::intersectionWith(const Set<T>& other) const {
        Set<T> result;
        for (const auto& item : data) {
            if (other.contains(item)) {
                result.add(item);
            }
        }
        return result;
    }

    template<typename T>
    ArabicCollections::Set<T> ArabicCollections::Set<T>::differenceWith(const Set<T>& other) const {
        Set<T> result;
        for (const auto& item : data) {
            if (!other.contains(item)) {
                result.add(item);
            }
        }
        return result;
    }

    template<typename T>
    ArabicCollections::Set<T>& ArabicCollections::Set<T>::operator=(const Set<T>& other) {
        if (this != &other) {
            data = other.data;
        }
        return *this;
    }

    template<typename T>
    ArabicCollections::Set<T>& ArabicCollections::Set<T>::operator=(Set<T>&& other) noexcept {
        if (this != &other) {
            data = std::move(other.data);
        }
        return *this;
    }

    template<typename T>
    bool ArabicCollections::Set<T>::operator==(const Set<T>& other) const {
        return data == other.data;
    }

    template<typename T>
    bool ArabicCollections::Set<T>::operator!=(const Set<T>& other) const {
        return data != other.data;
    }

    template<typename T>
    std::string ArabicCollections::Set<T>::toString() const {
        std::stringstream ss;
        ss << "{";
        bool first = true;
        for (const auto& item : data) {
            if (!first) ss << ", ";
            ss << item;
            first = false;
        }
        ss << "}";
        return ss.str();
    }

    template<typename T>
    void ArabicCollections::Set<T>::forEach(std::function<void(const T&)> action) const {
        for (const auto& item : data) {
            action(item);
        }
    }

    // ══════════════════════════════════════════════════════════════
    // 📚 تنفيذ الحاويات المتقدمة
    // ══════════════════════════════════════════════════════════════

    template<typename T>
    void ArabicCollections::Queue<T>::enqueue(const T& item) {
        data.push_back(item);
    }

    template<typename T>
    void ArabicCollections::Queue<T>::enqueue(T&& item) {
        data.push_back(std::move(item));
    }

    template<typename T>
    T ArabicCollections::Queue<T>::dequeue() {
        if (data.empty()) throw std::runtime_error("Queue is empty");
        T front = std::move(data.front());
        data.pop_front();
        return front;
    }

    template<typename T>
    T& ArabicCollections::Queue<T>::peek() const {
        if (data.empty()) throw std::runtime_error("Queue is empty");
        return const_cast<T&>(data.front());
    }

    template<typename T>
    bool ArabicCollections::Queue<T>::isEmpty() const {
        return data.empty();
    }

    template<typename T>
    size_t ArabicCollections::Queue<T>::size() const {
        return data.size();
    }

    template<typename T>
    void ArabicCollections::Queue<T>::clear() {
        data.clear();
    }

    template<typename T>
    void ArabicCollections::Stack<T>::push(const T& item) {
        data.push_back(item);
    }

    template<typename T>
    void ArabicCollections::Stack<T>::push(T&& item) {
        data.push_back(std::move(item));
    }

    template<typename T>
    T ArabicCollections::Stack<T>::pop() {
        if (data.empty()) throw std::runtime_error("Stack is empty");
        T top = std::move(data.back());
        data.pop_back();
        return top;
    }

    template<typename T>
    T& ArabicCollections::Stack<T>::peek() const {
        if (data.empty()) throw std::runtime_error("Stack is empty");
        return const_cast<T&>(data.back());
    }

    template<typename T>
    bool ArabicCollections::Stack<T>::isEmpty() const {
        return data.empty();
    }

    template<typename T>
    size_t ArabicCollections::Stack<T>::size() const {
        return data.size();
    }

    template<typename T>
    void ArabicCollections::Stack<T>::clear() {
        data.clear();
    }

    template<typename T>
    void ArabicCollections::LinkedList<T>::addFirst(const T& item) {
        data.push_front(item);
    }

    template<typename T>
    void ArabicCollections::LinkedList<T>::addLast(const T& item) {
        data.push_back(item);
    }

    template<typename T>
    void ArabicCollections::LinkedList<T>::removeFirst() {
        if (!data.empty()) data.pop_front();
    }

    template<typename T>
    void ArabicCollections::LinkedList<T>::removeLast() {
        if (!data.empty()) data.pop_back();
    }

    template<typename T>
    T& ArabicCollections::LinkedList<T>::getFirst() {
        return data.front();
    }

    template<typename T>
    T& ArabicCollections::LinkedList<T>::getLast() {
        return data.back();
    }

    template<typename T>
    bool ArabicCollections::LinkedList<T>::isEmpty() const {
        return data.empty();
    }

    template<typename T>
    size_t ArabicCollections::LinkedList<T>::size() const {
        return data.size();
    }

    template<typename T>
    void ArabicCollections::LinkedList<T>::clear() {
        data.clear();
    }

    // ══════════════════════════════════════════════════════════════
    // 🔍 تنفيذ خوارزميات البحث والترتيب
    // ══════════════════════════════════════════════════════════════

    template<typename T>
    size_t ArabicCollections::Algorithms::binarySearch(const List<T>& list, const T& target) {
        auto it = std::lower_bound(list.data.begin(), list.data.end(), target);
        if (it != list.data.end() && *it == target) {
            return std::distance(list.data.begin(), it);
        }
        return static_cast<size_t>(-1);
    }

    template<typename K, typename V>
    bool ArabicCollections::Algorithms::containsKey(const Map<K, V>& map, const K& key) {
        return map.containsKey(key);
    }

    template<typename T>
    bool ArabicCollections::Algorithms::containsElement(const Set<T>& set, const T& element) {
        return set.contains(element);
    }

    template<typename T>
    void ArabicCollections::Algorithms::quickSort(List<T>& list) {
        std::sort(list.data.begin(), list.data.end());
    }

    template<typename T>
    void ArabicCollections::Algorithms::mergeSort(List<T>& list) {
        // تنفيذ Merge Sort بسيط
        if (list.size() <= 1) return;

        size_t mid = list.size() / 2;
        List<T> left = list.subList(0, mid);
        List<T> right = list.subList(mid, list.size());

        mergeSort(left);
        mergeSort(right);

        size_t i = 0, j = 0, k = 0;
        while (i < left.size() && j < right.size()) {
            if (left[i] <= right[j]) {
                list[k++] = left[i++];
            } else {
                list[k++] = right[j++];
            }
        }

        while (i < left.size()) list[k++] = left[i++];
        while (j < right.size()) list[k++] = right[j++];
    }

    template<typename T>
    void ArabicCollections::Algorithms::heapSort(List<T>& list) {
        std::make_heap(list.data.begin(), list.data.end());
        std::sort_heap(list.data.begin(), list.data.end());
    }

    template<typename T>
    T ArabicCollections::Algorithms::findMax(const List<T>& list) {
        if (list.isEmpty()) throw std::runtime_error("List is empty");
        return *std::max_element(list.data.begin(), list.data.end());
    }

    template<typename T>
    T ArabicCollections::Algorithms::findMin(const List<T>& list) {
        if (list.isEmpty()) throw std::runtime_error("List is empty");
        return *std::min_element(list.data.begin(), list.data.end());
    }

    template<typename T>
    double ArabicCollections::Algorithms::findAverage(const List<T>& list) {
        if (list.isEmpty()) return 0.0;
        double sum = 0.0;
        for (const auto& item : list.data) {
            sum += static_cast<double>(item);
        }
        return sum / list.size();
    }

    template<typename T>
    T ArabicCollections::Algorithms::findMedian(List<T> list) {
        if (list.isEmpty()) throw std::runtime_error("List is empty");

        list.sort();
        size_t mid = list.size() / 2;

        if (list.size() % 2 == 0) {
            // متوسط العنصرين الأوسطين
            return (list[mid - 1] + list[mid]) / 2;
        } else {
            return list[mid];
        }
    }

    // ══════════════════════════════════════════════════════════════
    // 📈 تنفيذ مراقب الأداء
    // ══════════════════════════════════════════════════════════════

    void ArabicCollections::PerformanceMonitor::recordOperation(const std::string& operation, size_t size, double timeMs) {
        operationSizes[operation].push_back(size);
        operationTimes[operation].push_back(timeMs);
    }

    double ArabicCollections::PerformanceMonitor::getAverageTime(const std::string& operation) const {
        auto it = operationTimes.find(operation);
        if (it == operationTimes.end() || it->second.empty()) return 0.0;

        double sum = 0.0;
        for (double time : it->second) sum += time;
        return sum / it->second.size();
    }

    size_t ArabicCollections::PerformanceMonitor::getTotalOperations(const std::string& operation) const {
        auto it = operationTimes.find(operation);
        return it != operationTimes.end() ? it->second.size() : 0;
    }

    std::vector<std::string> ArabicCollections::PerformanceMonitor::getPerformanceReport() const {
        std::vector<std::string> report;
        report.push_back("=== تقرير أداء الحاويات ===");

        for (const auto& pair : operationTimes) {
            const std::string& operation = pair.first;
            double avgTime = getAverageTime(operation);
            size_t totalOps = getTotalOperations(operation);

            report.push_back("العملية: " + operation);
            report.push_back("  العدد الإجمالي: " + std::to_string(totalOps));
            report.push_back("  المتوسط الزمني: " + std::to_string(avgTime) + " ms");
        }

        return report;
    }

    // ══════════════════════════════════════════════════════════════
    // 🔧 تنفيذ دوال المساعدة
    // ══════════════════════════════════════════════════════════════

    namespace Collections {

        template<typename T>
        ArabicCollections::List<T> أنشئ_قائمة() {
            return ArabicCollections::List<T>();
        }

        template<typename K, typename V>
        ArabicCollections::Map<K, V> أنشئ_خريطة() {
            return ArabicCollections::Map<K, V>();
        }

        template<typename T>
        ArabicCollections::Set<T> أنشئ_مجموعة() {
            return ArabicCollections::Set<T>();
        }

        template<typename T>
        ArabicCollections::Queue<T> أنشئ_قائمة_انتظار() {
            return ArabicCollections::Queue<T>();
        }

        template<typename T>
        ArabicCollections::Stack<T> أنشئ_مكدس() {
            return ArabicCollections::Stack<T>();
        }

        template<typename T>
        void اطبع_قائمة(const ArabicCollections::List<T>& قائمة) {
            std::cout << قائمة.toString() << std::endl;
        }

        template<typename K, typename V>
        void اطبع_خريطة(const ArabicCollections::Map<K, V>& خريطة) {
            std::cout << خريطة.toString() << std::endl;
        }

        template<typename T>
        void اطبع_مجموعة(const ArabicCollections::Set<T>& مجموعة) {
            std::cout << مجموعة.toString() << std::endl;
        }

    } // namespace Collections

    // ══════════════════════════════════════════════════════════════
    // 📋 تنفيذات القوالب المحددة المطلوبة
    // ══════════════════════════════════════════════════════════════

    // تنفيذات القوالب المطلوبة للاستخدام الشائع
    template class ArabicCollections::List<int>;
    template class ArabicCollections::List<double>;
    template class ArabicCollections::List<std::string>;

    template class ArabicCollections::Map<std::string, int>;
    template class ArabicCollections::Map<std::string, double>;
    template class ArabicCollections::Map<std::string, std::string>;
    template class ArabicCollections::Map<int, std::string>;

    template class ArabicCollections::Set<int>;
    template class ArabicCollections::Set<double>;
    template class ArabicCollections::Set<std::string>;

    template class ArabicCollections::Queue<int>;
    template class ArabicCollections::Queue<double>;
    template class ArabicCollections::Queue<std::string>;

    template class ArabicCollections::Stack<int>;
    template class ArabicCollections::Stack<double>;
    template class ArabicCollections::Stack<std::string>;

    template class ArabicCollections::LinkedList<int>;
    template class ArabicCollections::LinkedList<double>;
    template class ArabicCollections::LinkedList<std::string>;

} // namespace ArabicLanguage::StdLib
