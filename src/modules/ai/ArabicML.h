// ArabicML.h - مكتبة الذكاء الاصطناعي العربية
// الأسبوع الأول من الشهر السادس: مكتبة الرياضيات المتقدمة
#ifndef ARABIC_ML_H
#define ARABIC_ML_H

#include <vector>
#include <map>
#include <string>
#include <memory>
#include <functional>
#include <random>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace ArabicLanguage {

// ==================== الأساسيات الرياضية ====================

/**
 * @brief مصفوفة متعددة الأبعاد
 */
template<typename T>
class Tensor {
private:
    std::vector<T> data;
    std::vector<size_t> shape;
    std::vector<size_t> strides;

public:
    Tensor();
    Tensor(const std::vector<size_t>& shape);
    Tensor(const std::vector<size_t>& shape, const std::vector<T>& data);
    Tensor(const Tensor<T>& other);
    Tensor(Tensor<T>&& other) noexcept;

    ~Tensor() = default;

    Tensor<T>& operator=(const Tensor<T>& other);
    Tensor<T>& operator=(Tensor<T>&& other) noexcept;

    // الوصول إلى العناصر
    T& operator[](const std::vector<size_t>& indices);
    const T& operator[](const std::vector<size_t>& indices) const;

    // العمليات الأساسية
    Tensor<T> operator+(const Tensor<T>& other) const;
    Tensor<T> operator-(const Tensor<T>& other) const;
    Tensor<T> operator*(const Tensor<T>& other) const;
    Tensor<T> operator/(const Tensor<T>& other) const;

    Tensor<T> operator+(T scalar) const;
    Tensor<T> operator-(T scalar) const;
    Tensor<T> operator*(T scalar) const;
    Tensor<T> operator/(T scalar) const;

    // العمليات الرياضية
    Tensor<T> transpose() const;
    Tensor<T> dot(const Tensor<T>& other) const;
    Tensor<T> sum(int axis = -1) const;
    Tensor<T> mean(int axis = -1) const;
    Tensor<T> std(int axis = -1) const;

    // المعلومات
    const std::vector<size_t>& getShape() const { return shape; }
    size_t getSize() const;
    size_t getDims() const { return shape.size(); }
    bool isEmpty() const { return data.empty(); }

    // التحويلات
    std::vector<T>& getData() { return data; }
    const std::vector<T>& getData() const { return data; }

    // الطباعة
    void print() const;

private:
    size_t getFlatIndex(const std::vector<size_t>& indices) const;
    void calculateStrides();
};

/**
 * @brief مصفوفة ثنائية الأبعاد (اختصار)
 */
template<typename T>
class Matrix : public Tensor<T> {
public:
    Matrix();
    Matrix(size_t rows, size_t cols);
    Matrix(size_t rows, size_t cols, const std::vector<T>& data);
    Matrix(const Tensor<T>& tensor);

    size_t rows() const;
    size_t cols() const;

    T& operator()(size_t i, size_t j);
    const T& operator()(size_t i, size_t j) const;

    Matrix<T> inverse() const;
    T determinant() const;
    Matrix<T> eigenvalueDecomposition() const;
};

/**
 * @brief متجه (اختصار)
 */
template<typename T>
class Vector : public Tensor<T> {
public:
    Vector();
    Vector(size_t size);
    Vector(size_t size, const std::vector<T>& data);
    Vector(const Tensor<T>& tensor);

    size_t size() const;

    T& operator[](size_t index);
    const T& operator[](size_t index) const;

    T dot(const Vector<T>& other) const;
    T norm() const;
    Vector<T> normalize() const;
};

// ==================== الإحصاء والمعالجة ====================

/**
 * @brief أدوات إحصائية أساسية
 */
class Statistics {
public:
    template<typename T>
    static T mean(const std::vector<T>& data);

    template<typename T>
    static T variance(const std::vector<T>& data);

    template<typename T>
    static T stddev(const std::vector<T>& data);

    template<typename T>
    static T median(const std::vector<T>& data);

    template<typename T>
    static T mode(const std::vector<T>& data);

    template<typename T>
    static T min(const std::vector<T>& data);

    template<typename T>
    static T max(const std::vector<T>& data);

    template<typename T>
    static std::vector<T> quantile(const std::vector<T>& data, const std::vector<double>& quantiles);

    template<typename T>
    static std::map<T, int> histogram(const std::vector<T>& data, int bins = 10);

