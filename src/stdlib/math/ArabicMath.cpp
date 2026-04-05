#define _USE_MATH_DEFINES
#include <cmath>
#include "ArabicMath.h"
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace ArabicLanguage::StdLib {

    ArabicMath& ArabicMath::getInstance() {
        static ArabicMath instance;
        return instance;
    }

    // ══════════════════════════════════════════════════════════════
    // 📊 تنفيذ دوال الرياضيات الأساسية
    // ══════════════════════════════════════════════════════════════

    double ArabicMath::BasicMath::erf(double x) {
        // تقريب دالة الخطأ باستخدام سلسلة تايلور
        double sum = 0.0;
        double term = x;
        double x_squared = x * x;
        int n = 1;

        for (int i = 0; i < 20; ++i) {
            sum += term / (n * std::sqrt(M_PI));
            term *= -x_squared / (i + 1);
            n += 2;
        }

        return (2.0 / std::sqrt(M_PI)) * sum;
    }

    double ArabicMath::BasicMath::erfc(double x) {
        return 1.0 - erf(x);
    }

    double ArabicMath::BasicMath::gamma(double x) {
        // تقريب دالة غاما باستخدام خوارزمية Lanczos
        if (x < 0.5) {
            return M_PI / (std::sin(M_PI * x) * gamma(1.0 - x));
        }

        x -= 1.0;

        double g = 0.99999999999980993;
        const double p[] = {
            676.5203681218851, -1259.1392167224028,
            771.32342877765313, -176.61502916214059,
            12.507343278686905, -0.13857109526572012,
            9.9843695780195716e-6, 1.5056327351493116e-7
        };

        double sum = p[0];
        for (int i = 1; i < 8; ++i) {
            sum += p[i] / (x + i);
        }

        double t = x + 7.5;
        return std::sqrt(2.0 * M_PI) * std::pow(t, x + 0.5) * std::exp(-t) * sum;
    }

    double ArabicMath::BasicMath::lgamma(double x) {
        return std::log(std::abs(gamma(x)));
    }

    // ══════════════════════════════════════════════════════════════
    // 📐 تنفيذ المتجهات
    // ══════════════════════════════════════════════════════════════

template<typename T>
ArabicMath::Vector<T>::Vector(size_t size, T defaultValue) : data(size, defaultValue) {
    // استخدام إدارة الذاكرة القياسية
}

template<typename T>
ArabicMath::Vector<T>::Vector(const std::vector<T>& values) : data(values) {
    // استخدام إدارة الذاكرة القياسية
}

template<typename T>
ArabicMath::Vector<T>::Vector(std::vector<T>&& values) noexcept : data(std::move(values)) {
    // استخدام إدارة الذاكرة القياسية
}

template<typename T>
ArabicMath::Vector<T>::Vector(const Vector<T>& other) : data(other.data) {
    // استخدام إدارة الذاكرة القياسية
}

template<typename T>
ArabicMath::Vector<T>::Vector(Vector<T>&& other) noexcept : data(std::move(other.data)) {
    // استخدام إدارة الذاكرة القياسية
}

    template<typename T>
    void ArabicMath::Vector<T>::resize(size_t newSize, T defaultValue) {
        data.resize(newSize, defaultValue);
    }

    template<typename T>
    void ArabicMath::Vector<T>::reserve(size_t capacity) {
        data.reserve(capacity);
    }

    template<typename T>
    T ArabicMath::Vector<T>::dot(const Vector<T>& other) const {
        if (size() != other.size()) {
            throw std::runtime_error("Vector sizes must be equal for dot product");
        }

        T result = T{};
        for (size_t i = 0; i < size(); ++i) {
            result += data[i] * other.data[i];
        }
        return result;
    }

    template<typename T>
    double ArabicMath::Vector<T>::magnitude() const {
        double sum = 0.0;
        for (const auto& val : data) {
            sum += static_cast<double>(val * val);
        }
        return std::sqrt(sum);
    }

    template<typename T>
    ArabicMath::Vector<T> ArabicMath::Vector<T>::normalize() const {
        double mag = magnitude();
        if (mag == 0.0) return Vector<T>(size(), T{});

        Vector<T> result = *this;
        for (auto& val : result.data) {
            val = static_cast<T>(static_cast<double>(val) / mag);
        }
        return result;
    }

    template<typename T>
    ArabicMath::Vector<T> ArabicMath::Vector<T>::cross(const Vector<T>& other) const {
        if (size() != 3 || other.size() != 3) {
            throw std::runtime_error("Cross product requires 3D vectors");
        }

        Vector<T> result(3);
        result[0] = data[1] * other[2] - data[2] * other[1];
        result[1] = data[2] * other[0] - data[0] * other[2];
        result[2] = data[0] * other[1] - data[1] * other[0];
        return result;
    }

    template<typename T>
    ArabicMath::Vector<T> ArabicMath::Vector<T>::operator+(const Vector<T>& other) const {
        if (size() != other.size()) {
            throw std::runtime_error("Vector sizes must be equal for addition");
        }

        Vector<T> result(size());
        for (size_t i = 0; i < size(); ++i) {
            result[i] = data[i] + other.data[i];
        }
        return result;
    }

    template<typename T>
    ArabicMath::Vector<T> ArabicMath::Vector<T>::operator-(const Vector<T>& other) const {
        if (size() != other.size()) {
            throw std::runtime_error("Vector sizes must be equal for subtraction");
        }

        Vector<T> result(size());
        for (size_t i = 0; i < size(); ++i) {
            result[i] = data[i] - other.data[i];
        }
        return result;
    }

    template<typename T>
    ArabicMath::Vector<T> ArabicMath::Vector<T>::operator*(T scalar) const {
        Vector<T> result(size());
        for (size_t i = 0; i < size(); ++i) {
            result[i] = data[i] * scalar;
        }
        return result;
    }

    template<typename T>
    ArabicMath::Vector<T> ArabicMath::Vector<T>::operator/(T scalar) const {
        if (scalar == T{}) {
            throw std::runtime_error("Division by zero");
        }

        Vector<T> result(size());
        for (size_t i = 0; i < size(); ++i) {
            result[i] = data[i] / scalar;
        }
        return result;
    }

    template<typename T>
    ArabicMath::Vector<T>& ArabicMath::Vector<T>::operator+=(const Vector<T>& other) {
        *this = *this + other;
        return *this;
    }

    template<typename T>
    ArabicMath::Vector<T>& ArabicMath::Vector<T>::operator-=(const Vector<T>& other) {
        *this = *this - other;
        return *this;
    }

    template<typename T>
    ArabicMath::Vector<T>& ArabicMath::Vector<T>::operator*=(T scalar) {
        *this = *this * scalar;
        return *this;
    }

    template<typename T>
    ArabicMath::Vector<T>& ArabicMath::Vector<T>::operator/=(T scalar) {
        *this = *this / scalar;
        return *this;
    }

    template<typename T>
    double ArabicMath::Vector<T>::distance(const Vector<T>& other) const {
        return (*this - other).magnitude();
    }

    template<typename T>
    double ArabicMath::Vector<T>::angle(const Vector<T>& other) const {
        double dotProduct = static_cast<double>(dot(other));
        double mag1 = magnitude();
        double mag2 = other.magnitude();

        if (mag1 == 0.0 || mag2 == 0.0) return 0.0;

        return std::acos(std::clamp(dotProduct / (mag1 * mag2), -1.0, 1.0));
    }

    template<typename T>
    ArabicMath::Vector<T> ArabicMath::Vector<T>::project(const Vector<T>& onto) const {
        double dotProduct = static_cast<double>(dot(onto));
        double ontoMagSquared = onto.magnitude();
        ontoMagSquared *= ontoMagSquared;

        if (ontoMagSquared == 0.0) return Vector<T>(size(), T{});

        return onto * static_cast<T>(dotProduct / ontoMagSquared);
    }

    template<typename T>
    ArabicMath::Vector<T> ArabicMath::Vector<T>::reflect(const Vector<T>& normal) const {
        Vector<T> normalizedNormal = normal.normalize();
        double dotProduct = static_cast<double>(dot(normalizedNormal));
        return *this - normalizedNormal * static_cast<T>(2.0 * dotProduct);
    }

    template<typename T>
    std::string ArabicMath::Vector<T>::toString() const {
        std::stringstream ss;
        ss << "[";
        for (size_t i = 0; i < size(); ++i) {
            if (i > 0) ss << ", ";
            ss << data[i];
        }
        ss << "]";
        return ss.str();
    }

    // ══════════════════════════════════════════════════════════════
    // 🔢 تنفيذ المصفوفات
    // ══════════════════════════════════════════════════════════════

template<typename T>
ArabicMath::Matrix<T>::Matrix(size_t rows, size_t cols, T defaultValue)
    : rows(rows), cols(cols), data(rows, std::vector<T>(cols, defaultValue)) {
    // استخدام إدارة الذاكرة القياسية
}

template<typename T>
ArabicMath::Matrix<T>::Matrix(const std::vector<std::vector<T>>& values)
    : data(values), rows(values.size()), cols(values.empty() ? 0 : values[0].size()) {
    // استخدام إدارة الذاكرة القياسية
}

template<typename T>
ArabicMath::Matrix<T>::Matrix(const Matrix<T>& other)
    : data(other.data), rows(other.rows), cols(other.cols) {
    // استخدام إدارة الذاكرة القياسية
}

template<typename T>
ArabicMath::Matrix<T>::Matrix(Matrix<T>&& other) noexcept
    : data(std::move(other.data)), rows(other.rows), cols(other.cols) {
    // استخدام إدارة الذاكرة القياسية
}

    template<typename T>
    ArabicMath::Matrix<T> ArabicMath::Matrix<T>::transpose() const {
        Matrix<T> result(cols, rows);
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                result(j, i) = data[i][j];
            }
        }
        return result;
    }

    template<typename T>
    T ArabicMath::Matrix<T>::determinant() const {
        if (!isSquare()) {
            throw std::runtime_error("Determinant requires square matrix");
        }

        if (rows == 1) return data[0][0];
        if (rows == 2) return data[0][0] * data[1][1] - data[0][1] * data[1][0];

        // تنفيذ بسيط للمصفوفات 2x2 و 3x3 (يمكن توسيعه)
        if (rows == 2) {
            return data[0][0] * data[1][1] - data[0][1] * data[1][0];
        }

        // للمصفوفات الأكبر، إرجاع 0 (تنفيذ مبسط)
        return T{0};
    }

    template<typename T>
    ArabicMath::Matrix<T> ArabicMath::Matrix<T>::operator+(const Matrix<T>& other) const {
        if (rows != other.rows || cols != other.cols) {
            throw std::runtime_error("Matrix dimensions must match for addition");
        }

        Matrix<T> result(rows, cols);
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                result(i, j) = data[i][j] + other.data[i][j];
            }
        }
        return result;
    }

    template<typename T>
    ArabicMath::Matrix<T> ArabicMath::Matrix<T>::operator-(const Matrix<T>& other) const {
        if (rows != other.rows || cols != other.cols) {
            throw std::runtime_error("Matrix dimensions must match for subtraction");
        }

        Matrix<T> result(rows, cols);
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                result(i, j) = data[i][j] - other.data[i][j];
            }
        }
        return result;
    }

    template<typename T>
    ArabicMath::Matrix<T> ArabicMath::Matrix<T>::operator*(const Matrix<T>& other) const {
        if (cols != other.rows) {
            throw std::runtime_error("Matrix multiplication: columns of first must equal rows of second");
        }

        Matrix<T> result(rows, other.cols, T{});
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < other.cols; ++j) {
                for (size_t k = 0; k < cols; ++k) {
                    result(i, j) += data[i][k] * other.data[k][j];
                }
            }
        }
        return result;
    }

    template<typename T>
    ArabicMath::Matrix<T> ArabicMath::Matrix<T>::operator*(T scalar) const {
        Matrix<T> result(rows, cols);
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                result(i, j) = data[i][j] * scalar;
            }
        }
        return result;
    }

    template<typename T>
    ArabicMath::Vector<T> ArabicMath::Matrix<T>::operator*(const Vector<T>& vec) const {
        if (cols != vec.size()) {
            throw std::runtime_error("Matrix-vector multiplication: columns must equal vector size");
        }

        Vector<T> result(rows, T{});
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                result[i] += data[i][j] * vec[j];
            }
        }
        return result;
    }

    template<typename T>
    ArabicMath::Matrix<T> ArabicMath::Matrix<T>::identity(size_t size) {
        Matrix<T> result(size, size, T{});
        for (size_t i = 0; i < size; ++i) {
            result(i, i) = T{1};
        }
        return result;
    }

    template<typename T>
    ArabicMath::Matrix<T> ArabicMath::Matrix<T>::zeros(size_t rows, size_t cols) {
        return Matrix<T>(rows, cols, T{});
    }

    template<typename T>
    ArabicMath::Matrix<T> ArabicMath::Matrix<T>::ones(size_t rows, size_t cols) {
        return Matrix<T>(rows, cols, T{1});
    }

    template<typename T>
    std::string ArabicMath::Matrix<T>::toString() const {
        std::stringstream ss;
        ss << "[\n";
        for (size_t i = 0; i < rows; ++i) {
            ss << "  [";
            for (size_t j = 0; j < cols; ++j) {
                if (j > 0) ss << ", ";
                ss << data[i][j];
            }
            ss << "]";
            if (i < rows - 1) ss << ",";
            ss << "\n";
        }
        ss << "]";
        return ss.str();
    }

