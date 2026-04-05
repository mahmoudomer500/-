#include "ArabicTextClassifier.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <fstream>

namespace ArabicApps {

ArabicTextClassifier::ArabicTextClassifier(size_t vocab_size) 
    : ai(std::make_unique<ArabicAI>()), max_vocab_size(vocab_size) {}

std::string ArabicTextClassifier::preprocess(const std::string& text) {
    // Simple preprocessing: remove punctuation and normalize some Arabic chars
    std::string processed = text;
    // Replace some characters (simplified)
    // Note: using strings instead of char constants for Arabic UTF-8
    size_t pos;
    auto replace_all = [&](const std::string& from, const std::string& to) {
        size_t start_pos = 0;
        while((start_pos = processed.find(from, start_pos)) != std::string::npos) {
            processed.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
    };

    replace_all("أ", "ا");
    replace_all("إ", "ا");
    replace_all("آ", "ا");
    replace_all("ة", "ه");
    
    // Remove punctuation (simplified)
    processed.erase(std::remove_if(processed.begin(), processed.end(), [](char c) {
        return ispunct(c);
    }), processed.end());
    
    return processed;
}

std::vector<double> ArabicTextClassifier::textToVector(const std::string& text) {
    std::vector<double> vec(max_vocab_size, 0.0);
    std::string processed = preprocess(text);
    std::stringstream ss(processed);
    std::string word;
    
    while (ss >> word) {
        if (vocabulary.count(word)) {
            vec[vocabulary[word]] += 1.0;
        }
    }
    
    // Normalize vector
    double sum = 0;
    for (double v : vec) sum += v * v;
    if (sum > 0) {
        double mag = std::sqrt(sum);
        for (double& v : vec) v /= mag;
    }
    
    return vec;
}

void ArabicTextClassifier::train(const std::vector<std::pair<std::string, std::string>>& training_data, int epochs) {
    // 1. Build vocabulary
    std::map<std::string, int> word_counts;
    for (const auto& item : training_data) {
        std::string processed = preprocess(item.first);
        std::stringstream ss(processed);
        std::string word;
        while (ss >> word) word_counts[word]++;
    }
    
    // Sort words by count and take top max_vocab_size
    std::vector<std::pair<int, std::string>> sorted_words;
    for (const auto& pair : word_counts) sorted_words.push_back({pair.second, pair.first});
    std::sort(sorted_words.rbegin(), sorted_words.rend());
    
    vocabulary.clear();
    for (size_t i = 0; i < std::min(max_vocab_size, sorted_words.size()); ++i) {
        vocabulary[sorted_words[i].second] = i;
    }
    
    // 2. Identify unique labels
    std::map<std::string, int> label_to_idx;
    labels.clear();
    for (const auto& item : training_data) {
        if (label_to_idx.find(item.second) == label_to_idx.end()) {
            label_to_idx[item.second] = labels.size();
            labels.push_back(item.second);
        }
    }
    
    // 3. Create model
    // Input: vocab_size, Hidden: 16, Output: labels.size()
    std::string config = "dense:" + std::to_string(max_vocab_size) + ",16,tanh;";
    config += "dense:16," + std::to_string(labels.size()) + ",softmax";
    ai->createDeepModel(config);
    
    // 4. Training loop
    std::cout << "Starting training with " << training_data.size() << " samples..." << std::endl;
    for (int epoch = 0; epoch < epochs; ++epoch) {
        double total_loss = 0;
        for (const auto& item : training_data) {
            std::vector<double> input = textToVector(item.first);
            std::vector<double> target(labels.size(), 0.0);
            target[label_to_idx[item.second]] = 1.0;
            
            total_loss += ai->trainStep(input, target);
        }
        if (epoch % (epochs / 10 + 1) == 0) {
            std::cout << "Epoch " << epoch << " - Loss: " << total_loss / training_data.size() << std::endl;
        }
    }
}

std::string ArabicTextClassifier::predict(const std::string& text) {
    std::vector<double> input = textToVector(text);
    ArabicAIResult result = ai->predictDeep(input);
    
    if (!result.success || result.suggestions.empty()) return "Unknown";
    
    // Find index of max value
    int max_idx = 0;
    double max_val = -1.0;
    for (size_t i = 0; i < result.suggestions.size(); ++i) {
        double val = std::stod(result.suggestions[i]);
        if (val > max_val) {
            max_val = val;
            max_idx = i;
        }
    }
    
    if (max_idx < labels.size()) return labels[max_idx];
    return "Unknown";
}

double ArabicTextClassifier::evaluate(const std::vector<std::pair<std::string, std::string>>& test_data) {
    if (test_data.empty()) return 0.0;
    
    int correct = 0;
    for (const auto& item : test_data) {
        if (predict(item.first) == item.second) {
            correct++;
        }
    }
    return static_cast<double>(correct) / test_data.size();
}

bool ArabicTextClassifier::save(const std::string& model_path, const std::string& vocab_path) {
    if (!ai->saveModel(model_path)) return false;
    
    std::ofstream ofs(vocab_path);
    if (!ofs) return false;
    
    // Save vocab size and labels
    ofs << max_vocab_size << "\n";
    ofs << labels.size() << "\n";
    for (const auto& label : labels) ofs << label << "\n";
    
    // Save vocabulary
    ofs << vocabulary.size() << "\n";
    for (const auto& pair : vocabulary) {
        ofs << pair.first << " " << pair.second << "\n";
    }
    
    return true;
}

bool ArabicTextClassifier::load(const std::string& model_path, const std::string& vocab_path) {
    if (!ai->loadModel(model_path)) return false;
    
    std::ifstream ifs(vocab_path);
    if (!ifs) return false;
    
    ifs >> max_vocab_size;
    size_t label_count;
    ifs >> label_count;
    labels.clear();
    std::string label;
    std::getline(ifs, label); // consume newline
    for (size_t i = 0; i < label_count; ++i) {
        std::getline(ifs, label);
        labels.push_back(label);
    }
    
    size_t vocab_count;
    ifs >> vocab_count;
    vocabulary.clear();
    for (size_t i = 0; i < vocab_count; ++i) {
        std::string word;
        int idx;
        ifs >> word >> idx;
        vocabulary[word] = idx;
    }
    
    return true;
}

} // namespace ArabicApps
