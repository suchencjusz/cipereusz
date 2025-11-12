#include <iostream>
#include <dpp/dpp.h>
#include <sstream>      // Dla std::stringstream
#include <algorithm>    // Dla std::transform
#include <cctype>       // Dla std::tolower
#include <mutex>        // Dla std::mutex i std::scoped_lock

#include "Utils.h"
#include "Config.h"
#include "MarkovChainsNGram.h" // Używamy nowego nagłówka

using json = nlohmann::json;

const int MODEL_N = 1;
MarkovChainsNGram mc(MODEL_N);
std::mutex mc_mutex;

const int SECOND_MODEL_N = 2;
MarkovChainsNGram scd_mc(SECOND_MODEL_N);
std::mutex scd_mc_mutex;

Config cfg;

int AUTO_MESSAGES_COUNTER = 0;
int SAVE_MODELS_COUNTER = 0;

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
    std::cout << std::endl << "Shutting down gracefully..." << std::endl;

    if (!cfg.model_path_n_one.empty()) {
        std::scoped_lock lock(mc_mutex, scd_mc_mutex);

        mc.save_model(cfg.model_path_n_one);
        scd_mc.save_model(cfg.model_path_n_two);
    }

    exit(signum);
}

//
// discord commands
//

void dpp_load_from_txt(dpp::cluster &bot, const dpp::slashcommand_t &event) {
    if (!is_admin(bot, event)) {
        event.reply("You don't have admin permissions to use this command.");
        return;
    }

    dpp::snowflake attachment_id = std::get<dpp::snowflake>(event.get_parameter("file"));
    dpp::attachment file_attachment = event.command.resolved.attachments.at(attachment_id);

    if (!file_attachment.filename.ends_with(".txt")) {
        event.reply("Error: Only .txt files are supported for training. (line by line)");
        return;
    }

    event.thinking(true);
    event.reply("Downloading and processing file: " + file_attachment.filename);

    bot.request(file_attachment.url, dpp::m_get,
                [event, file_attachment](const dpp::http_request_completion_t &http_event) {
                    if (http_event.status == 200) {
                        size_t new_size = 0; {
                            std::scoped_lock lock(mc_mutex);

                            mc.load_model_from_txt_file(http_event.body);

                            new_size = mc.get_brain_size();
                        }

                        event.edit_original_response(
                            dpp::message(
                                "File " + file_attachment.filename +
                                " has been successfully processed!\nNew brain size: " + std::to_string(new_size) +
                                " states."
                            ));
                    } else {
                        event.edit_original_response(
                            dpp::message(
                                "Error downloading file: " + file_attachment.filename +
                                "\n(HTTP Status: " + std::to_string(http_event.status) + ")"
                            )
                        );
                    }
                });
}

void dpp_brain_status(dpp::cluster &bot, const dpp::slashcommand_t &event) {
    size_t brain_size = 0;
    size_t scd_brain_size = 0; {
        std::scoped_lock lock(scd_mc_mutex);
        scd_brain_size = scd_mc.get_brain_size();
    } {
        std::scoped_lock lock(mc_mutex);
        brain_size = mc.get_brain_size();
    }

    event.reply("Brain 1N size: " + std::to_string(brain_size) + " states.\n" +
                "Brain 2N size: " + std::to_string(scd_brain_size) + " states.");
}

void dpp_info(dpp::cluster &bot, const dpp::slashcommand_t &event) {
    std::string info_message =
            "cipereusz bot - genai bot alternative\n"
            "https://github.com/suchencjusz/cipereusz";

    event.reply(info_message);
}

void dpp_save_models(dpp::cluster &bot, const dpp::slashcommand_t &event) {
    if (!is_admin(bot, event)) {
        event.reply("You don't have admin permissions to use this command.");
        return;
    }

    size_t model_one_size = 0;
    size_t model_two_size = 0;

    {
        std::scoped_lock lock(mc_mutex, scd_mc_mutex);
        mc.save_model(cfg.model_path_n_one);
        scd_mc.save_model(cfg.model_path_n_two);

        model_one_size = mc.get_brain_size();
        model_two_size = scd_mc.get_brain_size();
    }

    std::string response = "Models have been saved successfully!\n Model 1N size: " +
                           std::to_string(model_one_size) + " states.\n Model 2N size: " +
                           std::to_string(model_two_size) + " states.";

    event.reply(response);
}

