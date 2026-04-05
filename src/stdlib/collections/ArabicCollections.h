#pragma once

#include <vector>
#include <deque>
#include <list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <memory>
#include <functional>
#include "ArabicTypes.h"
#include "ArabicMemoryManager.h"

namespace ArabicLanguage::StdLib {

    /**
     * @brief مكتبة الحاويات والمجموعات القياسية
     * توفر حاويات بيانات عامة مع دعم generics شامل
     */
    class ArabicCollections {
    public:
        // Singleton pattern
        static ArabicCollections& getInstance();

        // منع النسخ والتعيين
        ArabicCollections(const ArabicCollections&) = delete;
        ArabicCollections& operator=(const ArabicCollections&) = delete;

        // ══════════════════════════════════════════════════════════════
        // 📋 قائمة ديناميكية (List)
        // ══════════════════════════════════════════════════════════════

        template<typename T>
        class List {
    private:
        std::vector<T> data;

        public:
            List();
            List(const List<T>& other);
            List(List<T>&& other) noexcept;
            ~List() = default;

            // العمليات الأساسية
            void add(const T& item);
            void add(T&& item);
            void insert(size_t index, const T& item);
            void insert(size_t index, T&& item);
            void remove(size_t index);
            void remove(const T& item);
            void clear();

            // الوصول إلى العناصر
            T& get(size_t index);
            const T& get(size_t index) const;
            T& operator[](size_t index);
            const T& operator[](size_t index) const;
            T& first();
            const T& first() const;
            T& last();
            const T& last() const;

            // البحث والتحقق
            bool contains(const T& item) const;
            size_t indexOf(const T& item) const;
            size_t lastIndexOf(const T& item) const;
            bool isEmpty() const;

            // حجم القائمة
            size_t size() const;
            size_t capacity() const;
            void reserve(size_t capacity);
            void shrinkToFit();

            // العمليات المتقدمة
            void sort();
            void sort(std::function<bool(const T&, const T&)> comparator);
            void reverse();
            void shuffle();

            template<typename Predicate>
            void removeIf(Predicate pred);

            template<typename UnaryOp>
            void transform(UnaryOp op);

            // تقسيم ودمج
            List<T> subList(size_t fromIndex, size_t toIndex) const;
            void addAll(const List<T>& other);
            void addAll(List<T>&& other);

            // النسخ والمقارنة
            List<T>& operator=(const List<T>& other);
            List<T>& operator=(List<T>&& other) noexcept;
            bool operator==(const List<T>& other) const;
            bool operator!=(const List<T>& other) const;

            // التكرار
            class Iterator {
            private:
                typename std::vector<T>::iterator it;

            public:
                Iterator(typename std::vector<T>::iterator iterator) : it(iterator) {}
                T& operator*() { return *it; }
                Iterator& operator++() { ++it; return *this; }
                Iterator operator++(int) { Iterator temp = *this; ++it; return temp; }
                bool operator==(const Iterator& other) const { return it == other.it; }
                bool operator!=(const Iterator& other) const { return it != other.it; }
            };

            Iterator begin() { return Iterator(data.begin()); }
            Iterator end() { return Iterator(data.end()); }

            // دوال مساعدة
            std::string toString() const;
            void forEach(std::function<void(T&)> action);
            void forEach(std::function<void(const T&)> action) const;
        };

        // ══════════════════════════════════════════════════════════════
        // 🗺️ خريطة (Map)
        // ══════════════════════════════════════════════════════════════

        template<typename K, typename V>
        class Map {
    private:
        std::unordered_map<K, V> data;

        public:
            Map();
            Map(const Map<K, V>& other);
            Map(Map<K, V>&& other) noexcept;
            ~Map() = default;

            // العمليات الأساسية
            void put(const K& key, const V& value);
            void put(const K& key, V&& value);
            void put(K&& key, const V& value);
            void put(K&& key, V&& value);
            void remove(const K& key);
            void clear();

