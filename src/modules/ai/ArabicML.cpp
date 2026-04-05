// ArabicML.cpp - تطبيق مكتبة الذكاء الاصطناعي العربية
// الأسبوع الأول من الشهر السادس: مكتبة الرياضيات المتقدمة
#include "ArabicML.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#define _USE_MATH_DEFINES
#include <cmath>
#include <random>

namespace ArabicLanguage {

// ==================== تطبيق Tensor ====================

template<typename T>
Tensor<T> Tensor<T>::operator+(const Tensor<T>& other) const {
    if (shape != other.shape) {
        throw std::runtime_error("Tensor shapes must match for addition");
    }

    Tensor<T> result(shape);
    for (size_t i = 0; i < data.size(); ++i) {
        result.data[i] = data[i] + other.data[i];
    }
    return result;
}

template<typename T>
Tensor<T> Tensor<T>::operator-(const Tensor<T>& other) const {
    if (shape != other.shape) {
        throw std::runtime_error("Tensor shapes must match for subtraction");
    }

    Tensor<T> result(shape);
    for (size_t i = 0; i < data.size(); ++i) {
        result.data[i] = data[i] - other.data[i];
    }
    return result;
}

template<typename T>
Tensor<T> Tensor<T>::operator*(const Tensor<T>& other) const {
    if (shape != other.shape) {
        throw std::runtime_error("Tensor shapes must match for element-wise multiplication");
    }

    Tensor<T> result(shape);
    for (size_t i = 0; i < data.size(); ++i) {
        result.data[i] = data[i] * other.data[i];
    }
    return result;
}

template<typename T>
Tensor<T> Tensor<T>::operator/(const Tensor<T>& other) const {
    if (shape != other.shape) {
        throw std::runtime_error("Tensor shapes must match for element-wise division");
    }

    Tensor<T> result(shape);
    for (size_t i = 0; i < data.size(); ++i) {
        if (other.data[i] == 0) {
            throw std::runtime_error("Division by zero");
        }
        result.data[i] = data[i] / other.data[i];
    }
    return result;
}

template<typename T>
Tensor<T> Tensor<T>::operator+(T scalar) const {
    Tensor<T> result(shape);
    for (size_t i = 0; i < data.size(); ++i) {
        result.data[i] = data[i] + scalar;
    }
    return result;
}

template<typename T>
Tensor<T> Tensor<T>::operator-(T scalar) const {
    Tensor<T> result(shape);
    for (size_t i = 0; i < data.size(); ++i) {
        result.data[i] = data[i] - scalar;
    }
    return result;
}

template<typename T>
Tensor<T> Tensor<T>::operator*(T scalar) const {
    Tensor<T> result(shape);
    for (size_t i = 0; i < data.size(); ++i) {
        result.data[i] = data[i] * scalar;
    }
    return result;
}

template<typename T>
Tensor<T> Tensor<T>::operator/(T scalar) const {
    if (scalar == 0) {
        throw std::runtime_error("Division by zero");
    }

    Tensor<T> result(shape);
    for (size_t i = 0; i < data.size(); ++i) {
        result.data[i] = data[i] / scalar;
    }
    return result;
}

template<typename T>
Tensor<T> Tensor<T>::transpose() const {
    if (shape.size() != 2) {
        throw std::runtime_error("Transpose only supported for 2D tensors");
    }

    std::vector<size_t> new_shape = {shape[1], shape[0]};
    Tensor<T> result(new_shape);

    for (size_t i = 0; i < shape[0]; ++i) {
        for (size_t j = 0; j < shape[1]; ++j) {
            result[{j, i}] = (*this)[{i, j}];
        }
    }

    return result;
}

template<typename T>
Tensor<T> Tensor<T>::dot(const Tensor<T>& other) const {
    // Handle Vector-Matrix or Matrix-Vector by treating them as 2D
    if (shape.size() == 1 && other.shape.size() == 2) {
        // Vector * Matrix [1, n] * [n, m] = [1, m]
        if (shape[0] != other.shape[0]) {
            throw std::runtime_error("Inner dimensions must match for dot product (Vector-Matrix)");
        }
        Tensor<T> result({other.shape[1]});
        for (size_t j = 0; j < other.shape[1]; ++j) {
            T sum = 0;
            for (size_t i = 0; i < shape[0]; ++i) {
                sum += data[i] * other[{i, j}];
            }
            result.data[j] = sum;
        }
        return result;
    }

    if (shape.size() == 2 && other.shape.size() == 1) {
        // Matrix * Vector [n, m] * [m, 1] = [n, 1]
        if (shape[1] != other.shape[0]) {
            throw std::runtime_error("Inner dimensions must match for dot product (Matrix-Vector)");
        }
        Tensor<T> result({shape[0]});
        for (size_t i = 0; i < shape[0]; ++i) {
            T sum = 0;
            for (size_t j = 0; j < shape[1]; ++j) {
                sum += (*this)[{i, j}] * other.data[j];
            }
            result.data[i] = sum;
        }
        return result;
    }

    if (shape.size() == 1 && other.shape.size() == 1) {
        // Dot product of two vectors
        if (shape[0] != other.shape[0]) {
            throw std::runtime_error("Vector sizes must match for dot product");
        }
        T sum = 0;
        for (size_t i = 0; i < shape[0]; ++i) {
            sum += data[i] * other.data[i];
        }
        return Tensor<T>({1}, {sum});
    }

    if (shape.size() != 2 || other.shape.size() != 2) {
        throw std::runtime_error("Dot product only supported for 1D or 2D tensors");
    }

    if (shape[1] != other.shape[0]) {
        throw std::runtime_error("Inner dimensions must match for dot product (Matrix-Matrix)");
    }

    std::vector<size_t> new_shape = {shape[0], other.shape[1]};
    Tensor<T> result(new_shape);

    for (size_t i = 0; i < shape[0]; ++i) {
        for (size_t j = 0; j < other.shape[1]; ++j) {
            T sum = 0;
            for (size_t k = 0; k < shape[1]; ++k) {
                sum += (*this)[{i, k}] * other[{k, j}];
            }
            result[{i, j}] = sum;
        }
    }

    return result;
}

template<typename T>
Tensor<T> Tensor<T>::sum(int axis) const {
    if (axis == -1) {
        // Sum all elements
        T total = 0;
        for (const T& val : data) {
            total += val;
        }
        return Tensor<T>({1}, {total});
    }

    if (axis < 0 || static_cast<size_t>(axis) >= shape.size()) {
        throw std::runtime_error("Invalid axis for sum operation");
    }

    std::vector<size_t> new_shape = shape;
    new_shape.erase(new_shape.begin() + axis);

    if (new_shape.empty()) {
        new_shape = {1};
    }

    Tensor<T> result(new_shape);

    // Simplified implementation for 2D tensors
    if (shape.size() == 2) {
        if (axis == 0) {
            // Sum along rows
            for (size_t j = 0; j < shape[1]; ++j) {
                T col_sum = 0;
                for (size_t i = 0; i < shape[0]; ++i) {
                    col_sum += (*this)[{i, j}];
                }
                result.data[j] = col_sum;
            }
        } else if (axis == 1) {
            // Sum along columns
            for (size_t i = 0; i < shape[0]; ++i) {
                T row_sum = 0;
                for (size_t j = 0; j < shape[1]; ++j) {
                    row_sum += (*this)[{i, j}];
                }
                result.data[i] = row_sum;
            }
        }
    }

    return result;
}

template<typename T>
Tensor<T> Tensor<T>::mean(int axis) const {
    if (axis == -1) {
        Tensor<T> sum_tensor = sum(axis);
        return sum_tensor / static_cast<T>(data.size());
    }

    Tensor<T> sum_tensor = sum(axis);
    size_t divisor = (axis == 0) ? shape[0] : shape[1];
    return sum_tensor / static_cast<T>(divisor);
}

template<typename T>
Tensor<T> Tensor<T>::std(int axis) const {
    if (axis != -1) {
        throw std::runtime_error("Std along axis not implemented yet");
    }

    Tensor<T> mean_tensor = mean(axis);
    T m = mean_tensor.data[0];

    T variance = 0;
    for (const T& val : data) {
        T diff = val - m;
        variance += diff * diff;
    }
    variance /= (data.size() - 1);

    return Tensor<T>({1}, {static_cast<T>(std::sqrt(static_cast<double>(variance)))});
}

// ==================== تطبيق Matrix ====================

template<typename T>
Matrix<T> Matrix<T>::inverse() const {
    if (rows() != cols()) {
        throw std::runtime_error("Matrix must be square for inverse");
    }

    size_t n = rows();
    Matrix<T> result(n, n);

    // Simple Gaussian elimination for 2x2 and 3x3 matrices
    if (n == 2) {
        T det = determinant();
        if (det == 0) {
            throw std::runtime_error("Matrix is singular");
        }

        T a = (*this)(0, 0), b = (*this)(0, 1);
        T c = (*this)(1, 0), d = (*this)(1, 1);

        result(0, 0) = d / det;
        result(0, 1) = -b / det;
        result(1, 0) = -c / det;
        result(1, 1) = a / det;
    } else {
        throw std::runtime_error("Inverse for matrices larger than 2x2 not implemented yet");
    }

    return result;
}

template<typename T>
T Matrix<T>::determinant() const {
    if (rows() != cols()) {
        throw std::runtime_error("Matrix must be square for determinant");
    }

    size_t n = rows();

    if (n == 2) {
        return (*this)(0, 0) * (*this)(1, 1) - (*this)(0, 1) * (*this)(1, 0);
    } else if (n == 3) {
        return (*this)(0, 0) * ((*this)(1, 1) * (*this)(2, 2) - (*this)(1, 2) * (*this)(2, 1)) -
               (*this)(0, 1) * ((*this)(1, 0) * (*this)(2, 2) - (*this)(1, 2) * (*this)(2, 0)) +
               (*this)(0, 2) * ((*this)(1, 0) * (*this)(2, 1) - (*this)(1, 1) * (*this)(2, 0));
    } else {
        throw std::runtime_error("Determinant for matrices larger than 3x3 not implemented yet");
    }
}

template<typename T>
Matrix<T> Matrix<T>::eigenvalueDecomposition() const {
    throw std::runtime_error("Eigenvalue decomposition not implemented yet");
    return *this;
}

// ==================== تطبيق Statistics ====================

template<typename T>
T Statistics::median(const std::vector<T>& data) {
    if (data.empty()) return 0;

    std::vector<T> sorted = data;
    std::sort(sorted.begin(), sorted.end());

    size_t n = sorted.size();
    if (n % 2 == 0) {
        return (sorted[n/2 - 1] + sorted[n/2]) / 2;
    } else {
        return sorted[n/2];
    }
}

template<typename T>
T Statistics::mode(const std::vector<T>& data) {
    if (data.empty()) return 0;

    std::map<T, int> counts;
    for (const T& val : data) {
        counts[val]++;
    }

    T mode = data[0];
    int max_count = 0;

    for (const auto& pair : counts) {
        if (pair.second > max_count) {
            max_count = pair.second;
            mode = pair.first;
        }
    }

    return mode;
}

template<typename T>
T Statistics::min(const std::vector<T>& data) {
    if (data.empty()) return 0;
    return *std::min_element(data.begin(), data.end());
}

template<typename T>
T Statistics::max(const std::vector<T>& data) {
    if (data.empty()) return 0;
    return *std::max_element(data.begin(), data.end());
}

template<typename T>
std::vector<T> Statistics::quantile(const std::vector<T>& data, const std::vector<double>& quantiles) {
    if (data.empty()) return {};

    std::vector<T> sorted = data;
    std::sort(sorted.begin(), sorted.end());

    std::vector<T> result;
    for (double q : quantiles) {
        if (q < 0.0 || q > 1.0) continue;

        double index = q * (sorted.size() - 1);
        size_t lower = static_cast<size_t>(std::floor(index));
        size_t upper = static_cast<size_t>(std::ceil(index));

        if (lower == upper) {
            result.push_back(sorted[lower]);
        } else {
            double fraction = index - lower;
            result.push_back(sorted[lower] * (1 - fraction) + sorted[upper] * fraction);
        }
    }

    return result;
}

template<typename T>
std::map<T, int> Statistics::histogram(const std::vector<T>& data, int bins) {
    if (data.empty() || bins <= 0) return {};

    T min_val = min(data);
    T max_val = max(data);
    T range = max_val - min_val;

    if (range == 0) {
        return {{min_val, static_cast<int>(data.size())}};
    }

    T bin_width = range / bins;
    std::map<T, int> hist;

    for (const T& val : data) {
        T bin_key = min_val + std::floor((val - min_val) / bin_width) * bin_width;
        hist[bin_key]++;
    }

    return hist;
}

double Statistics::normal_pdf(double x, double mean, double stddev) {
    const double PI = 3.14159265358979323846;
    double variance = stddev * stddev;
    double exponent = -0.5 * std::pow((x - mean), 2) / variance;
    return (1.0 / std::sqrt(2 * PI * variance)) * std::exp(exponent);
}

double Statistics::normal_cdf(double x, double mean, double stddev) {
    // Simplified CDF using approximation
    double z = (x - mean) / stddev;
    return 0.5 * (1 + std::erf(z / std::sqrt(2)));
}

double Statistics::binomial_pdf(int k, int n, double p) {
    if (k < 0 || k > n || p < 0 || p > 1) return 0.0;

    double binomial_coeff = 1.0;
    for (int i = 1; i <= k; ++i) {
        binomial_coeff *= (n - k + i) / static_cast<double>(i);
    }

    return binomial_coeff * std::pow(p, k) * std::pow(1 - p, n - k);
}

double Statistics::poisson_pdf(int k, double lambda) {
    if (k < 0 || lambda <= 0) return 0.0;

    return std::exp(-lambda) * std::pow(lambda, k) / std::tgamma(k + 1);
}

// ==================== تطبيق DataProcessor ====================

template<typename T>
void DataProcessor::normalize(std::vector<T>& data, T min_val, T max_val) {
    if (data.empty()) return;

    T current_min = Statistics::min(data);
    T current_max = Statistics::max(data);

    if (current_max == current_min) {
        std::fill(data.begin(), data.end(), min_val);
        return;
    }

    T range = max_val - min_val;
    T current_range = current_max - current_min;

    for (T& val : data) {
        val = min_val + (val - current_min) * range / current_range;
    }
}

template<typename T>
void DataProcessor::standardize(std::vector<T>& data) {
    if (data.size() <= 1) return;

    T mean_val = Statistics::mean(data);
    T std_val = Statistics::stddev(data);

    if (std_val == 0) {
        std::fill(data.begin(), data.end(), T(0));
        return;
    }

    for (T& val : data) {
        val = (val - mean_val) / std_val;
    }
}

template<typename T>
void DataProcessor::shuffle(std::vector<T>& data) {
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(data.begin(), data.end(), g);
}

template<typename T>
std::pair<std::vector<T>, std::vector<T>> DataProcessor::train_test_split(
    const std::vector<T>& data, double test_size) {

    if (test_size <= 0.0 || test_size >= 1.0) {
        throw std::runtime_error("test_size must be between 0 and 1");
    }

    size_t test_count = static_cast<size_t>(data.size() * test_size);
    size_t train_count = data.size() - test_count;

    std::vector<T> shuffled_data = data;
    shuffle(shuffled_data);

    std::vector<T> train_data(shuffled_data.begin(), shuffled_data.begin() + train_count);
    std::vector<T> test_data(shuffled_data.begin() + train_count, shuffled_data.end());

    return {train_data, test_data};
}

template<typename T>
std::vector<std::vector<T>> DataProcessor::k_fold_split(const std::vector<T>& data, int k) {
    if (k <= 1) {
        throw std::runtime_error("k must be greater than 1");
    }

    std::vector<T> shuffled_data = data;
    shuffle(shuffled_data);

    size_t fold_size = data.size() / k;
    std::vector<std::vector<T>> folds;

    for (int i = 0; i < k; ++i) {
        size_t start = i * fold_size;
        size_t end = (i == k - 1) ? data.size() : (i + 1) * fold_size;

        std::vector<T> fold(shuffled_data.begin() + start, shuffled_data.begin() + end);
        folds.push_back(fold);
    }

    return folds;
}

template<typename T>
void DataProcessor::fill_missing(std::vector<T>& data, T value) {
    for (T& val : data) {
        if (std::isnan(static_cast<double>(val))) {
            val = value;
        }
    }
}

template<typename T>
void DataProcessor::interpolate_missing(std::vector<T>& data) {
    // Simple linear interpolation for missing values
    for (size_t i = 0; i < data.size(); ++i) {
        if (std::isnan(static_cast<double>(data[i]))) {
            // Find previous non-NaN value
            size_t prev_idx = i;
            while (prev_idx > 0 && std::isnan(static_cast<double>(data[--prev_idx])));

            // Find next non-NaN value
            size_t next_idx = i;
            while (next_idx < data.size() - 1 && std::isnan(static_cast<double>(data[++next_idx])));

            if (prev_idx != i && next_idx != i) {
                // Linear interpolation
                double ratio = static_cast<double>(i - prev_idx) / (next_idx - prev_idx);
                data[i] = data[prev_idx] + (data[next_idx] - data[prev_idx]) * ratio;
            } else if (prev_idx != i) {
                data[i] = data[prev_idx];
            } else if (next_idx != i) {
                data[i] = data[next_idx];
            }
        }
    }
}

// ==================== تطبيق خوارزميات التعلم الآلي ====================

LinearRegression::LinearRegression(double lr, int max_iter)
    : learning_rate(lr), max_iterations(max_iter), bias(0.0) {}

void LinearRegression::fit(const Tensor<double>& X, const Tensor<double>& y) {
    if (X.getDims() != 2 || y.getDims() != 1) {
        throw std::runtime_error("X must be 2D and y must be 1D for LinearRegression");
    }

    size_t n_samples = X.getShape()[0];
    size_t n_features = X.getShape()[1];

    if (y.getShape()[0] != n_samples) {
        throw std::runtime_error("X and y must have the same number of samples");
    }

    // Initialize weights
    weights = Vector<double>(n_features);
    for (size_t i = 0; i < n_features; ++i) {
        weights[i] = 0.0;
    }

    // Gradient descent
    for (int iter = 0; iter < max_iterations; ++iter) {
        // Compute predictions manually (matrix-vector multiplication)
        Vector<double> predictions(n_samples);
        for (size_t i = 0; i < n_samples; ++i) {
            double pred = bias;
            for (size_t j = 0; j < n_features; ++j) {
                pred += weights[j] * X[{i, j}];
            }
            predictions[i] = pred;
        }

        // Compute errors
        Vector<double> errors(n_samples);
        for (size_t i = 0; i < n_samples; ++i) {
            errors[i] = predictions[i] - y.getData()[i];
        }

        // Compute gradients
        Vector<double> weight_gradients(n_features);
        for (size_t j = 0; j < n_features; ++j) {
            double grad = 0.0;
            for (size_t i = 0; i < n_samples; ++i) {
                grad += X[{i, j}] * errors[i];
            }
            weight_gradients[j] = grad / n_samples;
        }
        
        double bias_gradient = 0.0;
        for (size_t i = 0; i < n_samples; ++i) {
            bias_gradient += errors[i];
        }
        bias_gradient /= n_samples;

        // Update weights and bias
        for (size_t i = 0; i < n_features; ++i) {
            weights[i] -= learning_rate * weight_gradients[i];
        }
        bias -= learning_rate * bias_gradient;
    }
}

Tensor<double> LinearRegression::predict(const Tensor<double>& X) const {
    if (X.getDims() != 2) {
        throw std::runtime_error("X must be 2D for prediction");
    }

    size_t n_samples = X.getShape()[0];
    size_t n_features = X.getShape()[1];
    
    Tensor<double> predictions({n_samples, 1});
    for (size_t i = 0; i < n_samples; ++i) {
        double pred = bias;
        for (size_t j = 0; j < n_features; ++j) {
            pred += weights[j] * X[{i, j}];
        }
        predictions[{i, 0}] = pred;
    }
    
    return predictions;
}

double LinearRegression::score(const Tensor<double>& X, const Tensor<double>& y) const {
    Tensor<double> predictions = predict(X);
    return ModelEvaluator::r2_score(y, predictions);
}

std::map<std::string, double> LinearRegression::getParameters() const {
    return {
        {"learning_rate", learning_rate},
        {"max_iterations", static_cast<double>(max_iterations)},
        {"bias", bias}
    };
}

LogisticRegression::LogisticRegression(double lr, int max_iter)
    : learning_rate(lr), max_iterations(max_iter), bias(0.0) {}

void LogisticRegression::fit(const Tensor<double>& X, const Tensor<double>& y) {
    if (X.getDims() != 2 || y.getDims() != 1) {
        throw std::runtime_error("X must be 2D and y must be 1D for LogisticRegression");
    }

    size_t n_samples = X.getShape()[0];
    size_t n_features = X.getShape()[1];

    if (y.getShape()[0] != n_samples) {
        throw std::runtime_error("X and y must have the same number of samples");
    }

    // Initialize weights
    weights = Vector<double>(n_features);
    for (size_t i = 0; i < n_features; ++i) {
        weights[i] = 0.0;
    }

    // Gradient descent with sigmoid
    for (int iter = 0; iter < max_iterations; ++iter) {
        // Compute predictions manually
        Vector<double> linear_pred(n_samples);
        Vector<double> predictions(n_samples);
        
        for (size_t i = 0; i < n_samples; ++i) {
            double pred = bias;
            for (size_t j = 0; j < n_features; ++j) {
                pred += weights[j] * X[{i, j}];
            }
            linear_pred[i] = pred;
            predictions[i] = Utils::sigmoid(pred);
        }

        // Compute errors
        Vector<double> errors(n_samples);
        for (size_t i = 0; i < n_samples; ++i) {
            errors[i] = predictions[i] - y.getData()[i];
        }

        // Compute gradients
        Vector<double> weight_gradients(n_features);
        for (size_t j = 0; j < n_features; ++j) {
            double grad = 0.0;
            for (size_t i = 0; i < n_samples; ++i) {
                grad += X[{i, j}] * errors[i];
            }
            weight_gradients[j] = grad / n_samples;
        }
        
        double bias_gradient = 0.0;
        for (size_t i = 0; i < n_samples; ++i) {
            bias_gradient += errors[i];
        }
        bias_gradient /= n_samples;

        // Update weights and bias
        for (size_t i = 0; i < n_features; ++i) {
            weights[i] -= learning_rate * weight_gradients[i];
        }
        bias -= learning_rate * bias_gradient;
    }
}

Tensor<double> LogisticRegression::predict(const Tensor<double>& X) const {
    if (X.getDims() != 2) {
        throw std::runtime_error("X must be 2D for prediction");
    }

    size_t n_samples = X.getShape()[0];
    size_t n_features = X.getShape()[1];
    
    Tensor<double> predictions({n_samples, 1});
    
    for (size_t i = 0; i < n_samples; ++i) {
        double linear_pred = bias;
        for (size_t j = 0; j < n_features; ++j) {
            linear_pred += weights[j] * X[{i, j}];
        }
        
        double probability = Utils::sigmoid(linear_pred);
        predictions[{i, 0}] = (probability >= 0.5) ? 1.0 : 0.0;
    }

    return predictions;
}

double LogisticRegression::score(const Tensor<double>& X, const Tensor<double>& y) const {
    Tensor<double> predictions = predict(X);
    return ModelEvaluator::accuracy_score(y, predictions);
}

std::map<std::string, double> LogisticRegression::getParameters() const {
    return {
        {"learning_rate", learning_rate},
        {"max_iterations", static_cast<double>(max_iterations)},
        {"bias", bias}
    };
}

KNN::KNN(int k_val) : k(k_val) {}

void KNN::fit(const Tensor<double>& X, const Tensor<double>& y) {
    if (X.getDims() != 2 || y.getDims() != 1) {
        throw std::runtime_error("X must be 2D and y must be 1D for KNN");
    }

    if (X.getShape()[0] != y.getShape()[0]) {
        throw std::runtime_error("X and y must have the same number of samples");
    }

    X_train = X;
    y_train = y;
}

Tensor<double> KNN::predict(const Tensor<double>& X) const {
    if (X.getDims() != 2) {
        throw std::runtime_error("X must be 2D for prediction");
    }

    if (X_train.getSize() == 0) {
        throw std::runtime_error("Model must be fitted before prediction");
    }

    size_t n_samples = X.getShape()[0];
    size_t n_features = X.getShape()[1];

    if (n_features != X_train.getShape()[1]) {
        throw std::runtime_error("X must have the same number of features as training data");
    }

    Tensor<double> predictions({n_samples, 1});

    for (size_t i = 0; i < n_samples; ++i) {
        // Calculate distances to all training samples
        std::vector<std::pair<double, size_t>> distances;

        for (size_t j = 0; j < X_train.getShape()[0]; ++j) {
            double distance = 0.0;
            for (size_t f = 0; f < n_features; ++f) {
                double diff = X[{i, f}] - X_train[{j, f}];
                distance += diff * diff;
            }
            distance = std::sqrt(distance);
            distances.push_back({distance, j});
        }

        // Sort by distance and get k nearest neighbors
        std::sort(distances.begin(), distances.end());
        std::map<double, int> vote_count;

        for (int neighbor = 0; neighbor < k && neighbor < static_cast<int>(distances.size()); ++neighbor) {
            size_t train_idx = distances[neighbor].second;
            double label = y_train.getData()[train_idx];
            vote_count[label]++;
        }

        // Find the most common label
        double best_label = 0.0;
        int max_votes = 0;
        for (const auto& vote : vote_count) {
            if (vote.second > max_votes) {
                max_votes = vote.second;
                best_label = vote.first;
            }
        }

        predictions[{i, 0}] = best_label;
    }

    return predictions;
}

double KNN::score(const Tensor<double>& X, const Tensor<double>& y) const {
    Tensor<double> predictions = predict(X);
    return ModelEvaluator::accuracy_score(y, predictions);
}

std::map<std::string, double> KNN::getParameters() const {
    return {{"k", static_cast<double>(k)}};
}

// ==================== تطبيق SVM ====================

SVM::SVM(double C_val, double lr, int max_iter)
    : C(C_val), learning_rate(lr), max_iterations(max_iter), bias(0.0) {}

void SVM::fit(const Tensor<double>& X, const Tensor<double>& y) {
    if (X.getDims() != 2 || y.getDims() != 1) {
        throw std::runtime_error("X must be 2D and y must be 1D for SVM");
    }

    size_t n_samples = X.getShape()[0];
    size_t n_features = X.getShape()[1];

    if (y.getShape()[0] != n_samples) {
        throw std::runtime_error("X and y must have the same number of samples");
    }

    // Convert labels to -1 and 1 for SVM
    Tensor<double> y_binary(y.getShape());
    for (size_t i = 0; i < n_samples; ++i) {
        y_binary.getData()[i] = (y.getData()[i] > 0.5) ? 1.0 : -1.0;
    }

    // Initialize weights
    weights = Vector<double>(n_features);
    for (size_t i = 0; i < n_features; ++i) {
        weights[i] = 0.0;
    }

    // Simplified SVM training using gradient descent with hinge loss
    for (int iter = 0; iter < max_iterations; ++iter) {
        for (size_t i = 0; i < n_samples; ++i) {
            // Compute prediction
            double prediction = 0.0;
            for (size_t j = 0; j < n_features; ++j) {
                prediction += weights[j] * X[{i, j}];
            }
            prediction += bias;

            // Hinge loss: max(0, 1 - y * prediction)
            double y_i = y_binary.getData()[i];
            double margin = y_i * prediction;

            if (margin < 1.0) {
                // Update weights: gradient of hinge loss
                for (size_t j = 0; j < n_features; ++j) {
                    weights[j] += learning_rate * (y_i * X[{i, j}] - C * weights[j]);
                }
                bias += learning_rate * y_i;
            } else {
                // Only regularization term
                for (size_t j = 0; j < n_features; ++j) {
                    weights[j] -= learning_rate * C * weights[j];
                }
            }
        }
    }
}

Tensor<double> SVM::predict(const Tensor<double>& X) const {
    if (X.getDims() != 2) {
        throw std::runtime_error("X must be 2D for prediction");
    }

    size_t n_samples = X.getShape()[0];
    size_t n_features = X.getShape()[1];

    if (n_features != weights.size()) {
        throw std::runtime_error("X must have the same number of features as training data");
    }

    Tensor<double> predictions({n_samples, 1});

    for (size_t i = 0; i < n_samples; ++i) {
        double prediction = 0.0;
        for (size_t j = 0; j < n_features; ++j) {
            prediction += weights[j] * X[{i, j}];
        }
        prediction += bias;

        // Convert to binary prediction: sign of prediction
        predictions[{i, 0}] = (prediction >= 0.0) ? 1.0 : 0.0;
    }

    return predictions;
}

double SVM::score(const Tensor<double>& X, const Tensor<double>& y) const {
    Tensor<double> predictions = predict(X);
    return ModelEvaluator::accuracy_score(y, predictions);
}

std::map<std::string, double> SVM::getParameters() const {
    return {
        {"C", C},
        {"learning_rate", learning_rate},
        {"max_iterations", static_cast<double>(max_iterations)},
        {"bias", bias}
    };
}

// ==================== تطبيق ModelEvaluator ====================

double ModelEvaluator::accuracy_score(const Tensor<double>& y_true, const Tensor<double>& y_pred) {
    if (y_true.getSize() != y_pred.getSize()) {
        throw std::runtime_error("y_true and y_pred must have the same size");
    }

    size_t correct = 0;
    for (size_t i = 0; i < y_true.getSize(); ++i) {
        if (std::abs(y_true.getData()[i] - y_pred.getData()[i]) < 1e-6) {
            correct++;
        }
    }

    return static_cast<double>(correct) / y_true.getSize();
}

double ModelEvaluator::precision_score(const Tensor<double>& y_true, const Tensor<double>& y_pred) {
    auto cm = confusion_matrix(y_true, y_pred);

    if (cm.size() < 2 || cm[0].size() < 2) return 0.0;

    double tp = cm[1][1];
    double fp = cm[0][1];

    return (tp + fp > 0) ? tp / (tp + fp) : 0.0;
}

double ModelEvaluator::recall_score(const Tensor<double>& y_true, const Tensor<double>& y_pred) {
    auto cm = confusion_matrix(y_true, y_pred);

    if (cm.size() < 2 || cm[0].size() < 2) return 0.0;

    double tp = cm[1][1];
    double fn = cm[1][0];

    return (tp + fn > 0) ? tp / (tp + fn) : 0.0;
}

double ModelEvaluator::f1_score(const Tensor<double>& y_true, const Tensor<double>& y_pred) {
    double precision = precision_score(y_true, y_pred);
    double recall = recall_score(y_true, y_pred);

    return (precision + recall > 0) ? 2 * precision * recall / (precision + recall) : 0.0;
}

double ModelEvaluator::mse(const Tensor<double>& y_true, const Tensor<double>& y_pred) {
    if (y_true.getSize() != y_pred.getSize()) {
        throw std::runtime_error("y_true and y_pred must have the same size");
    }

    double sum_squared_errors = 0.0;
    for (size_t i = 0; i < y_true.getSize(); ++i) {
        double error = y_true.getData()[i] - y_pred.getData()[i];
        sum_squared_errors += error * error;
    }

    return sum_squared_errors / y_true.getSize();
}

double ModelEvaluator::rmse(const Tensor<double>& y_true, const Tensor<double>& y_pred) {
    return std::sqrt(mse(y_true, y_pred));
}

double ModelEvaluator::mae(const Tensor<double>& y_true, const Tensor<double>& y_pred) {
    if (y_true.getSize() != y_pred.getSize()) {
        throw std::runtime_error("y_true and y_pred must have the same size");
    }

    double sum_abs_errors = 0.0;
    for (size_t i = 0; i < y_true.getSize(); ++i) {
        sum_abs_errors += std::abs(y_true.getData()[i] - y_pred.getData()[i]);
    }

    return sum_abs_errors / y_true.getSize();
}

double ModelEvaluator::r2_score(const Tensor<double>& y_true, const Tensor<double>& y_pred) {
    if (y_true.getSize() != y_pred.getSize()) {
        throw std::runtime_error("y_true and y_pred must have the same size");
    }

    double mean_y = 0.0;
    for (size_t i = 0; i < y_true.getSize(); ++i) {
        mean_y += y_true.getData()[i];
    }
    mean_y /= y_true.getSize();

    double ss_tot = 0.0;
    double ss_res = 0.0;

    for (size_t i = 0; i < y_true.getSize(); ++i) {
        double y_i = y_true.getData()[i];
        double pred_i = y_pred.getData()[i];

        ss_tot += (y_i - mean_y) * (y_i - mean_y);
        ss_res += (y_i - pred_i) * (y_i - pred_i);
    }

    return (ss_tot > 0) ? 1.0 - (ss_res / ss_tot) : 0.0;
}

std::vector<std::vector<int>> ModelEvaluator::confusion_matrix(const Tensor<double>& y_true, const Tensor<double>& y_pred) {
    // Simple binary confusion matrix
    int tp = 0, tn = 0, fp = 0, fn = 0;

    for (size_t i = 0; i < y_true.getSize(); ++i) {
        bool actual = (y_true.getData()[i] > 0.5);
        bool predicted = (y_pred.getData()[i] > 0.5);

        if (actual && predicted) tp++;
        else if (!actual && !predicted) tn++;
        else if (!actual && predicted) fp++;
        else if (actual && !predicted) fn++;
    }

    return {{tn, fp}, {fn, tp}};
}

// ==================== تطبيق Random ====================

Random::Random(unsigned int seed) : generator(seed) {}

double Random::uniform(double min, double max) {
    std::uniform_real_distribution<double> dist(min, max);
    return dist(generator);
}

double Random::normal(double mean, double stddev) {
    std::normal_distribution<double> dist(mean, stddev);
    return dist(generator);
}

int Random::randint(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(generator);
}

bool Random::choice(double probability) {
    std::bernoulli_distribution dist(probability);
    return dist(generator);
}

Tensor<double> Random::random_tensor(const std::vector<size_t>& shape, double min, double max) {
    Tensor<double> tensor(shape);
    for (size_t i = 0; i < tensor.getSize(); ++i) {
        tensor.getData()[i] = uniform(min, max);
    }
    return tensor;
}

Vector<double> Random::random_vector(size_t size, double min, double max) {
    Vector<double> vec(size);
    for (size_t i = 0; i < size; ++i) {
        vec[i] = uniform(min, max);
    }
    return vec;
}

// ==================== تطبيق Utils ====================

template<typename T>
void Utils::print_vector(const std::vector<T>& vec, const std::string& name) {
    if (!name.empty()) {
        std::cout << name << ": ";
    }
    std::cout << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << vec[i];
    }
    std::cout << "]" << std::endl;
}

