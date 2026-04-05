#pragma once

#include <vector>
#include <cmath>
#include <limits>
#include <random>
#include <complex>
#include <numeric>
#include "ArabicTypes.h"
#include "ArabicMemoryManager.h"

namespace ArabicLanguage::StdLib {

    /**
     * @brief مكتبة الرياضيات المتقدمة
     * توفر دوال رياضية متقدمة، متجهات، مصفوفات، وحسابات إحصائية
     */
    class ArabicMath {
    public:
        // Singleton pattern
        static ArabicMath& getInstance();

        // منع النسخ والتعيين
        ArabicMath(const ArabicMath&) = delete;
        ArabicMath& operator=(const ArabicMath&) = delete;

        // ══════════════════════════════════════════════════════════════
        // 🔢 الثوابت الرياضية
        // ══════════════════════════════════════════════════════════════

        static constexpr double PI = 3.14159265358979323846;
        static constexpr double E = 2.71828182845904523536;
        static constexpr double PHI = 1.61803398874989484820; // نسبة ذهبية
        static constexpr double SQRT2 = 1.41421356237309504880;
        static constexpr double SQRT3 = 1.73205080756887729352;

        // ══════════════════════════════════════════════════════════════
        // 📊 دوال رياضية أساسية
        // ══════════════════════════════════════════════════════════════

        class BasicMath {
        public:
            // دوال أساسية
            static double abs(double x) { return std::abs(x); }
            static double sqrt(double x) { return std::sqrt(x); }
            static double pow(double base, double exponent) { return std::pow(base, exponent); }
            static double exp(double x) { return std::exp(x); }
            static double log(double x) { return std::log(x); }
            static double log10(double x) { return std::log10(x); }
            static double log2(double x) { return std::log2(x); }

            // دوال مثلثية
            static double sin(double x) { return std::sin(x); }
            static double cos(double x) { return std::cos(x); }
            static double tan(double x) { return std::tan(x); }
            static double asin(double x) { return std::asin(x); }
            static double acos(double x) { return std::acos(x); }
            static double atan(double x) { return std::atan(x); }
            static double atan2(double y, double x) { return std::atan2(y, x); }

            // دوال مثلثية معكوسة
            static double sinh(double x) { return std::sinh(x); }
            static double cosh(double x) { return std::cosh(x); }
            static double tanh(double x) { return std::tanh(x); }
            static double asinh(double x) { return std::asinh(x); }
            static double acosh(double x) { return std::acosh(x); }
            static double atanh(double x) { return std::atanh(x); }

            // دوال تقريب
            static double ceil(double x) { return std::ceil(x); }
            static double floor(double x) { return std::floor(x); }
            static double round(double x) { return std::round(x); }
            static double trunc(double x) { return std::trunc(x); }

            // دوال أخرى
            static double fmod(double x, double y) { return std::fmod(x, y); }
            static double remainder(double x, double y) { return std::remainder(x, y); }
            static double fmax(double x, double y) { return std::fmax(x, y); }
            static double fmin(double x, double y) { return std::fmin(x, y); }

            // دوال مقارنة
            static bool isNaN(double x) { return std::isnan(x); }
            static bool isInfinite(double x) { return std::isinf(x); }
            static bool isFinite(double x) { return std::isfinite(x); }

            // دوال خاصة
            static double erf(double x); // دالة الخطأ
            static double erfc(double x); // دالة الخطأ المتممة
            static double gamma(double x); // دالة غاما
            static double lgamma(double x); // لوغاريتم دالة غاما
        };

        // ══════════════════════════════════════════════════════════════
        // 📈 حسابات إحصائية
        // ══════════════════════════════════════════════════════════════

        class Statistics {
        public:
            template<typename T>
            static double mean(const std::vector<T>& data) {
                if (data.empty()) return 0.0;
                double sum = std::accumulate(data.begin(), data.end(), 0.0);
                return sum / data.size();
            }

            template<typename T>
            static double median(std::vector<T> data) {
                if (data.empty()) return 0.0;
                std::sort(data.begin(), data.end());
                size_t n = data.size();
                if (n % 2 == 0) {
                    return (data[n/2 - 1] + data[n/2]) / 2.0;
                } else {
                    return data[n/2];
                }
            }

