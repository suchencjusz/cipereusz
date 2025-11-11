//
// Created by ognisty on 11/11/25.
//

#ifndef CIPEREUSZ_UTILS_H
#define CIPEREUSZ_UTILS_H

#include <string_view>
#include <string>
#include <dpp/dpp.h>

inline std::string to_lower_cpp(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
    return s;
}


inline bool is_admin(const dpp::cluster& bot, const dpp::slashcommand_t& command_event) {
    auto guild_id = command_event.command.guild_id;

    dpp::guild* guild = dpp::find_guild(command_event.command.guild_id);

    if (guild) {
        dpp::permission user_perms = guild->base_permissions(command_event.command.member);
        if (user_perms & dpp::p_administrator) {
            return true;
        }
    }

    return false;
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