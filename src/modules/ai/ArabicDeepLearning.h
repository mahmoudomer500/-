// ArabicDeepLearning.h - مكتبة التعلم العميق العربية
// الأسبوع الرابع من الشهر السادس: تطبيقات الذكاء الاصطناعي
#ifndef ARABIC_DEEP_LEARNING_H
#define ARABIC_DEEP_LEARNING_H

#include "ArabicML.h"
#include <vector>
#include <memory>
#include <string>
#include <map>

namespace ArabicLanguage {

// ==================== دوال التنشيط ====================

enum class ActivationType {
    SIGMOID,
    RELU,
    TANH,
    SOFTMAX,
    LINEAR
};

class Activation {
public:
    static Tensor<double> apply(const Tensor<double>& input, ActivationType type);
    static Tensor<double> derivative(const Tensor<double>& input, ActivationType type);
};

// ==================== الطبقات ====================

class Layer {
protected:
    std::string name;
    bool trainable = true;

public:
    Layer(const std::string& name) : name(name) {}
    virtual ~Layer() = default;

    virtual Tensor<double> forward(const Tensor<double>& input) = 0;
    virtual Tensor<double> backward(const Tensor<double>& grad_output) = 0;

    virtual void update(double learning_rate) {}
    
    std::string getName() const { return name; }
    bool isTrainable() const { return trainable; }

    // للحفظ والتحميل
    virtual std::map<std::string, Tensor<double>> getParameters() const { return {}; }
    virtual void setParameters(const std::map<std::string, Tensor<double>>& params) {}
};

class DenseLayer : public Layer {
private:
    Matrix<double> weights;
    Vector<double> bias;
    Tensor<double> last_input;
    Tensor<double> last_output;
    ActivationType activation;

    // تدرجات
    Matrix<double> grad_weights;
    Vector<double> grad_bias;

public:
    DenseLayer(size_t input_size, size_t output_size, ActivationType activation = ActivationType::RELU);

    Tensor<double> forward(const Tensor<double>& input) override;
    Tensor<double> backward(const Tensor<double>& grad_output) override;
    void update(double learning_rate) override;

    std::map<std::string, Tensor<double>> getParameters() const override;
    void setParameters(const std::map<std::string, Tensor<double>>& params) override;
};

// ==================== دوال الخسارة ====================

enum class LossType {
    MSE,           // Mean Squared Error
    CROSS_ENTROPY  // Binary or Categorical Cross Entropy
};

class Loss {
public:
    static double calculate(const Tensor<double>& y_true, const Tensor<double>& y_pred, LossType type);
    static Tensor<double> gradient(const Tensor<double>& y_true, const Tensor<double>& y_pred, LossType type);
};

// ==================== الشبكة العصبية ====================

class NeuralNetwork {
private:
    std::vector<std::shared_ptr<Layer>> layers;
    LossType loss_type;
    double learning_rate;

public:
    NeuralNetwork(LossType loss = LossType::MSE, double lr = 0.01);

    void addLayer(std::shared_ptr<Layer> layer);
    
    Tensor<double> predict(const Tensor<double>& input);
    double train_step(const Tensor<double>& X, const Tensor<double>& y);
    
    void train(const Tensor<double>& X, const Tensor<double>& y, int epochs, int batch_size = 32);

    // الحفظ والتحميل
    void save(const std::string& path);
    void load(const std::string& path);

    std::vector<std::shared_ptr<Layer>>& getLayers() { return layers; }
};

// ==================== أدوات التدريب ====================

class DeepLearningTools {
public:
    // توليد بيانات تجريبية
    static std::pair<Tensor<double>, Tensor<double>> create_xor_dataset();
    static std::pair<Tensor<double>, Tensor<double>> create_mnist_placeholder(int samples = 100);

    // تحويل التصنيفات إلى One-Hot
    static Tensor<double> to_one_hot(const Tensor<double>& labels, int num_classes);
};

} // namespace ArabicLanguage

#endif // ARABIC_DEEP_LEARNING_H