    // توزيعات إحصائية
    static double normal_pdf(double x, double mean = 0.0, double stddev = 1.0);
    static double normal_cdf(double x, double mean = 0.0, double stddev = 1.0);
    static double binomial_pdf(int k, int n, double p);
    static double poisson_pdf(int k, double lambda);
};

/**
 * @brief معالج البيانات
 */
class DataProcessor {
public:
    template<typename T>
    static void normalize(std::vector<T>& data, T min_val = 0, T max_val = 1);

    template<typename T>
    static void standardize(std::vector<T>& data);

    template<typename T>
    static void shuffle(std::vector<T>& data);

    template<typename T>
    static std::pair<std::vector<T>, std::vector<T>> train_test_split(
        const std::vector<T>& data, double test_size = 0.2);

    template<typename T>
    static std::vector<std::vector<T>> k_fold_split(
        const std::vector<T>& data, int k = 5);

    // معالجة البيانات المفقودة
    template<typename T>
    static void fill_missing(std::vector<T>& data, T value);

    template<typename T>
    static void interpolate_missing(std::vector<T>& data);
};

// ==================== خوارزميات التعلم الآلي ====================

/**
 * @brief واجهة خوارزمية التعلم الآلي
 */
class MLAlgorithm {
public:
    virtual ~MLAlgorithm() = default;

    virtual void fit(const Tensor<double>& X, const Tensor<double>& y) = 0;
    virtual Tensor<double> predict(const Tensor<double>& X) const = 0;
    virtual double score(const Tensor<double>& X, const Tensor<double>& y) const = 0;

    // معلومات النموذج
    virtual std::string getName() const = 0;
    virtual std::map<std::string, double> getParameters() const = 0;
};

/**
 * @brief الانحدار الخطي البسيط
 */
class LinearRegression : public MLAlgorithm {
private:
    Vector<double> weights;
    double bias;
    double learning_rate;
    int max_iterations;

public:
    LinearRegression(double lr = 0.01, int max_iter = 1000);

    void fit(const Tensor<double>& X, const Tensor<double>& y) override;
    Tensor<double> predict(const Tensor<double>& X) const override;
    double score(const Tensor<double>& X, const Tensor<double>& y) const override;

    std::string getName() const override { return "LinearRegression"; }
    std::map<std::string, double> getParameters() const override;

    const Vector<double>& getWeights() const { return weights; }
    double getBias() const { return bias; }
};

/**
 * @brief الانحدار اللوجستي
 */
class LogisticRegression : public MLAlgorithm {
private:
    Vector<double> weights;
    double bias;
    double learning_rate;
    int max_iterations;

public:
    LogisticRegression(double lr = 0.01, int max_iter = 1000);

    void fit(const Tensor<double>& X, const Tensor<double>& y) override;
    Tensor<double> predict(const Tensor<double>& X) const override;
    double score(const Tensor<double>& X, const Tensor<double>& y) const override;

    std::string getName() const override { return "LogisticRegression"; }
    std::map<std::string, double> getParameters() const override;

    const Vector<double>& getWeights() const { return weights; }
    double getBias() const { return bias; }
};

/**
 * @brief آلة المتجهات الداعمة البسيطة
 */
class SVM : public MLAlgorithm {
private:
    Vector<double> weights;
    double bias;
    double C;  // معلمة التنظيم
    double learning_rate;
    int max_iterations;

public:
    SVM(double C = 1.0, double lr = 0.01, int max_iter = 1000);

    void fit(const Tensor<double>& X, const Tensor<double>& y) override;
    Tensor<double> predict(const Tensor<double>& X) const override;
    double score(const Tensor<double>& X, const Tensor<double>& y) const override;

    std::string getName() const override { return "SVM"; }
    std::map<std::string, double> getParameters() const override;
};

/**
 * @brief K-أقرب الجيران
 */
class KNN : public MLAlgorithm {
private:
    Tensor<double> X_train;
    Tensor<double> y_train;
    int k;

public:
    KNN(int k = 3);

    void fit(const Tensor<double>& X, const Tensor<double>& y) override;
    Tensor<double> predict(const Tensor<double>& X) const override;
    double score(const Tensor<double>& X, const Tensor<double>& y) const override;

    std::string getName() const override { return "KNN"; }
    std::map<std::string, double> getParameters() const override;
};

/**
 * @brief تقييم النماذج
 */
