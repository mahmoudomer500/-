// ArabicDeepLearning.cpp - تطبيق مكتبة التعلم العميق العربية
#include "ArabicDeepLearning.h"
#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm>

namespace ArabicLanguage {

// ==================== تطبيق Activation ====================

Tensor<double> Activation::apply(const Tensor<double>& input, ActivationType type) {
    Tensor<double> result = input;
    auto& data = result.getData();

    switch (type) {
        case ActivationType::SIGMOID:
            for (auto& val : data) val = 1.0 / (1.0 + std::exp(-val));
            break;
        case ActivationType::RELU:
            for (auto& val : data) val = std::max(0.0, val);
            break;
        case ActivationType::TANH:
            for (auto& val : data) val = std::tanh(val);
            break;
        case ActivationType::SOFTMAX: {
            double max_val = *std::max_element(data.begin(), data.end());
            double sum = 0;
            for (auto& val : data) {
                val = std::exp(val - max_val);
                sum += val;
            }
            for (auto& val : data) val /= sum;
            break;
        }
        case ActivationType::LINEAR:
        default:
            break;
    }
    return result;
}

Tensor<double> Activation::derivative(const Tensor<double>& input, ActivationType type) {
    Tensor<double> result(input.getShape());
    auto& data = result.getData();
    const auto& in_data = input.getData();

    switch (type) {
        case ActivationType::SIGMOID: {
            for (size_t i = 0; i < data.size(); ++i) {
                double s = 1.0 / (1.0 + std::exp(-in_data[i]));
                data[i] = s * (1.0 - s);
            }
            break;
        }
        case ActivationType::RELU:
            for (size_t i = 0; i < data.size(); ++i) data[i] = (in_data[i] > 0) ? 1.0 : 0.0;
            break;
        case ActivationType::TANH: {
            for (size_t i = 0; i < data.size(); ++i) {
                double t = std::tanh(in_data[i]);
                data[i] = 1.0 - t * t;
            }
            break;
        }
        case ActivationType::SOFTMAX:
            // Softmax derivative is complex and usually combined with Cross Entropy
            // For simplicity in this basic version, we return 1 (assuming it's used at the end)
            for (size_t i = 0; i < data.size(); ++i) data[i] = 1.0;
            break;
        case ActivationType::LINEAR:
        default:
            for (size_t i = 0; i < data.size(); ++i) data[i] = 1.0;
            break;
    }
    return result;
}

// ==================== تطبيق DenseLayer ====================

DenseLayer::DenseLayer(size_t input_size, size_t output_size, ActivationType activation)
    : Layer("Dense"), weights(output_size, input_size), bias(output_size), activation(activation) {
    
    Random rnd;
    // Xavier/Glorot initialization
    double limit = std::sqrt(6.0 / (input_size + output_size));
    
    auto& w_data = weights.getData();
    for (auto& w : w_data) w = rnd.uniform(-limit, limit);
    
    auto& b_data = bias.getData();
    for (auto& b : b_data) b = 0.0;

    grad_weights = Matrix<double>(output_size, input_size);
    grad_bias = Vector<double>(output_size);
}

Tensor<double> DenseLayer::forward(const Tensor<double>& input) {
    last_input = input;
    
    // Z = W * X + b
    // input is [input_size], weights is [output_size, input_size]
    // Matrix-Vector dot product: [output_size, input_size] * [input_size] = [output_size]
    Tensor<double> z = weights.dot(input) + bias;
    last_output = z;
    
    return Activation::apply(z, activation);
}

Tensor<double> DenseLayer::backward(const Tensor<double>& grad_output) {
    // grad_output is dL/dA [output_size]
    Tensor<double> da_dz = Activation::derivative(last_output, activation);
    
    // dL/dZ = dL/dA * dA/dZ (element-wise) [output_size]
    Tensor<double> dz(grad_output.getShape());
    for (size_t i = 0; i < dz.getSize(); ++i) {
        dz.getData()[i] = grad_output.getData()[i] * da_dz.getData()[i];
    }

    // dL/dW = dL/dZ * X^T
    // grad_weights += dz * last_input^T (outer product)
    const auto& dz_data = dz.getData();
    const auto& in_data = last_input.getData();
    auto& gw_data = grad_weights.getData();
    
    for (size_t i = 0; i < weights.rows(); ++i) {
        for (size_t j = 0; j < weights.cols(); ++j) {
            gw_data[i * weights.cols() + j] += dz_data[i] * in_data[j];
        }
    }

    // dL/db = dL/dZ
    auto& gb_data = grad_bias.getData();
    for (size_t i = 0; i < bias.size(); ++i) {
        gb_data[i] += dz_data[i];
    }

    // dL/dX = W^T * dL/dZ
    // W^T is [input_size, output_size], dz is [output_size]
    // Matrix-Vector dot product: [input_size, output_size] * [output_size] = [input_size]
    return weights.transpose().dot(dz);
}

void DenseLayer::update(double learning_rate) {
    auto& w_data = weights.getData();
    const auto& gw_data = grad_weights.getData();
    for (size_t i = 0; i < w_data.size(); ++i) {
        w_data[i] -= learning_rate * gw_data[i];
    }

    auto& b_data = bias.getData();
    const auto& gb_data = grad_bias.getData();
    for (size_t i = 0; i < b_data.size(); ++i) {
        b_data[i] -= learning_rate * gb_data[i];
    }

    // Reset gradients
    std::fill(grad_weights.getData().begin(), grad_weights.getData().end(), 0.0);
    std::fill(grad_bias.getData().begin(), grad_bias.getData().end(), 0.0);
}

std::map<std::string, Tensor<double>> DenseLayer::getParameters() const {
    std::map<std::string, Tensor<double>> params;
    params["weights"] = weights;
    params["bias"] = bias;
    return params;
}

void DenseLayer::setParameters(const std::map<std::string, Tensor<double>>& params) {
    if (params.count("weights")) weights = Matrix<double>(params.at("weights"));
    if (params.count("bias")) bias = Vector<double>(params.at("bias"));
}

// ==================== تطبيق Loss ====================

double Loss::calculate(const Tensor<double>& y_true, const Tensor<double>& y_pred, LossType type) {
    const auto& true_data = y_true.getData();
    const auto& pred_data = y_pred.getData();
    double loss = 0;

    switch (type) {
        case LossType::MSE:
            for (size_t i = 0; i < true_data.size(); ++i) {
                double diff = true_data[i] - pred_data[i];
                loss += diff * diff;
            }
            return loss / true_data.size();
        
        case LossType::CROSS_ENTROPY:
            for (size_t i = 0; i < true_data.size(); ++i) {
                // Avoid log(0)
                double p = std::max(1e-15, std::min(1.0 - 1e-15, pred_data[i]));
                loss -= true_data[i] * std::log(p) + (1.0 - true_data[i]) * std::log(1.0 - p);
            }
            return loss / true_data.size();
    }
    return 0;
}

Tensor<double> Loss::gradient(const Tensor<double>& y_true, const Tensor<double>& y_pred, LossType type) {
    Tensor<double> grad(y_true.getShape());
    const auto& true_data = y_true.getData();
    const auto& pred_data = y_pred.getData();
    auto& grad_data = grad.getData();

    switch (type) {
        case LossType::MSE:
            for (size_t i = 0; i < grad_data.size(); ++i) {
                grad_data[i] = 2.0 * (pred_data[i] - true_data[i]) / grad_data.size();
            }
            break;
        
        case LossType::CROSS_ENTROPY:
            for (size_t i = 0; i < grad_data.size(); ++i) {
                double p = std::max(1e-15, std::min(1.0 - 1e-15, pred_data[i]));
                grad_data[i] = (p - true_data[i]) / (p * (1.0 - p));
            }
            break;
    }
    return grad;
}

// ==================== تطبيق NeuralNetwork ====================

NeuralNetwork::NeuralNetwork(LossType loss, double lr) : loss_type(loss), learning_rate(lr) {}

void NeuralNetwork::addLayer(std::shared_ptr<Layer> layer) {
    layers.push_back(layer);
}

Tensor<double> NeuralNetwork::predict(const Tensor<double>& input) {
    Tensor<double> output = input;
    for (auto& layer : layers) {
        output = layer->forward(output);
    }
    return output;
}

double NeuralNetwork::train_step(const Tensor<double>& X, const Tensor<double>& y) {
    // Forward
    Tensor<double> output = predict(X);
    double loss = Loss::calculate(y, output, loss_type);

    // Backward
    Tensor<double> grad = Loss::gradient(y, output, loss_type);
    for (int i = static_cast<int>(layers.size()) - 1; i >= 0; --i) {
        grad = layers[i]->backward(grad);
    }

    // Update
    for (auto& layer : layers) {
        layer->update(learning_rate);
    }

    return loss;
}

void NeuralNetwork::train(const Tensor<double>& X, const Tensor<double>& y, int epochs, int batch_size) {
    size_t num_samples = X.getShape()[0];
    
    for (int epoch = 0; epoch < epochs; ++epoch) {
        double epoch_loss = 0;
        
        for (size_t i = 0; i < num_samples; ++i) {
            // Get sample i (this is a simplified slicing)
            // In a full implementation, we'd have a slice() method
            std::vector<double> sample_data;
            size_t features = X.getShape()[1];
            for (size_t j = 0; j < features; ++j) {
                sample_data.push_back(X[{i, j}]);
            }
            
            std::vector<double> target_data;
            size_t outputs = y.getShape()[1];
            for (size_t j = 0; j < outputs; ++j) {
                target_data.push_back(y[{i, j}]);
            }
            
            Tensor<double> x_sample({features}, sample_data);
            Tensor<double> y_sample({outputs}, target_data);
            
            epoch_loss += train_step(x_sample, y_sample);
        }
        
        if (epoch % 10 == 0 || epoch == epochs - 1) {
            // Loss reporting could be added here
        }
    }
}

void NeuralNetwork::save(const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    if (!out) return;

    size_t num_layers = layers.size();
    out.write(reinterpret_cast<const char*>(&num_layers), sizeof(num_layers));

    for (auto& layer : layers) {
        auto params = layer->getParameters();
        size_t num_params = params.size();
        out.write(reinterpret_cast<const char*>(&num_params), sizeof(num_params));

        for (auto const& [name, tensor] : params) {
            size_t name_len = name.length();
            out.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
            out.write(name.c_str(), name_len);

            auto shape = tensor.getShape();
            size_t dims = shape.size();
            out.write(reinterpret_cast<const char*>(&dims), sizeof(dims));
            out.write(reinterpret_cast<const char*>(shape.data()), dims * sizeof(size_t));

            const auto& data = tensor.getData();
            size_t data_size = data.size();
            out.write(reinterpret_cast<const char*>(&data_size), sizeof(data_size));
            out.write(reinterpret_cast<const char*>(data.data()), data_size * sizeof(double));
        }
    }
}

void NeuralNetwork::load(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return;

    size_t num_layers;
    in.read(reinterpret_cast<char*>(&num_layers), sizeof(num_layers));

    for (size_t i = 0; i < num_layers && i < layers.size(); ++i) {
        size_t num_params;
        in.read(reinterpret_cast<char*>(&num_params), sizeof(num_params));

        std::map<std::string, Tensor<double>> params;
        for (size_t j = 0; j < num_params; ++j) {
            size_t name_len;
            in.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
            std::string name(name_len, ' ');
            in.read(&name[0], name_len);

            size_t dims;
            in.read(reinterpret_cast<char*>(&dims), sizeof(dims));
            std::vector<size_t> shape(dims);
            in.read(reinterpret_cast<char*>(shape.data()), dims * sizeof(size_t));

            size_t data_size;
            in.read(reinterpret_cast<char*>(&data_size), sizeof(data_size));
            std::vector<double> data(data_size);
            in.read(reinterpret_cast<char*>(data.data()), data_size * sizeof(double));

            params[name] = Tensor<double>(shape, data);
        }
        layers[i]->setParameters(params);
    }
}

// ==================== تطبيق DeepLearningTools ====================

std::pair<Tensor<double>, Tensor<double>> DeepLearningTools::create_xor_dataset() {
    // Inputs: (0,0), (0,1), (1,0), (1,1)
    // Targets: (0), (1), (1), (0)
    
    // We'll return them as vectors for now since our simple NN works sample by sample
    // In a real scenario, this would be a matrix.
    
    // Placeholder return (logic for multi-sample training needed in NN)
    return {};
}

Tensor<double> DeepLearningTools::to_one_hot(const Tensor<double>& labels, int num_classes) {
    const auto& label_data = labels.getData();
    std::vector<double> one_hot_data(label_data.size() * num_classes, 0.0);
    
    for (size_t i = 0; i < label_data.size(); ++i) {
        int label = static_cast<int>(label_data[i]);
        if (label >= 0 && label < num_classes) {
            one_hot_data[i * num_classes + label] = 1.0;
        }
    }
    
    return Tensor<double>({label_data.size(), static_cast<size_t>(num_classes)}, one_hot_data);
}

} // namespace ArabicLanguage