            template<typename T>
            static double mode(const std::vector<T>& data) {
                if (data.empty()) return 0.0;
                std::map<T, int> frequency;
                for (const auto& value : data) {
                    frequency[value]++;
                }
                auto maxFreq = std::max_element(frequency.begin(), frequency.end(),
                    [](const auto& a, const auto& b) { return a.second < b.second; });
                return maxFreq->first;
            }

            template<typename T>
            static double variance(const std::vector<T>& data) {
                if (data.size() < 2) return 0.0;
                double m = mean(data);
                double sum = 0.0;
                for (const auto& value : data) {
                    double diff = value - m;
                    sum += diff * diff;
                }
                return sum / (data.size() - 1);
            }

            template<typename T>
            static double standardDeviation(const std::vector<T>& data) {
                return std::sqrt(variance(data));
            }

            template<typename T>
            static double covariance(const std::vector<T>& x, const std::vector<T>& y) {
                if (x.size() != y.size() || x.size() < 2) return 0.0;
                double mx = mean(x);
                double my = mean(y);
                double sum = 0.0;
                for (size_t i = 0; i < x.size(); ++i) {
                    sum += (x[i] - mx) * (y[i] - my);
                }
                return sum / (x.size() - 1);
            }

            template<typename T>
            static double correlation(const std::vector<T>& x, const std::vector<T>& y) {
                double cov = covariance(x, y);
                double sd_x = standardDeviation(x);
                double sd_y = standardDeviation(y);
                if (sd_x == 0.0 || sd_y == 0.0) return 0.0;
                return cov / (sd_x * sd_y);
            }

            template<typename T>
            static T min(const std::vector<T>& data) {
                if (data.empty()) throw std::runtime_error("Empty data");
                return *std::min_element(data.begin(), data.end());
            }

            template<typename T>
            static T max(const std::vector<T>& data) {
                if (data.empty()) throw std::runtime_error("Empty data");
                return *std::max_element(data.begin(), data.end());
            }

            template<typename T>
            static std::vector<T> percentile(const std::vector<T>& data, double p) {
                if (data.empty() || p < 0.0 || p > 100.0) return {};

                std::vector<T> sorted = data;
                std::sort(sorted.begin(), sorted.end());

                double rank = (p / 100.0) * (sorted.size() - 1);
                size_t lower = static_cast<size_t>(rank);
                size_t upper = std::min(lower + 1, sorted.size() - 1);

                if (lower == upper) return {sorted[lower]};

                double fraction = rank - lower;
                return {sorted[lower] + fraction * (sorted[upper] - sorted[lower])};
            }
        };

        // ══════════════════════════════════════════════════════════════
        // 📐 متجهات رياضية
        // ══════════════════════════════════════════════════════════════

        template<typename T = double>
        class Vector {
    private:
        std::vector<T> data;

        public:
            Vector(size_t size = 0, T defaultValue = T{});
            Vector(const std::vector<T>& values);
            Vector(std::vector<T>&& values) noexcept;
            Vector(const Vector<T>& other);
            Vector(Vector<T>&& other) noexcept;
            ~Vector() = default;

            // العمليات الأساسية
            void resize(size_t newSize, T defaultValue = T{});
            void reserve(size_t capacity);
            size_t size() const { return data.size(); }
            bool empty() const { return data.empty(); }

            // الوصول إلى العناصر
            T& operator[](size_t index) { return data[index]; }
            const T& operator[](size_t index) const { return data[index]; }
            T& at(size_t index) { return data.at(index); }
            const T& at(size_t index) const { return data.at(index); }

            // العمليات المتجهية
            T dot(const Vector<T>& other) const; // الضرب النقطي
            double magnitude() const; // المقدار
            Vector<T> normalize() const; // التطبيع
            Vector<T> cross(const Vector<T>& other) const; // الضرب المتقاطع (3D فقط)

            // العمليات الحسابية
            Vector<T> operator+(const Vector<T>& other) const;
            Vector<T> operator-(const Vector<T>& other) const;
            Vector<T> operator*(T scalar) const;
            Vector<T> operator/(T scalar) const;
            Vector<T>& operator+=(const Vector<T>& other);
            Vector<T>& operator-=(const Vector<T>& other);
            Vector<T>& operator*=(T scalar);
            Vector<T>& operator/=(T scalar);

            // العمليات المتقدمة
            double distance(const Vector<T>& other) const;
            double angle(const Vector<T>& other) const; // بالراديان
            Vector<T> project(const Vector<T>& onto) const; // الإسقاط
            Vector<T> reflect(const Vector<T>& normal) const; // الانعكاس

