//
// Created by ognisty on 11/11/25.
//

#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <vector>

#include <dpp/json.h>

#include "MarkovChainsNGram.h"
#include "Utils.h"

const std::string START_TOKEN = "$START";
const std::string END_TOKEN = "$END";

using json = nlohmann::json;

std::string MarkovChainsNGram::word_process(const std::string &word) const {
    // tokens for markov chains
    if (word == START_TOKEN || word == END_TOKEN) {
        return "";
    }

    // media links etc.
    if (word.rfind("http:", 0) == 0 || word.rfind("https:", 0) == 0) {
        return word;
    }

    // discord pings
    if (word.rfind("<a:", 0) == 0 || word.rfind("<:", 0) == 0 || word.rfind("<@&", 0) == 0 || word.rfind("<@", 0) == 0) {
        // bot id
        if (word.contains("1437781243751301264")) {
            return "";
        }

        return word;
    }

    return to_lower_cpp(word);
}

//
// prywatki
//

std::string MarkovChainsNGram::get_next_word(const SuffixMap &suffixes) const {
    if (suffixes.empty()) {
        return END_TOKEN;
    }

    std::vector<std::string> items;
    std::vector<double> weights;

    for (const auto &[word, count]: suffixes) {
        items.push_back(word);
        weights.push_back(static_cast<double>(count));
    }

    std::discrete_distribution<> dist(weights.begin(), weights.end());

    return items[dist(rng)];
}

//
// public
//

void MarkovChainsNGram::train(const std::string &text) {
    std::stringstream ss(text);
    std::string word;

    std::deque<std::string> window;

    for (int i = 0; i < n; ++i) {
        window.push_back(START_TOKEN);
    }

    while (ss >> word) {
        std::string processed_word = word_process(word);

        if (processed_word.empty()) {
            continue;
        }

        StatePrefix prefix_key(window.begin(), window.end());

        brain[prefix_key][processed_word]++;

        window.pop_front();
        window.push_back(processed_word);
    }

    StatePrefix prefix_key(window.begin(), window.end());
    brain[prefix_key][END_TOKEN]++;
}

std::string MarkovChainsNGram::generate_sentence(int max_length, StatePrefix start_prefix, int max_attempts) const {
    if (brain.empty()) {
        return "";
    }

    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        std::stringstream ss;
        StatePrefix current_prefix;

        if (!start_prefix.empty()) {
            current_prefix = start_prefix;

            if (current_prefix.size() > n) {
                current_prefix = StatePrefix(current_prefix.end() - n, current_prefix.end());
            } else if (current_prefix.size() < n) {
                StatePrefix padding(n - current_prefix.size(), START_TOKEN);

                padding.insert(padding.end(), current_prefix.begin(), current_prefix.end());
                current_prefix = padding;
            }

            for (const auto &w: start_prefix) {
                ss << w << " ";
            }

        } else {
            current_prefix.assign(n, START_TOKEN);
        }

        for (int i = 0; i < max_length; ++i) {
            auto it = brain.find(current_prefix);

            if (it == brain.end()) {
                break;
            }

            const SuffixMap &suffixes = it->second;
            std::string next_word = get_next_word(suffixes);

            if (next_word == END_TOKEN) {
                break;
            }

            int current_len = ss.str().length();
            int added_len = next_word.length() + 1;

            if (current_len + added_len > max_length) {
                break;
            }

            ss << next_word << " ";

            current_prefix.pop_front();
            current_prefix.push_back(next_word);
        }

        std::string final_sentence = ss.str();
        if (!final_sentence.empty()) {
            final_sentence.pop_back();
        }

        std::string start_string_check;
        if (!start_prefix.empty()) {
            std::stringstream ss_start;

            for (const auto &w: start_prefix) {
                ss_start << w << " ";
            }

            start_string_check = ss_start.str();
            start_string_check.pop_back();
        }

        if (final_sentence != start_string_check) {
            return final_sentence;
        }
    }

    return "";
}


void MarkovChainsNGram::save_model(const std::string &filename) const {
    std::cout << "Saving N-Gram model to JSON: " << filename << "..." << std::endl;
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not save model, failed to open " << filename << std::endl;
        return;
    }

    json model_data = json::array();
    for (const auto &[prefix, suffix_map]: brain) {
        json entry;
        entry["prefix"] = prefix;
        entry["suffixes"] = suffix_map;
        model_data.push_back(entry);
    }

    json root;
    root["n"] = this->n;
    root["model"] = model_data;

    try {
        file << root.dump(4);
    } catch (const std::exception &e) {
        std::cerr << "JSON dump error! " << e.what() << std::endl;
        return;
    }

    std::cout << "Model saved successfully, states: " << brain.size() << std::endl;
}

void MarkovChainsNGram::load_model(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Model file not found: " << filename << ". Starting with an empty model." << std::endl;
        return;
    }

    json root;
    try {
        file >> root;
    } catch (json::parse_error &e) {
        std::cerr << "JSON parsing error! " << e.what() << std::endl;
        return;
    }

    brain.clear();

    try {
        int loaded_n = root.at("n").get<int>();

        if (loaded_n != this->n) {
            std::cerr << "Model file error: 'n' mismatch. File is n=" << loaded_n
                    << ", but this model instance is n=" << this->n << std::endl;
            return;
        }

        json model_data = root.at("model");
        if (!model_data.is_array()) {
            std::cerr << "Model file error: 'model' field is not an array." << std::endl;
            return;
        }

        for (const auto &entry: model_data) {
            StatePrefix prefix = entry.at("prefix").get<StatePrefix>();
            SuffixMap suffixes = entry.at("suffixes").get<SuffixMap>();
            brain[prefix] = suffixes;
        }
    } catch (const std::exception &e) {
        std::cerr << "Error converting JSON->Model! " << e.what() << std::endl;
        brain.clear();
        return;
    }
    std::cout << "Model loaded successfully, states: " << brain.size() << std::endl;
}

void MarkovChainsNGram::load_model_from_txt_file(const std::string &content) {
    brain.clear();

    std::stringstream buffer(content);

    train(buffer.str());

    std::cout << "Model loaded from text content." << std::endl;
    std::cout << "Brain size: " << brain.size() << " states." << std::endl;
}

size_t MarkovChainsNGram::get_brain_size() const {
    return brain.size();
}