template<typename T>
void Utils::print_matrix(const std::vector<std::vector<T>>& mat, const std::string& name) {
    if (!name.empty()) {
        std::cout << name << ":" << std::endl;
    }

    for (const auto& row : mat) {
        std::cout << "[";
        for (size_t i = 0; i < row.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << row[i];
        }
        std::cout << "]" << std::endl;
    }
}

double Utils::sigmoid(double x) {
    return 1.0 / (1.0 + std::exp(-x));
}

double Utils::relu(double x) {
    return std::max(0.0, x);
}

double Utils::tanh_activation(double x) {
    return std::tanh(x);
}

double Utils::softmax(const std::vector<double>& x, int index) {
    double max_val = *std::max_element(x.begin(), x.end());
    double sum = 0.0;

    for (double val : x) {
        sum += std::exp(val - max_val);
    }

    return std::exp(x[index] - max_val) / sum;
}

double Utils::binary_cross_entropy(double y_true, double y_pred) {
    y_pred = std::max(1e-15, std::min(1.0 - 1e-15, y_pred)); // Clip to avoid log(0)
    return -(y_true * std::log(y_pred) + (1 - y_true) * std::log(1 - y_pred));
}

double Utils::mean_squared_error(const std::vector<double>& y_true, const std::vector<double>& y_pred) {
    if (y_true.size() != y_pred.size()) {
        throw std::runtime_error("Vectors must have the same size");
    }

    double sum = 0.0;
    for (size_t i = 0; i < y_true.size(); ++i) {
        double error = y_true[i] - y_pred[i];
        sum += error * error;
    }

    return sum / y_true.size();
}