            // النسخ والمقارنة
            Vector<T>& operator=(const Vector<T>& other) = default;
            Vector<T>& operator=(Vector<T>&& other) noexcept = default;
            bool operator==(const Vector<T>& other) const;
            bool operator!=(const Vector<T>& other) const;

            // دوال مساعدة
            std::string toString() const;
            std::vector<T>& getData() { return data; }
            const std::vector<T>& getData() const { return data; }

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
        };

        // ══════════════════════════════════════════════════════════════
        // 🔢 مصفوفات رياضية
        // ══════════════════════════════════════════════════════════════

        template<typename T = double>
        class Matrix {
    private:
        std::vector<std::vector<T>> data;
        size_t rows, cols;

        public:
            Matrix(size_t rows = 0, size_t cols = 0, T defaultValue = T{});
            Matrix(const std::vector<std::vector<T>>& values);
            Matrix(const Matrix<T>& other);
            Matrix(Matrix<T>&& other) noexcept;
            ~Matrix() = default;

            // الخصائص الأساسية
            size_t getRows() const { return rows; }
            size_t getCols() const { return cols; }
            bool isSquare() const { return rows == cols; }
            bool isEmpty() const { return rows == 0 || cols == 0; }

            // الوصول إلى العناصر
            T& at(size_t row, size_t col) { return data.at(row).at(col); }
            const T& at(size_t row, size_t col) const { return data.at(row).at(col); }
            T& operator()(size_t row, size_t col) { return at(row, col); }
            const T& operator()(size_t row, size_t col) const { return at(row, col); }

            // العمليات الأساسية
            Matrix<T> transpose() const;
            T determinant() const; // للمصفوفات المربعة الصغيرة
            Matrix<T> inverse() const; // للمصفوفات المربعة

            // العمليات الحسابية
            Matrix<T> operator+(const Matrix<T>& other) const;
            Matrix<T> operator-(const Matrix<T>& other) const;
            Matrix<T> operator*(const Matrix<T>& other) const;
            Matrix<T> operator*(T scalar) const;
            Vector<T> operator*(const Vector<T>& vec) const;

            // عمليات المصفوفات الخاصة
            static Matrix<T> identity(size_t size);
            static Matrix<T> zeros(size_t rows, size_t cols);
            static Matrix<T> ones(size_t rows, size_t cols);
            Matrix<T> diagonal() const;

            // خوارزميات المصفوفات
            Matrix<T> luDecomposition() const;
            std::pair<Matrix<T>, Matrix<T>> qrDecomposition() const;
            Vector<T> solveLinearSystem(const Vector<T>& b) const;

            // دوال مساعدة
            std::string toString() const;
            void print() const;
            bool isSymmetric() const;
            bool isOrthogonal() const;

            // الحصول على البيانات
            const std::vector<std::vector<T>>& getData() const { return data; }
        };

        // ══════════════════════════════════════════════════════════════
        // 🔄 الأعداد المركبة
        // ══════════════════════════════════════════════════════════════

        class Complex {
        private:
            double real, imag;

        public:
            Complex(double r = 0.0, double i = 0.0) : real(r), imag(i) {}

            // الخصائص الأساسية
            double getReal() const { return real; }
            double getImag() const { return imag; }
            double magnitude() const { return std::sqrt(real*real + imag*imag); }
            double phase() const { return std::atan2(imag, real); }
            Complex conjugate() const { return Complex(real, -imag); }

            // العمليات الحسابية
            Complex operator+(const Complex& other) const;
            Complex operator-(const Complex& other) const;
            Complex operator*(const Complex& other) const;
            Complex operator/(const Complex& other) const;
            Complex operator*(double scalar) const;
            Complex operator/(double scalar) const;

            // دوال رياضية
            Complex sqrt() const;
            Complex exp() const;
            Complex log() const;
            Complex pow(const Complex& exponent) const;
            Complex sin() const;
            Complex cos() const;

            // دوال مساعدة
            std::string toString() const;
            static Complex fromPolar(double magnitude, double phase);
        };

        // ══════════════════════════════════════════════════════════════
        // 🎲 مولدات الأرقام العشوائية
        // ══════════════════════════════════════════════════════════════

        class Random {
        private:
            std::mt19937 generator;
            std::uniform_real_distribution<double> uniformDist;