class ModelEvaluator {
public:
    static double accuracy_score(const Tensor<double>& y_true, const Tensor<double>& y_pred);
    static double precision_score(const Tensor<double>& y_true, const Tensor<double>& y_pred);
    static double recall_score(const Tensor<double>& y_true, const Tensor<double>& y_pred);
    static double f1_score(const Tensor<double>& y_true, const Tensor<double>& y_pred);
    static double mse(const Tensor<double>& y_true, const Tensor<double>& y_pred);
    static double rmse(const Tensor<double>& y_true, const Tensor<double>& y_pred);
    static double mae(const Tensor<double>& y_true, const Tensor<double>& y_pred);
    static double r2_score(const Tensor<double>& y_true, const Tensor<double>& y_pred);

    // مصفوفة الالتباس
    static std::vector<std::vector<int>> confusion_matrix(const Tensor<double>& y_true, const Tensor<double>& y_pred);
};

// ==================== الأدوات المساعدة ====================

/**
 * @brief مولد الأرقام العشوائية
 */
class Random {
private:
    std::mt19937 generator;

public:
    Random(unsigned int seed = std::random_device{}());

    // توزيعات مختلفة
    double uniform(double min = 0.0, double max = 1.0);
    double normal(double mean = 0.0, double stddev = 1.0);
    int randint(int min, int max);
    bool choice(double probability = 0.5);

    // توليد بيانات عشوائية
    Tensor<double> random_tensor(const std::vector<size_t>& shape, double min = 0.0, double max = 1.0);
    Vector<double> random_vector(size_t size, double min = 0.0, double max = 1.0);
};

/**
 * @brief مساعدات متنوعة
 */
namespace Utils {
    template<typename T>
    void print_vector(const std::vector<T>& vec, const std::string& name = "");

    template<typename T>
    void print_matrix(const std::vector<std::vector<T>>& mat, const std::string& name = "");

    double sigmoid(double x);
    double relu(double x);
    double tanh_activation(double x);
    double softmax(const std::vector<double>& x, int index);

    // دوال الخسارة
    double binary_cross_entropy(double y_true, double y_pred);
    double mean_squared_error(const std::vector<double>& y_true, const std::vector<double>& y_pred);
}

// ==================== تنفيذ القوالب ====================

// تنفيذ Tensor
template<typename T>
Tensor<T>::Tensor() {}

template<typename T>
Tensor<T>::Tensor(const std::vector<size_t>& shape) : shape(shape) {
    calculateStrides();
    data.resize(getSize());
}

template<typename T>
Tensor<T>::Tensor(const std::vector<size_t>& shape, const std::vector<T>& init_data) : shape(shape) {
    calculateStrides();
    data = init_data;
    if (data.size() != getSize()) {
        throw std::runtime_error("Data size does not match tensor shape");
    }
}

template<typename T>
Tensor<T>::Tensor(const Tensor<T>& other) : shape(other.shape), strides(other.strides), data(other.data) {}

template<typename T>
Tensor<T>::Tensor(Tensor<T>&& other) noexcept : shape(std::move(other.shape)), strides(std::move(other.strides)), data(std::move(other.data)) {}

template<typename T>
Tensor<T>& Tensor<T>::operator=(const Tensor<T>& other) {
    if (this != &other) {
        shape = other.shape;
        strides = other.strides;
        data = other.data;
    }
    return *this;
}

template<typename T>
Tensor<T>& Tensor<T>::operator=(Tensor<T>&& other) noexcept {
    if (this != &other) {
        shape = std::move(other.shape);
        strides = std::move(other.strides);
        data = std::move(other.data);
    }
    return *this;
}

template<typename T>
T& Tensor<T>::operator[](const std::vector<size_t>& indices) {
    return data[getFlatIndex(indices)];
}

template<typename T>
const T& Tensor<T>::operator[](const std::vector<size_t>& indices) const {
    return data[getFlatIndex(indices)];
}

template<typename T>
size_t Tensor<T>::getSize() const {
    size_t size = 1;
    for (size_t dim : shape) {
        size *= dim;
    }
    return size;
}

template<typename T>
void Tensor<T>::print() const {
    std::cout << "Tensor shape: [";
    for (size_t i = 0; i < shape.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << shape[i];
    }
    std::cout << "]\n";

    // طباعة بسيطة للمصفوفات الصغيرة
    if (shape.size() == 1 && shape[0] <= 10) {
        std::cout << "[";
        for (size_t i = 0; i < shape[0]; ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << data[i];
        }
        std::cout << "]\n";
    } else if (shape.size() == 2 && shape[0] <= 5 && shape[1] <= 5) {
        for (size_t i = 0; i < shape[0]; ++i) {
            std::cout << "[";
            for (size_t j = 0; j < shape[1]; ++j) {
                if (j > 0) std::cout << ", ";
                std::cout << data[i * shape[1] + j];
            }
            std::cout << "]\n";
        }
    } else {
        std::cout << "Data size: " << data.size() << " elements\n";
    }
}

