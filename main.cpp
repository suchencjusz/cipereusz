#include <iostream>
#include <dpp/dpp.h>
#include <sstream>
#include <algorithm>
#include <mutex>

#include "Utils.h"
#include "Config.h"
#include "dpp_commands.h"
#include "MarkovChainsNGram.h"

using json = nlohmann::json;

constexpr int FIRST_MODEL_N = 1;
MarkovChainsNGram mc(FIRST_MODEL_N);
std::mutex mc_mutex;

constexpr int SECOND_MODEL_N = 2;
MarkovChainsNGram scd_mc(SECOND_MODEL_N);
std::mutex scd_mc_mutex;

Config cfg;

int AUTO_MESSAGES_COUNTER = 0;
int SAVE_MODELS_COUNTER = 0;
bool FIRST_MODEL = true;

//
// helper functions utils
//

std::string process_word_for_prefix(const std::string &word) {
    if (word.rfind("http:", 0) == 0 || word.rfind("https:", 0) == 0) {
        return word;
    }
    if (word.rfind("<a:", 0) == 0 || word.rfind("<:", 0) == 0) {
        return word;
    }
    return to_lower_cpp(remove_interpunction(word));
}

//
// signal handlers
//

void shutdown_handler(int signum) {
    log_msg("Shutting down gracefully...", log_level::INFO);

    if (!cfg.model_path_n_one.empty()) {
        std::scoped_lock lock(mc_mutex, scd_mc_mutex);

        mc.save_model(cfg.model_path_n_one);
        scd_mc.save_model(cfg.model_path_n_two);
    }

    exit(signum);
}

//
// utils
//

std::string generate_sentence(const std::string &message) {
    std::string word;
    StatePrefix start_prefix;
    std::vector<std::string> processed_words;

    int model_n = FIRST_MODEL ? FIRST_MODEL_N : SECOND_MODEL_N;

    std::stringstream ss(message);

    while (ss >> word) {
        std::string processed = process_word_for_prefix(word);
        if (!processed.empty()) {
            processed_words.push_back(processed);
        }
    }

    if (!processed_words.empty()) {
        int start_index = std::max(0, static_cast<int>(processed_words.size()) - model_n);
        for (size_t i = start_index; i < processed_words.size(); ++i) {
            start_prefix.push_back(processed_words[i]);
        }
    }

    std::string generated_sentence; {
        std::scoped_lock lock(mc_mutex);

        if (FIRST_MODEL) {
            if (!start_prefix.empty()) {
                generated_sentence = mc.generate_sentence(50, start_prefix, 1000);
            }

            if (generated_sentence.empty()) {
                generated_sentence = mc.generate_sentence(50, {}, 1000);
            }
        } else {
            if (!start_prefix.empty()) {
                generated_sentence = scd_mc.generate_sentence(50, start_prefix, 1000);
            }

            if (generated_sentence.empty()) {
                generated_sentence = scd_mc.generate_sentence(50, {}, 1000);
            }
        }
    }

    return generated_sentence;
}

//
// discord commands
//