        public:
            Random(unsigned int seed = std::random_device{}());

            // أرقام عشوائية أساسية
            double nextDouble(); // [0, 1)
            double nextDouble(double min, double max);
            int nextInt(int min, int max);
            bool nextBool();

            // توزيعات إحصائية
            double nextGaussian(double mean = 0.0, double stddev = 1.0);
            double nextExponential(double lambda);
            int nextPoisson(double lambda);

            // أرقام عشوائية في حاويات
            template<typename T>
            void shuffle(std::vector<T>& container);

            template<typename T>
            T choose(const std::vector<T>& container);

            // إعادة تعيين البذرة
            void setSeed(unsigned int seed);
        };

        // ══════════════════════════════════════════════════════════════
        // 🔬 حسابات متقدمة
        // ══════════════════════════════════════════════════════════════

        class AdvancedMath {
        public:
            // معادلات تفاضلية
            static double numericalDerivative(double (*f)(double), double x, double h = 1e-6);
            static double numericalIntegral(double (*f)(double), double a, double b, size_t steps = 1000);

            // معادلات تفاضلية جزئية
            static double solveODE_Euler(double (*f)(double, double), double x0, double y0, double h, size_t steps);
            static double solveODE_RungeKutta(double (*f)(double, double), double x0, double y0, double h, size_t steps);

            // تحسين
            static double findMinimum(double (*f)(double), double a, double b, double tolerance = 1e-6);
            static double findRoot(double (*f)(double), double a, double b, double tolerance = 1e-6);

            // تحويلات إحداثية
            static std::pair<double, double> cartesianToPolar(double x, double y);
            static std::pair<double, double> polarToCartesian(double r, double theta);
            static std::tuple<double, double, double> cartesianToSpherical(double x, double y, double z);
            static std::tuple<double, double, double> sphericalToCartesian(double r, double theta, double phi);

            // حسابات هندسية
            static double distance2D(double x1, double y1, double x2, double y2);
            static double distance3D(double x1, double y1, double z1, double x2, double y2, double z2);
            static double areaTriangle(double x1, double y1, double x2, double y2, double x3, double y3);
            static double areaPolygon(const std::vector<std::pair<double, double>>& points);

            // حسابات فيزيائية
            static double gravitationalForce(double m1, double m2, double r);
            static double kineticEnergy(double mass, double velocity);
            static double potentialEnergy(double mass, double height, double g = 9.81);
        };

        // ══════════════════════════════════════════════════════════════
        // 📊 مراقبة الأداء
        // ══════════════════════════════════════════════════════════════

        class PerformanceMonitor {
        private:
            std::unordered_map<std::string, std::vector<double>> operationTimes;

        public:
            void recordOperation(const std::string& operation, double timeMs);
            double getAverageTime(const std::string& operation) const;
            size_t getTotalOperations(const std::string& operation) const;
            std::vector<std::string> getPerformanceReport() const;
        };

    private:
        PerformanceMonitor perfMonitor;

        ArabicMath() = default;
        ~ArabicMath() = default;
    };

    // ══════════════════════════════════════════════════════════════
    // 🔧 دوال مساعدة للمكتبة القياسية
    // ══════════════════════════════════════════════════════════════

    namespace Math {

        // دوال رياضية أساسية
        double جذر_تربيعي(double رقم);
        double قيمة_مطلقة(double رقم);
        double أس(double قاعدة, double أسس);
        double جيب(double زاوية);
        double جيب_تمام(double زاوية);
        double ظل(double زاوية);

        // ثوابت
        constexpr double PI = 3.14159265358979323846;
        constexpr double E = 2.71828182845904523536;

        // دوال إحصائية
        template<typename T>
        double متوسط(const std::vector<T>& بيانات);

        template<typename T>
        double انحراف_معياري(const std::vector<T>& بيانات);

        // متجهات ومصفوفات
        ArabicMath::Vector<double> أنشئ_متجهاً(size_t حجم, double قيمة_افتراضية = 0.0);
        ArabicMath::Matrix<double> أنشئ_مصفوفة(size_t صفوف, size_t أعمدة, double قيمة_افتراضية = 0.0);

        // أرقام عشوائية
        double عشوائي();
        double عشوائي(double دقيق, double كبير);
        int عشوائي_صحيح(int دقيق, int كبير);

    } // namespace Math

} // namespace ArabicLanguage::StdLib