// ==================== إجبار التطبيق ====================

// Force template instantiation for common types
template class Tensor<double>;
template class Tensor<int>;
template class Tensor<float>;

template class Matrix<double>;
template class Matrix<int>;
template class Matrix<float>;

template class Vector<double>;
template class Vector<int>;
template class Vector<float>;

// Statistics templates
template double Statistics::mean(const std::vector<double>&);
template double Statistics::variance(const std::vector<double>&);
template double Statistics::stddev(const std::vector<double>&);
template double Statistics::median(const std::vector<double>&);
template double Statistics::mode(const std::vector<double>&);
template double Statistics::min(const std::vector<double>&);
template double Statistics::max(const std::vector<double>&);
template std::vector<double> Statistics::quantile(const std::vector<double>&, const std::vector<double>&);
template std::map<double, int> Statistics::histogram(const std::vector<double>&, int);

// DataProcessor templates
template void DataProcessor::normalize(std::vector<double>&, double, double);
template void DataProcessor::standardize(std::vector<double>&);
template void DataProcessor::shuffle(std::vector<double>&);
template std::pair<std::vector<double>, std::vector<double>> DataProcessor::train_test_split(const std::vector<double>&, double);
template std::vector<std::vector<double>> DataProcessor::k_fold_split(const std::vector<double>&, int);
template void DataProcessor::fill_missing(std::vector<double>&, double);
template void DataProcessor::interpolate_missing(std::vector<double>&);

// Utils templates
template void Utils::print_vector(const std::vector<double>&, const std::string&);
template void Utils::print_matrix(const std::vector<std::vector<double>>&, const std::string&);

} // namespace ArabicLanguage