            // الوصول إلى القيم
            V& get(const K& key);
            const V& get(const K& key) const;
            V& operator[](const K& key);
            V& operator[](K&& key);

            // البحث والتحقق
            bool containsKey(const K& key) const;
            bool containsValue(const V& value) const;
            bool isEmpty() const;

            // حجم الخريطة
            size_t size() const;

            // الحصول على المفاتيح والقيم
            List<K> keys() const;
            List<V> values() const;
            List<std::pair<K, V>> entries() const;

            // العمليات المتقدمة
            V getOrDefault(const K& key, const V& defaultValue) const;
            V getOrDefault(const K& key, V&& defaultValue) const;

            template<typename Function>
            V computeIfAbsent(const K& key, Function mappingFunction);

            template<typename Function>
            V computeIfPresent(const K& key, Function mappingFunction);

            // النسخ والمقارنة
            Map<K, V>& operator=(const Map<K, V>& other);
            Map<K, V>& operator=(Map<K, V>&& other) noexcept;
            bool operator==(const Map<K, V>& other) const;
            bool operator!=(const Map<K, V>& other) const;

            // التكرار
            class Iterator {
            private:
                typename std::unordered_map<K, V>::iterator it;

            public:
                Iterator(typename std::unordered_map<K, V>::iterator iterator) : it(iterator) {}
                std::pair<const K, V>& operator*() { return *it; }
                Iterator& operator++() { ++it; return *this; }
                Iterator operator++(int) { Iterator temp = *this; ++it; return temp; }
                bool operator==(const Iterator& other) const { return it == other.it; }
                bool operator!=(const Iterator& other) const { return it != other.it; }
            };

            Iterator begin() { return Iterator(data.begin()); }
            Iterator end() { return Iterator(data.end()); }

            // دوال مساعدة
            std::string toString() const;
            void forEach(std::function<void(const K&, V&)> action);
            void forEach(std::function<void(const K&, const V&)> action) const;
        };

        // ══════════════════════════════════════════════════════════════
        // 🔗 مجموعة (Set)
        // ══════════════════════════════════════════════════════════════

        template<typename T>
        class Set {
    private:
        std::unordered_set<T> data;

        public:
            Set();
            Set(const Set<T>& other);
            Set(Set<T>&& other) noexcept;
            ~Set() = default;

            // العمليات الأساسية
            void add(const T& item);
            void add(T&& item);
            void remove(const T& item);
            void clear();

            // البحث والتحقق
            bool contains(const T& item) const;
            bool isEmpty() const;

            // حجم المجموعة
            size_t size() const;

            // العمليات المتقدمة
            void addAll(const Set<T>& other);
            void addAll(Set<T>&& other);
            void removeAll(const Set<T>& other);
            void retainAll(const Set<T>& other);

            // العمليات المجموعية
            Set<T> unionWith(const Set<T>& other) const;
            Set<T> intersectionWith(const Set<T>& other) const;
            Set<T> differenceWith(const Set<T>& other) const;

            // النسخ والمقارنة
            Set<T>& operator=(const Set<T>& other);
            Set<T>& operator=(Set<T>&& other) noexcept;
            bool operator==(const Set<T>& other) const;
            bool operator!=(const Set<T>& other) const;

            // التكرار
            class Iterator {
            private:
                typename std::unordered_set<T>::iterator it;

            public:
                Iterator(typename std::unordered_set<T>::iterator iterator) : it(iterator) {}
                const T& operator*() { return *it; }
                Iterator& operator++() { ++it; return *this; }
                Iterator operator++(int) { Iterator temp = *this; ++it; return temp; }
                bool operator==(const Iterator& other) const { return it == other.it; }
                bool operator!=(const Iterator& other) const { return it != other.it; }
            };

            Iterator begin() { return Iterator(data.begin()); }
            Iterator end() { return Iterator(data.end()); }

            // دوال مساعدة
            std::string toString() const;
            void forEach(std::function<void(const T&)> action) const;
        };

