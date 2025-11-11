#include <iostream>
#include <dpp/dpp.h>

#include "Utils.h"
#include "Config.h"
#include "MarkovChains.h"

using json = nlohmann::json;

MarkovChains mc;
Config cfg;
int AUTO_MESSAGES_COUNTER = 0;

void shutdown_handler(int signum) {
    std::cout << std::endl << "Shutting down gracefully..." << std::endl;

    if (!cfg.model_path.empty()) {
        mc.save_model(cfg.model_path);
    }


    exit(signum);
}

void dpp_load_from_txt(const dpp::cluster& bot, const dpp::slashcommand_t& event) {
    if (!is_admin(bot, event))
        event.reply("Brak admina!");

    dpp::snowflake attachment_id = std::get<dpp::snowflake>(event.get_parameter("plik"));
    dpp::attachment file_attachment = event.command.resolved.attachments.at(attachment_id);

    if (!file_attachment.filename.ends_with(".txt")) {
        event.reply("Błąd: To nie jest plik .txt!");
        return;
    }

    event.reply("Przetwarzam plik: " + file_attachment.filename + "...");

    // to do: dodaj opcje importu

}

int main() {
    signal(SIGINT, shutdown_handler);
    signal(SIGTERM, shutdown_handler);

    cfg.load_config("config_cipereusz.json");
    mc.load_model(cfg.model_path);

    dpp::cluster bot(cfg.discord_token, dpp::i_default_intents | dpp::i_message_content);
    bot.on_log(dpp::utility::cout_logger());

    bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
        if (event.command.get_command_name() == "load_from_txt") {
            dpp_load_from_txt(bot, event);
        }
    });

    bot.on_message_create([&bot](const dpp::message_create_t& event) {
        if (event.msg.author.id == bot.me.id) {
            return;
        }

        if (event.msg.author.is_bot() == false && !event.msg.content.empty()) {
            mc.train(event.msg.content);
        }

        // sprawdzenie czy jest ping
        bool is_mentioned = false;
        for (const auto& mentioned_user : event.msg.mentions) {
            if (mentioned_user.first.id == bot.me.id) {
                is_mentioned = true;
                break;
            }
        }

        // dla pinga
        if (is_mentioned) {
            std::string last_word = get_last_word(event.msg.content);
            std::string generated_sentence;

            if (!last_word.empty()) {
                generated_sentence = mc.generate_sentence(50, last_word, 1000);
            }
            if (generated_sentence.empty()) {
                generated_sentence = mc.generate_sentence(50, "", 1000);
            }

            if (!generated_sentence.empty()) {
                event.reply(generated_sentence);
            }

            return;
        }

        AUTO_MESSAGES_COUNTER++;

        if (AUTO_MESSAGES_COUNTER % 10 != 0) {
            return;
        }

        AUTO_MESSAGES_COUNTER = 0;

        std::string generated_sentence = mc.generate_sentence(50, "", 1000);
        if (!generated_sentence.empty()) {
            event.reply(generated_sentence);
        }
    });

    bot.on_ready([&bot](const dpp::ready_t& event) {
    if (dpp::run_once<struct register_bot_commands>()) {

        // LOAD_FROM_tXT
        dpp::slashcommand train_cmd("load_from_txt", "Trenuje bota na pliku .txt (Admin)", bot.me.id);
        train_cmd.add_option(
            dpp::command_option(dpp::co_attachment, "plik", "Plik .txt do treningu", true) // true = wymagane
        );

        bot.global_command_create(train_cmd);
    }
});

    bot.start(dpp::st_wait);
}


// int main() {
//     load_dotenv();
//     const char* BOT_TOKEN = std::getenv("DISCORD_TOKEN");
//     if (!BOT_TOKEN) {
//         std::cerr << "Error: DISCORD_TOKEN environment variable not set." << std::endl;
//         return 1;
//     }
//
//     dpp::cluster bot(BOT_TOKEN, dpp::i_default_intents | dpp::i_message_content);
//
//     bot.on_log(dpp::utility::cout_logger());
//
//     bot.on_message_create([](const dpp::message_create_t& event) {
//         std::cout << "Message received: " << event.msg.content << std::endl;
//     });
//
//     bot.on_slashcommand([](const dpp::slashcommand_t& event) {
//         if (event.command.get_command_name() == "ping") {
//             event.reply("Pong!");
//         }
//     });
//
//     bot.on_ready([&bot](const dpp::ready_t& event) {
//         if (dpp::run_once<struct register_bot_commands>()) {
//             bot.global_command_create(dpp::slashcommand("ping", "Ping pong!", bot.me.id));
//         }
//     });
//
//     bot.start(dpp::st_wait);
// }