//
// Created by suchencjusz on 11/19/25.
//

#ifndef CIPEREUSZ_DPP_COMMANDS_H
#define CIPEREUSZ_DPP_COMMANDS_H

#include "dpp/dpp.h"
#include "MarkovChainsNGram.h"
#include "Config.h"

class dpp_commands {
private:
    dpp::cluster &bot;
    MarkovChainsNGram &mc; // n = 1
    MarkovChainsNGram &scd_mc; // n = 2

    Config &cfg;
    std::mutex &mc_mutex;
    std::mutex &scd_mc_mutex;

    bool is_admin(const dpp::slashcommand_t &event);

    // friend class Bot;
public:
    dpp_commands(
        MarkovChainsNGram &mc,
        MarkovChainsNGram &scd_mc,
        Config &cfg,
        std::mutex &mc_mutex,
        std::mutex &scd_mc_mutex,
        dpp::cluster &bot
    ) : mc(mc), scd_mc(scd_mc), cfg(cfg), mc_mutex(mc_mutex), scd_mc_mutex(scd_mc_mutex), bot(bot) {
    }

    void import_model_from_json_file_one_gram(const dpp::slashcommand_t &event);
    void import_model_from_json_file_two_gram(const dpp::slashcommand_t &event);

    void load_from_txt(const dpp::slashcommand_t &event);

    void brain_status(const dpp::slashcommand_t &event);

    void info(const dpp::slashcommand_t &event);

    void save_models(const dpp::slashcommand_t &event);

    void get_models(const dpp::slashcommand_t &event);

    void change_model(const dpp::slashcommand_t &event, bool *first_model);

    // historia czatu
    void fetch_channel_history(const dpp::slashcommand_t& event, dpp::snowflake channel_id, dpp::snowflake before_id, std::shared_ptr<std::vector<nlohmann::json>> all_json_msgs, std::shared_ptr<std::deque<dpp::snowflake>> channels_queue);
    void process_next_channel_from_queue(const dpp::slashcommand_t& event, std::shared_ptr<std::vector<nlohmann::json>> all_json_msgs, std::shared_ptr<std::deque<dpp::snowflake>> channels_queue);
    void get_guild_logs(const dpp::slashcommand_t &event);
};


#endif //CIPEREUSZ_DPP_COMMANDS_H
