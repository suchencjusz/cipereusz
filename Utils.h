//
// Created by ognisty on 11/11/25.
//

#ifndef CIPEREUSZ_UTILS_H
#define CIPEREUSZ_UTILS_H

#include <string_view>
#include <string>
#include <dpp/dpp.h>

inline std::string remove_interpunction(const std::string& str) {
    std::string result;
    for (char c : str) {
        if (!std::ispunct(static_cast<unsigned char>(c))) {
            result += c;
        }
    }
    return result;
}

inline std::string to_lower_cpp(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
    return s;
}

inline std::string get_last_word(std::string_view text) {
    constexpr std::string_view whitespace = " \t\n\r";

    size_t end_pos = text.find_last_not_of(whitespace);
    if (end_pos == std::string_view::npos) {
        return "";
    }

    size_t start_pos = text.find_last_of(whitespace, end_pos);
    if (start_pos == std::string_view::npos) {
        return std::string(text.substr(0, end_pos + 1));
    }

    return std::string(text.substr(start_pos + 1, end_pos - start_pos));
}

#endif //CIPEREUSZ_UTILS_H