//
// Created by ognisty on 11/11/25.
//

#include <sstream>
#include <deque>
#include <fstream>
#include <iostream>

#include <dpp/json.h>

#include "MarkovChains.h"
#include "Utils.h"

const std::string START_TOKEN = "$START";
const std::string END_TOKEN = "$END";

using json = nlohmann::json;


std::string MarkovChains::word_process(const std::string& word) const {
    if (word.rfind("http:", 0) == 0 || word.rfind("https:", 0) == 0) {
        return word;
    }

    if (word.rfind("<a:", 0) == 0 || word.rfind("<:", 0) == 0) {
        return word;
    }

    return to_lower_cpp(word);
}


void MarkovChains::train(const std::string& text) {
    std::stringstream ss(text);
    std::string word;

    std::string prev_word = START_TOKEN;

    while (ss >> word) {
        if (word.empty() || word == START_TOKEN || word == END_TOKEN) {
            continue;
        }

        std::string processed_word = word_process(word);

        brain[prev_word].push_back(processed_word);

        prev_word = processed_word;
    }

    brain[prev_word].push_back(END_TOKEN);
}

std::string MarkovChains::generate_sentence(int max_length, const std::string& start_word, int max_attempts) const {
    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        std::stringstream ss;
        std::string current_key;

        if (!start_word.empty()) {
            std::stringstream start_ss(start_word);
            std::string temp_word;
            while (start_ss >> temp_word) {
                current_key = temp_word;
            }

            current_key = word_process(current_key);
            ss << start_word << " ";

        } else {
            current_key = START_TOKEN;
        }

        while (true) {
            auto it = brain.find(current_key);
            if (it == brain.end() || it->second.empty()) {
                break;
            }

            const std::vector<std::string>& next_words = it->second;
            std::uniform_int_distribution<> dis(0, next_words.size() - 1);
            std::string next_word = next_words[dis(rng)];

            if (next_word == END_TOKEN) {
                break;
            }

            int current_len = ss.str().length();
            int added_len = next_word.length() + 1;

            if (current_len + added_len > max_length) {
                break;
            }

            ss << next_word << " ";
            current_key = next_word;
        }

        std::string final_sentence = ss.str();
        if (!final_sentence.empty()) {
            final_sentence.pop_back();

            if (final_sentence != start_word) {
                return final_sentence;
            }
        }
    }
    return "";
}

void MarkovChains::save_model(const std::string& filename) const {
    std::cout << "Saving model to JSON: " << filename << "..." << std::endl;
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Could not save model, failed to open " << filename << std::endl;
        return;
    }

    json model_json = brain;

    try {
        file << model_json.dump(4);
    } catch (const std::exception& e) {
        std::cerr << "JSON dump error! " << e.what() << std::endl;
        return;
    }

    std::cout << "Model saved successfully, states: " << brain.size() << std::endl;
}

void MarkovChains::load_model(const std::string& filename) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Model file not found: " << filename << ". Starting with an empty model." << std::endl;
        return;
    }

    json model_json;

    try {
        file >> model_json;
    } catch (json::parse_error& e) {
        std::cerr << "JSON parsing error! " << e.what() << std::endl;
        return;
    }

    try {
        brain = model_json.get<Brain>();
    } catch (const std::exception& e) {
        std::cerr << "Error converting JSON->Model! " << e.what() << std::endl;
        brain.clear();
        return;
    }
    std::cout << "Model loaded successfully, states: " << brain.size() << std::endl;
}

void MarkovChains::load_model_from_txt_file(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Error opening file " << filename << std::endl;
    }

    brain.clear();

    for (std::string line; std::getline(file, line); ) {
        train(line);
    }

    std::cout << "Model loaded from " << filename << std::endl;
    std::cout << "Brain size: " << brain.size() << " states." << std::endl;
}

size_t MarkovChains::get_brain_size() const {
    return brain.size();
}
