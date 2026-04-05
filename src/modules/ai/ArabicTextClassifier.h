#pragma once
#include "ArabicAppsAPI.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace ArabicApps {

class ArabicTextClassifier {
private:
    std::unique_ptr<ArabicAI> ai;
    std::map<std::string, int> vocabulary;
    std::vector<std::string> labels;
    size_t max_vocab_size;

    std::vector<double> textToVector(const std::string& text);
    std::string preprocess(const std::string& text);

public:
    ArabicTextClassifier(size_t vocab_size = 100);
    ~ArabicTextClassifier() = default;

    // تدريب المصنف
    void train(const std::vector<std::pair<std::string, std::string>>& training_data, int epochs = 100);

    // التنبؤ بالفئة
    std::string predict(const std::string& text);

    // تقييم النموذج
    double evaluate(const std::vector<std::pair<std::string, std::string>>& test_data);

    // حفظ وتحميل
    bool save(const std::string& model_path, const std::string& vocab_path);
    bool load(const std::string& model_path, const std::string& vocab_path);
};

} // namespace ArabicApps
