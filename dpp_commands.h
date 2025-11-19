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

    void load_from_txt(const dpp::slashcommand_t &event);

    void brain_status(const dpp::slashcommand_t &event);

    void info(const dpp::slashcommand_t &event);

    void save_models(const dpp::slashcommand_t &event);

    void get_models(const dpp::slashcommand_t &event);

    void change_model(const dpp::slashcommand_t &event, bool* first_model);
};


#endif //CIPEREUSZ_DPP_COMMANDS_H
