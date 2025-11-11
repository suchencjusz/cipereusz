//
// Created by ognisty on 11/11/25.
//

#ifndef CIPEREUSZ_MARKOVCHAINS_H
#define CIPEREUSZ_MARKOVCHAINS_H
#include <map>
#include <random>
#include <string>
#include <vector>

using Brain = std::map<std::string, std::vector<std::string>>;

class MarkovChains {

private:
    Brain brain;
    mutable std::mt19937 rng;

    std::string word_process(const std::string& word) const;

public:
    MarkovChains() : rng(std::random_device{}()) {}

    void train(const std::string& text);
    std::string generate_sentence(int max_length = 50, const std::string& start_word = "", int max_attempts = 1000) const;

    void load_model(const std::string& filename);
    void save_model(const std::string& filename) const;

    void load_model_from_txt_file(const std::string& filename);
    size_t get_brain_size() const;
};


#endif //CIPEREUSZ_MARKOVCHAINS_H