void dpp_get_models(dpp::cluster &bot, const dpp::slashcommand_t &event) {
    if (!is_admin(bot, event)) {
        event.reply("You don't have admin permissions to use this command.");
        return;
    }

    event.thinking(true);

    std::string model_one_path;
    std::string model_two_path;

    {
        std::scoped_lock lock(mc_mutex, scd_mc_mutex);
        model_one_path = cfg.model_path_n_one;
        model_two_path = cfg.model_path_n_two;

        dpp::message msg = dpp::message()
                .set_content("Here are the current models:")
                .add_file("model1n.json", dpp::utility::read_file(model_one_path))
                .add_file("model2n.json", dpp::utility::read_file(model_two_path));

        dpp::user user_to_dm  = event.command.get_issuing_user();
        bot.direct_message_create(user_to_dm.id, msg);
        event.edit_original_response(
            dpp::message("Models have been sent to your DMs.")
        );
    }
}


int main() {
    signal(SIGINT, shutdown_handler);
    signal(SIGTERM, shutdown_handler);

    cfg.load_config("config_cipereusz.json"); {
        std::scoped_lock lock(mc_mutex);
        mc.load_model(cfg.model_path_n_one);
    } {
        std::scoped_lock lock(scd_mc_mutex);
        scd_mc.load_model(cfg.model_path_n_two);
    }

    dpp::cluster bot(cfg.discord_token, dpp::i_default_intents | dpp::i_message_content);
    bot.on_log(dpp::utility::cout_logger());

    bot.on_slashcommand([&bot](const dpp::slashcommand_t &event) {
        if (event.command.get_command_name() == "load_from_txt")
            dpp_load_from_txt(bot, event);

        if (event.command.get_command_name() == "brain_status")
            dpp_brain_status(bot, event);

        if (event.command.get_command_name() == "info")
            dpp_info(bot, event);

        if (event.command.get_command_name() == "save_models")
            dpp_save_models(bot, event);

        if (event.command.get_command_name() == "get_models")
            dpp_get_models(bot, event);
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
            StatePrefix start_prefix;

            std::stringstream ss(event.msg.content);
            std::string word;
            std::vector<std::string> processed_words;

            while (ss >> word) {
                std::string processed = process_word_for_prefix(word);
                if (!processed.empty()) {
                    processed_words.push_back(processed);
                }
            }

            if (!processed_words.empty()) {
                int start_index = std::max(0, static_cast<int>(processed_words.size()) - MODEL_N);
                for (size_t i = start_index; i < processed_words.size(); ++i) {
                    start_prefix.push_back(processed_words[i]);
                }
            }

            std::string generated_sentence; {
                std::scoped_lock lock(mc_mutex);

                if (!start_prefix.empty()) {
                    generated_sentence = mc.generate_sentence(50, start_prefix, 1000);
                }

                if (generated_sentence.empty()) {
                    generated_sentence = mc.generate_sentence(50, {}, 1000);
                }
            }

            if (!generated_sentence.empty()) {
                event.reply(generated_sentence);
            }

            return;
        }

        AUTO_MESSAGES_COUNTER++;

        if (AUTO_MESSAGES_COUNTER % 13 != 0) {
            return;
        }

        AUTO_MESSAGES_COUNTER = 0;

        std::string generated_sentence; {
            std::scoped_lock lock(mc_mutex);
            generated_sentence = mc.generate_sentence(50, {}, 1000);
        }

        if (!generated_sentence.empty()) {
            event.reply(generated_sentence);
        }

        SAVE_MODELS_COUNTER++;

        if (SAVE_MODELS_COUNTER % 50 == 0) {
            SAVE_MODELS_COUNTER = 0;

            std::scoped_lock lock(mc_mutex);
            mc.save_model(cfg.model_path_n_one);

            std::scoped_lock lock2(scd_mc_mutex);
            scd_mc.save_model(cfg.model_path_n_two);
        }
    });

    bot.on_ready([&bot](const dpp::ready_t &event) {
        if (dpp::run_once<struct register_bot_commands>()) {
            // LOAD_FROM_TXT
            dpp::slashcommand train_cmd("load_from_txt", "Trains bot from txt file (line by line) (Admin)", bot.me.id);
            train_cmd.add_option(
                dpp::command_option(dpp::co_attachment, "file", "Txt file for training ", true) // true = wymagane
            );

            // BRAIN_STATUS
            dpp::slashcommand brain_status("brain_status", "Shows the current brain size",
                                           bot.me.id);

            // INFO
            dpp::slashcommand info_cmd("info", "Info about project", bot.me.id);

            // SAVE_MODELS
            dpp::slashcommand save_models("save_models", "Saves the current models to disk (Admin)", bot.me.id);

            // GET_MODELS
            dpp::slashcommand get_models("get_models", "Gets the current models as files (Admin), sends to DM",
                                         bot.me.id);

            bot.global_command_create(train_cmd);
            bot.global_command_create(brain_status);
            bot.global_command_create(info_cmd);
            bot.global_command_create(save_models);
            bot.global_command_create(get_models);
        }
    });


    bot.start(dpp::st_wait);
}