// دوال مساعدة للمصفوفات (مبسطة للإصدار الأولي)

    // ══════════════════════════════════════════════════════════════
    // 🔄 تنفيذ الأعداد المركبة
    // ══════════════════════════════════════════════════════════════

    ArabicMath::Complex ArabicMath::Complex::operator+(const Complex& other) const {
        return Complex(real + other.real, imag + other.imag);
    }

    ArabicMath::Complex ArabicMath::Complex::operator-(const Complex& other) const {
        return Complex(real - other.real, imag - other.imag);
    }

    ArabicMath::Complex ArabicMath::Complex::operator*(const Complex& other) const {
        return Complex(real * other.real - imag * other.imag,
                      real * other.imag + imag * other.real);
    }

    ArabicMath::Complex ArabicMath::Complex::operator/(const Complex& other) const {
        double denom = other.real * other.real + other.imag * other.imag;
        return Complex((real * other.real + imag * other.imag) / denom,
                      (imag * other.real - real * other.imag) / denom);
    }

    ArabicMath::Complex ArabicMath::Complex::operator*(double scalar) const {
        return Complex(real * scalar, imag * scalar);
    }

    ArabicMath::Complex ArabicMath::Complex::operator/(double scalar) const {
        return Complex(real / scalar, imag / scalar);
    }

    ArabicMath::Complex ArabicMath::Complex::sqrt() const {
        double mag = magnitude();
        double phi = phase();
        return fromPolar(std::sqrt(mag), phi / 2.0);
    }

    ArabicMath::Complex ArabicMath::Complex::exp() const {
        double e_real = std::exp(real);
        return Complex(e_real * std::cos(imag), e_real * std::sin(imag));
    }

    ArabicMath::Complex ArabicMath::Complex::log() const {
        return Complex(std::log(magnitude()), phase());
    }

    ArabicMath::Complex ArabicMath::Complex::pow(const Complex& exponent) const {
        return (log() * exponent).exp();
    }

    ArabicMath::Complex ArabicMath::Complex::sin() const {
        return Complex(std::sin(real) * std::cosh(imag),
                      std::cos(real) * std::sinh(imag));
    }

    ArabicMath::Complex ArabicMath::Complex::cos() const {
        return Complex(std::cos(real) * std::cosh(imag),
                      -std::sin(real) * std::sinh(imag));
    }

    std::string ArabicMath::Complex::toString() const {
        std::stringstream ss;
        ss << real;
        if (imag >= 0) ss << "+";
        ss << imag << "i";
        return ss.str();
    }

    ArabicMath::Complex ArabicMath::Complex::fromPolar(double magnitude, double phase) {
        return Complex(magnitude * std::cos(phase), magnitude * std::sin(phase));
    }

    // ══════════════════════════════════════════════════════════════
    // 🎲 تنفيذ مولدات الأرقام العشوائية
    // ══════════════════════════════════════════════════════════════

    ArabicMath::Random::Random(unsigned int seed) : generator(seed), uniformDist(0.0, 1.0) {}

    double ArabicMath::Random::nextDouble() {
        return uniformDist(generator);
    }

    double ArabicMath::Random::nextDouble(double min, double max) {
        return min + (max - min) * nextDouble();
    }

    int ArabicMath::Random::nextInt(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(generator);
    }

    bool ArabicMath::Random::nextBool() {
        return nextDouble() < 0.5;
    }

    double ArabicMath::Random::nextGaussian(double mean, double stddev) {
        std::normal_distribution<double> dist(mean, stddev);
        return dist(generator);
    }

    double ArabicMath::Random::nextExponential(double lambda) {
        std::exponential_distribution<double> dist(lambda);
        return dist(generator);
    }

    int ArabicMath::Random::nextPoisson(double lambda) {
        std::poisson_distribution<int> dist(lambda);
        return dist(generator);
    }

    template<typename T>
    void ArabicMath::Random::shuffle(std::vector<T>& container) {
        std::shuffle(container.begin(), container.end(), generator);
    }

    template<typename T>
    T ArabicMath::Random::choose(const std::vector<T>& container) {
        if (container.empty()) throw std::runtime_error("Container is empty");
        return container[nextInt(0, container.size() - 1)];
    }

    void ArabicMath::Random::setSeed(unsigned int seed) {
        generator.seed(seed);
    }

    // ══════════════════════════════════════════════════════════════
    // 🔬 تنفيذ الحسابات المتقدمة
    // ══════════════════════════════════════════════════════════════

    double ArabicMath::AdvancedMath::numericalDerivative(double (*f)(double), double x, double h) {
        return (f(x + h) - f(x - h)) / (2.0 * h);
    }

    double ArabicMath::AdvancedMath::numericalIntegral(double (*f)(double), double a, double b, size_t steps) {
        double h = (b - a) / steps;
        double sum = 0.5 * (f(a) + f(b));

        for (size_t i = 1; i < steps; ++i) {
            sum += f(a + i * h);
        }

        return sum * h;
    }

    double ArabicMath::AdvancedMath::solveODE_Euler(double (*f)(double, double), double x0, double y0, double h, size_t steps) {
        double x = x0;
        double y = y0;

        for (size_t i = 0; i < steps; ++i) {
            y += h * f(x, y);
            x += h;
        }

        return y;
    }

    double ArabicMath::AdvancedMath::solveODE_RungeKutta(double (*f)(double, double), double x0, double y0, double h, size_t steps) {
        double x = x0;
        double y = y0;

        for (size_t i = 0; i < steps; ++i) {
            double k1 = h * f(x, y);
            double k2 = h * f(x + h/2, y + k1/2);
            double k3 = h * f(x + h/2, y + k2/2);
            double k4 = h * f(x + h, y + k3);

            y += (k1 + 2*k2 + 2*k3 + k4) / 6.0;
            x += h;
        }

        return y;
    }

    double ArabicMath::AdvancedMath::findMinimum(double (*f)(double), double a, double b, double tolerance) {
        // خوارزمية البحث الثنائي للنقطة الحرجة
        double x1 = a, x2 = b;
        while (std::abs(x2 - x1) > tolerance) {
            double mid = (x1 + x2) / 2;
            double f_mid = numericalDerivative(f, mid);

            if (f_mid > 0) {
                x2 = mid;
            } else {
                x1 = mid;
            }
        }
        return (x1 + x2) / 2;
    }

    double ArabicMath::AdvancedMath::findRoot(double (*f)(double), double a, double b, double tolerance) {
        // طريقة البحث الثنائي
        double fa = f(a), fb = f(b);
        if (fa * fb >= 0) throw std::runtime_error("Function must have opposite signs at endpoints");

        double c = a;
        while ((b - a) / 2 > tolerance) {
            c = (a + b) / 2;
            double fc = f(c);

            if (fc == 0.0) break;
            else if (fa * fc < 0) b = c;
            else a = c;
        }

        return c;
    }

    std::pair<double, double> ArabicMath::AdvancedMath::cartesianToPolar(double x, double y) {
        double r = std::sqrt(x*x + y*y);
        double theta = std::atan2(y, x);
        return {r, theta};
    }

    std::pair<double, double> ArabicMath::AdvancedMath::polarToCartesian(double r, double theta) {
        double x = r * std::cos(theta);
        double y = r * std::sin(theta);
        return {x, y};
    }

    std::tuple<double, double, double> ArabicMath::AdvancedMath::cartesianToSpherical(double x, double y, double z) {
        double r = std::sqrt(x*x + y*y + z*z);
        double theta = std::atan2(y, x);
        double phi = std::acos(z / r);
        return {r, theta, phi};
    }

    std::tuple<double, double, double> ArabicMath::AdvancedMath::sphericalToCartesian(double r, double theta, double phi) {
        double x = r * std::sin(phi) * std::cos(theta);
        double y = r * std::sin(phi) * std::sin(theta);
        double z = r * std::cos(phi);
        return {x, y, z};
    }

    double ArabicMath::AdvancedMath::distance2D(double x1, double y1, double x2, double y2) {
        return std::sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
    }

    double ArabicMath::AdvancedMath::distance3D(double x1, double y1, double z1, double x2, double y2, double z2) {
        return std::sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) + (z2-z1)*(z2-z1));
    }

    double ArabicMath::AdvancedMath::areaTriangle(double x1, double y1, double x2, double y2, double x3, double y3) {
        return std::abs((x1*(y2 - y3) + x2*(y3 - y1) + x3*(y1 - y2)) / 2.0);
    }

    double ArabicMath::AdvancedMath::gravitationalForce(double m1, double m2, double r) {
        const double G = 6.67430e-11;
        return G * m1 * m2 / (r * r);
    }

    double ArabicMath::AdvancedMath::kineticEnergy(double mass, double velocity) {
        return 0.5 * mass * velocity * velocity;
    }

    double ArabicMath::AdvancedMath::potentialEnergy(double mass, double height, double g) {
        return mass * g * height;
    }

    // ══════════════════════════════════════════════════════════════
    // 📈 تنفيذ مراقب الأداء
    // ══════════════════════════════════════════════════════════════

    void ArabicMath::PerformanceMonitor::recordOperation(const std::string& operation, double timeMs) {
        operationTimes[operation].push_back(timeMs);
    }

    double ArabicMath::PerformanceMonitor::getAverageTime(const std::string& operation) const {
        auto it = operationTimes.find(operation);
        if (it == operationTimes.end() || it->second.empty()) return 0.0;

        double sum = 0.0;
        for (double time : it->second) sum += time;
        return sum / it->second.size();
    }

    size_t ArabicMath::PerformanceMonitor::getTotalOperations(const std::string& operation) const {
        auto it = operationTimes.find(operation);
        return it != operationTimes.end() ? it->second.size() : 0;
    }

    std::vector<std::string> ArabicMath::PerformanceMonitor::getPerformanceReport() const {
        std::vector<std::string> report;
        report.push_back("=== تقرير أداء الرياضيات ===");

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

    namespace Math {

        double جذر_تربيعي(double رقم) {
            return ArabicMath::BasicMath::sqrt(رقم);
        }

        double قيمة_مطلقة(double رقم) {
            return ArabicMath::BasicMath::abs(رقم);
        }

        double أس(double قاعدة, double أسس) {
            return ArabicMath::BasicMath::pow(قاعدة, أسس);
        }

        double جيب(double زاوية) {
            return ArabicMath::BasicMath::sin(زاوية);
        }

        double جيب_تمام(double زاوية) {
            return ArabicMath::BasicMath::cos(زاوية);
        }

        double ظل(double زاوية) {
            return ArabicMath::BasicMath::tan(زاوية);
        }

        template<typename T>
        double متوسط(const std::vector<T>& بيانات) {
            return ArabicMath::Statistics::mean(بيانات);
        }

        template<typename T>
        double انحراف_معياري(const std::vector<T>& بيانات) {
            return ArabicMath::Statistics::standardDeviation(بيانات);
        }

        ArabicMath::Vector<double> أنشئ_متجهاً(size_t حجم, double قيمة_افتراضية) {
            return ArabicMath::Vector<double>(حجم, قيمة_افتراضية);
        }

        ArabicMath::Matrix<double> أنشئ_مصفوفة(size_t صفوف, size_t أعمدة, double قيمة_افتراضية) {
            return ArabicMath::Matrix<double>(صفوف, أعمدة, قيمة_افتراضية);
        }

        double عشوائي() {
            static ArabicMath::Random rand;
            return rand.nextDouble();
        }

        double عشوائي(double دقيق, double كبير) {
            static ArabicMath::Random rand;
            return rand.nextDouble(دقيق, كبير);
        }

        int عشوائي_صحيح(int دقيق, int كبير) {
            static ArabicMath::Random rand;
            return rand.nextInt(دقيق, كبير);
        }

    } // namespace Math

    // ══════════════════════════════════════════════════════════════
    // 📋 تنفيذات القوالب المحددة المطلوبة
    // ══════════════════════════════════════════════════════════════

    // تنفيذات القوالب المطلوبة للاستخدام الشائع
    template class ArabicMath::Vector<double>;
    template class ArabicMath::Vector<int>;
    template class ArabicMath::Vector<float>;

    template class ArabicMath::Matrix<double>;
    template class ArabicMath::Matrix<int>;
    template class ArabicMath::Matrix<float>;

    template double ArabicMath::Statistics::mean(const std::vector<double>&);
    template double ArabicMath::Statistics::median(std::vector<double>);
    template double ArabicMath::Statistics::variance(const std::vector<double>&);
    template double ArabicMath::Statistics::standardDeviation(const std::vector<double>&);
    template double ArabicMath::Statistics::min(const std::vector<double>&);
    template double ArabicMath::Statistics::max(const std::vector<double>&);

    template void ArabicMath::Random::shuffle(std::vector<int>&);
    template void ArabicMath::Random::shuffle(std::vector<double>&);
    template int ArabicMath::Random::choose(const std::vector<int>&);
    template double ArabicMath::Random::choose(const std::vector<double>&);

    template double Math::متوسط(const std::vector<double>&);
    template double Math::انحراف_معياري(const std::vector<double>&);

} // namespace ArabicLanguage::StdLib