        // ══════════════════════════════════════════════════════════════
        // 📚 حاويات متقدمة
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief قائمة انتظار (Queue)
         */
        template<typename T>
        class Queue {
        private:
            std::deque<T> data;

        public:
            Queue() = default;

            void enqueue(const T& item);
            void enqueue(T&& item);
            T dequeue();
            T& peek() const;
            bool isEmpty() const;
            size_t size() const;
            void clear();
        };

        /**
         * @brief مكدس (Stack)
         */
        template<typename T>
        class Stack {
        private:
            std::vector<T> data;

        public:
            Stack() = default;

            void push(const T& item);
            void push(T&& item);
            T pop();
            T& peek() const;
            bool isEmpty() const;
            size_t size() const;
            void clear();
        };

        /**
         * @brief قائمة مرتبطة (LinkedList)
         */
        template<typename T>
        class LinkedList {
        private:
            std::list<T> data;

        public:
            LinkedList() = default;

            void addFirst(const T& item);
            void addLast(const T& item);
            void removeFirst();
            void removeLast();
            T& getFirst();
            T& getLast();
            bool isEmpty() const;
            size_t size() const;
            void clear();
        };

        // ══════════════════════════════════════════════════════════════
        // 🔍 خوارزميات البحث والترتيب
        // ══════════════════════════════════════════════════════════════

        class Algorithms {
        public:
            // خوارزميات البحث
            template<typename T>
            static size_t binarySearch(const List<T>& list, const T& target);

            template<typename K, typename V>
            static bool containsKey(const Map<K, V>& map, const K& key);

            template<typename T>
            static bool containsElement(const Set<T>& set, const T& element);

            // خوارزميات الترتيب
            template<typename T>
            static void quickSort(List<T>& list);

            template<typename T>
            static void mergeSort(List<T>& list);

            template<typename T>
            static void heapSort(List<T>& list);

            // خوارزميات أخرى
            template<typename T>
            static T findMax(const List<T>& list);

            template<typename T>
            static T findMin(const List<T>& list);

            template<typename T>
            static double findAverage(const List<T>& list);

            template<typename T>
            static T findMedian(List<T> list); // يعدل القائمة
        };

        // ══════════════════════════════════════════════════════════════
        // 📈 مراقبة الأداء
        // ══════════════════════════════════════════════════════════════

        class PerformanceMonitor {
        private:
            std::unordered_map<std::string, std::vector<size_t>> operationSizes;
            std::unordered_map<std::string, std::vector<double>> operationTimes;

        public:
            void recordOperation(const std::string& operation, size_t size, double timeMs);
            double getAverageTime(const std::string& operation) const;
            size_t getTotalOperations(const std::string& operation) const;
            std::vector<std::string> getPerformanceReport() const;
        };

    private:
        PerformanceMonitor perfMonitor;

        ArabicCollections() = default;
        ~ArabicCollections() = default;
    };

    // ══════════════════════════════════════════════════════════════
    // 🔧 دوال مساعدة للمكتبة القياسية
    // ══════════════════════════════════════════════════════════════

    namespace Collections {

        // إنشاء حاويات
        template<typename T>
        ArabicCollections::List<T> أنشئ_قائمة();

        template<typename K, typename V>
        ArabicCollections::Map<K, V> أنشئ_خريطة();

        template<typename T>
        ArabicCollections::Set<T> أنشئ_مجموعة();

        template<typename T>
        ArabicCollections::Queue<T> أنشئ_قائمة_انتظار();

        template<typename T>
        ArabicCollections::Stack<T> أنشئ_مكدس();

        // دوال مساعدة
        template<typename T>
        void اطبع_قائمة(const ArabicCollections::List<T>& قائمة);

        template<typename K, typename V>
        void اطبع_خريطة(const ArabicCollections::Map<K, V>& خريطة);

        template<typename T>
        void اطبع_مجموعة(const ArabicCollections::Set<T>& مجموعة);

    } // namespace Collections

} // namespace ArabicLanguage::StdLib