template<typename T>
size_t Tensor<T>::getFlatIndex(const std::vector<size_t>& indices) const {
    if (indices.size() != shape.size()) {
        throw std::runtime_error("Index dimensions do not match tensor dimensions");
    }

    size_t index = 0;
    for (size_t i = 0; i < indices.size(); ++i) {
        if (indices[i] >= shape[i]) {
            throw std::runtime_error("Index out of bounds");
        }
        index += indices[i] * strides[i];
    }
    return index;
}

template<typename T>
void Tensor<T>::calculateStrides() {
    strides.resize(shape.size());
    if (!strides.empty()) {
        strides.back() = 1;
        for (int i = static_cast<int>(shape.size()) - 2; i >= 0; --i) {
            strides[i] = strides[i + 1] * shape[i + 1];
        }
    }
}

// تنفيذ Matrix
template<typename T>
Matrix<T>::Matrix() : Tensor<T>({0, 0}) {}

template<typename T>
Matrix<T>::Matrix(size_t rows, size_t cols) : Tensor<T>({rows, cols}) {}

template<typename T>
Matrix<T>::Matrix(size_t rows, size_t cols, const std::vector<T>& data)
    : Tensor<T>({rows, cols}, data) {}

template<typename T>
Matrix<T>::Matrix(const Tensor<T>& tensor) : Tensor<T>(tensor) {
    if (tensor.getDims() != 2) {
        throw std::runtime_error("Tensor must be 2-dimensional for Matrix");
    }
}

template<typename T>
size_t Matrix<T>::rows() const {
    return this->getShape()[0];
}

template<typename T>
size_t Matrix<T>::cols() const {
    return this->getShape()[1];
}

template<typename T>
T& Matrix<T>::operator()(size_t i, size_t j) {
    return (*this)[{i, j}];
}

template<typename T>
const T& Matrix<T>::operator()(size_t i, size_t j) const {
    return (*this)[{i, j}];
}

// تنفيذ Vector
template<typename T>
Vector<T>::Vector() : Tensor<T>({0}) {}

template<typename T>
Vector<T>::Vector(size_t size) : Tensor<T>({size}) {}

template<typename T>
Vector<T>::Vector(size_t size, const std::vector<T>& data)
    : Tensor<T>({size}, data) {}

template<typename T>
Vector<T>::Vector(const Tensor<T>& tensor) : Tensor<T>(tensor) {
    if (tensor.getDims() != 1) {
        throw std::runtime_error("Tensor must be 1-dimensional for Vector");
    }
}

template<typename T>
size_t Vector<T>::size() const {
    return this->getShape()[0];
}

template<typename T>
T& Vector<T>::operator[](size_t index) {
    return (*this)[{index}];
}

template<typename T>
const T& Vector<T>::operator[](size_t index) const {
    return (*this)[{index}];
}

template<typename T>
T Vector<T>::dot(const Vector<T>& other) const {
    if (this->size() != other.size()) {
        throw std::runtime_error("Vector sizes must match for dot product");
    }

    T result = 0;
    for (size_t i = 0; i < this->size(); ++i) {
        result += (*this)[i] * other[i];
    }
    return result;
}

template<typename T>
T Vector<T>::norm() const {
    return std::sqrt(this->dot(*this));
}

template<typename T>
Vector<T> Vector<T>::normalize() const {
    T n = norm();
    if (n == 0) return *this;

    Vector<T> result(this->size());
    for (size_t i = 0; i < this->size(); ++i) {
        result[i] = (*this)[i] / n;
    }
    return result;
}

// تنفيذ Statistics
template<typename T>
T Statistics::mean(const std::vector<T>& data) {
    if (data.empty()) return 0;
    return std::accumulate(data.begin(), data.end(), T(0)) / data.size();
}

template<typename T>
T Statistics::variance(const std::vector<T>& data) {
    if (data.size() <= 1) return 0;

    T m = mean(data);
    T sum = 0;
    for (const T& val : data) {
        sum += (val - m) * (val - m);
    }
    return sum / (data.size() - 1);
}

template<typename T>
T Statistics::stddev(const std::vector<T>& data) {
    return std::sqrt(variance(data));
}

} // namespace ArabicLanguage

#endif // ARABIC_ML_H