int main() {
    signal(SIGINT, shutdown_handler);
    signal(SIGTERM, shutdown_handler);

    cfg.load_config("config_cipereusz.json"); {
        std::scoped_lock lock(mc_mutex, scd_mc_mutex);
        mc.load_model(cfg.model_path_n_one);
        scd_mc.load_model(cfg.model_path_n_two);
    }

    std::unique_ptr<dpp::cluster> bot_ptr = std::make_unique<dpp::cluster>(
        cfg.discord_token, dpp::i_default_intents | dpp::i_message_content);

    dpp_commands dpp_commands
    (
        mc,
        scd_mc,
        cfg,
        mc_mutex,
        scd_mc_mutex,
        *bot_ptr
    );

    dpp::cluster &bot = *bot_ptr;

    auto severity_to_string = [](int sev) -> std::string {
        switch(sev) {
            case dpp::ll_trace: return "TRACE";
            case dpp::ll_debug: return "DEBUG";
            case dpp::ll_info: return "INFO";
            case dpp::ll_warning: return "WARN";
            case dpp::ll_error: return "ERROR";
            case dpp::ll_critical: return "CRITICAL";
            default: return "UNKNOWN";
        }
    };

    bot.on_log([&](const dpp::log_t &event) {
        std::string msg = "[" + severity_to_string(event.severity) + "] " + event.message;
        log_msg(msg);
    });

    // bot.on_log(dpp::utility::cout_logger());

    bot.on_slashcommand([&dpp_commands](const dpp::slashcommand_t &event) {
        if (event.command.get_command_name() == "load_from_txt")
            dpp_commands.load_from_txt(event);

        if (event.command.get_command_name() == "brain_status")
            dpp_commands.brain_status(event);

        if (event.command.get_command_name() == "info")
            dpp_commands.info(event);

        if (event.command.get_command_name() == "save_models")
            dpp_commands.save_models(event);

        if (event.command.get_command_name() == "get_models")
            dpp_commands.get_models(event);

        if (event.command.get_command_name() == "change_model")
            dpp_commands.change_model(event, &FIRST_MODEL);
    });

    bot.on_message_create([&bot](const dpp::message_create_t &event) {
        if (event.msg.author.id == bot.me.id)
            return;

        if (event.msg.is_dm() == true) {
            event.reply("I DO NOT RESPOND TO DMs :c");
            return;
        }

        if (event.msg.author.is_bot() == false && !event.msg.content.empty()) {
            {
                std::scoped_lock lock(mc_mutex, scd_mc_mutex);

                mc.train(event.msg.content);
                scd_mc.train(event.msg.content);
            }
        }

        // sprawdzenie czy jest ping
        bool is_mentioned = false;
        for (const auto &mentioned_user: event.msg.mentions) {
            if (mentioned_user.first.id == bot.me.id) {
                is_mentioned = true;
                break;
            }
        }

        // dla pinga
        if (is_mentioned) {
            std::string generated_sentence = generate_sentence(event.msg.content);

            if (!generated_sentence.empty()) {
                event.reply(generated_sentence);
            }

            return;
        }

        SAVE_MODELS_COUNTER++;

        if (SAVE_MODELS_COUNTER % 25 == 0) {
            SAVE_MODELS_COUNTER = 0;

            log_msg("Auto-saving models...", log_level::INFO);
            log_msg("Model 1N size: " + std::to_string(mc.get_brain_size()) + " states.", log_level::INFO);
            log_msg("Model 2N size: " + std::to_string(scd_mc.get_brain_size()) + " states.", log_level::INFO);

            std::scoped_lock lock(mc_mutex, scd_mc_mutex);
            mc.save_model(cfg.model_path_n_one);
            scd_mc.save_model(cfg.model_path_n_two);
        }

        AUTO_MESSAGES_COUNTER++;

        if (AUTO_MESSAGES_COUNTER % 14 != 0) {
            return;
        }

        AUTO_MESSAGES_COUNTER = 0;

        std::string generated_sentence;
        generated_sentence = generate_sentence(event.msg.content);


        if (!generated_sentence.empty()) {
            event.reply(generated_sentence);
        }
    });

    bot.on_ready([&bot](const dpp::ready_t &event) {
        if (dpp::run_once<struct register_bot_commands>()) {
            std::vector<dpp::slashcommand> commands;

            // Zbierz wszystkie komendy
            dpp::slashcommand train_cmd("load_from_txt", "Trains bot from txt file (line by line) (Admin)", bot.me.id);
            train_cmd.add_option(dpp::command_option(dpp::co_attachment, "file", "Txt file for training", true));
            commands.push_back(train_cmd);

            commands.push_back(dpp::slashcommand("brain_status", "Shows the current brain size", bot.me.id));
            commands.push_back(dpp::slashcommand("info", "Info about project", bot.me.id));
            commands.push_back(dpp::slashcommand("save_models", "Saves the current models to disk (Admin)", bot.me.id));
            commands.push_back(dpp::slashcommand("get_models", "Gets the current models as files (Admin), sends to DM", bot.me.id));
            commands.push_back(dpp::slashcommand("change_model", "Changes the current model between 1N and 2N (Admin)", bot.me.id));

            // Zarejestruj wszystkie na raz
            bot.global_bulk_command_create(commands);
        }
    });


    bot.start(dpp::st_wait);
}
