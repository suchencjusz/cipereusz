//
// Created by ognisty on 11/11/25.
//

#ifndef CIPEREUSZ_UTILS_H
#define CIPEREUSZ_UTILS_H

#include <string_view>
#include <string>
#include <dpp/dpp.h>

// inline void log_msg(const std::string& msg) {
//     auto now = std::chrono::system_clock::now();
//     auto in_time_t = std::chrono::system_clock::to_time_t(now);
//     auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
//
//     std::cout << "[" << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S")
//               << "." << std::setfill('0') << std::setw(3) << ms.count() << "] "
//               << " " << msg << std::endl;
// }

enum class log_level {
    DPP,
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

inline void log_msg(const std::string& msg, log_level level = log_level::DPP) {
    std::string prefix;

    if (level != log_level::DPP) {
        prefix = "[cipereusz] ";
    }

    switch (level) {
        case log_level::DEBUG: prefix += "[DEBUG] "; break;
        case log_level::INFO: prefix += "[INFO] "; break;
        case log_level::WARNING: prefix += "[WARNING] "; break;
        case log_level::ERROR: prefix += "[ERROR] "; break;

        case log_level::DPP: prefix += "[dpp] "; break;
    }

    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::cout << "[" << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S")
              << "." << std::setfill('0') << std::setw(3) << ms.count() << "] "
              << prefix << msg << std::endl;
}

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