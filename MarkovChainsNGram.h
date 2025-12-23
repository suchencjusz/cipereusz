//
// Created by ognisty on 11/11/25.
//

#ifndef CIPEREUSZ_NGRAM_MODEL_H
#define CIPEREUSZ_NGRAM_MODEL_H

#include <map>
#include <random>
#include <string>
#include <deque>

using StatePrefix = std::deque<std::string>;
using SuffixMap = std::map<std::string, int>;

using Brain = std::map<StatePrefix, SuffixMap>;

class MarkovChainsNGram {

private:
    int n;
    Brain brain;
    mutable std::mt19937 rng;

    std::string word_process(const std::string& word) const;

    std::string get_next_word(const SuffixMap& suffixes) const;

public:
    MarkovChainsNGram(int order) : n(order), rng(std::random_device{}()) {}

    void train(const std::string& text);

    std::string generate_sentence(int max_length = 50, StatePrefix start_prefix = {}, int max_attempts = 1000) const;

    // Returns true on success; returns false and fills `error` on failure.
    bool import_model_from_json_file(const std::string &content, std::string* error = nullptr);

    void load_model(const std::string& filename);
    void save_model(const std::string& filename) const;

    // Trains further from plain text content (does NOT clear existing brain).
    void load_model_from_txt_file(const std::string& content);
    size_t get_brain_size() const;
};

#endif //CIPEREUSZ_NGRAM_MODEL_H