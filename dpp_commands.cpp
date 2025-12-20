//
// Created by suchencjusz on 11/19/25.
//

#include "dpp/dpp.h"
#include "dpp_commands.h"
#include "Utils.h"


bool dpp_commands::is_admin(const dpp::slashcommand_t &event) {
    dpp::guild *guild = dpp::find_guild(event.command.guild_id);

    if (guild) {
        dpp::permission user_perms = guild->base_permissions(event.command.member);
        if (user_perms & dpp::p_administrator) {
            return true;
        }
    }

    return false;
}

void dpp_commands::load_from_txt(const dpp::slashcommand_t &event) {
    event.thinking(true);

    if (!is_admin(event)) {
        event.reply("You don't have admin permissions to use this command.");
        return;
    }

    dpp::snowflake attachment_id = std::get<dpp::snowflake>(event.get_parameter("file"));
    dpp::attachment file_attachment = event.command.resolved.attachments.at(attachment_id);

    if (!file_attachment.filename.ends_with(".txt")) {
        event.reply("Error: Only .txt files are supported for training.");
        return;
    }

    bot.request(file_attachment.url, dpp::m_get,
                [this, event, file_attachment](const dpp::http_request_completion_t &http_event) {
                    if (http_event.status == 200) {
                        size_t new_size = 0; {
                            std::scoped_lock lock(mc_mutex);
                            mc.load_model_from_txt_file(http_event.body);
                            new_size = mc.get_brain_size();
                        }
                        event.edit_original_response(
                            dpp::message(
                                "File " + file_attachment.filename + " processed!\nNew brain size: " + std::to_string(
                                    new_size) + " states.")
                        );
                    } else {
                        event.edit_original_response(
                            dpp::message(
                                "Error downloading file: " + file_attachment.filename + "\n(HTTP Status: " +
                                std::to_string(http_event.status) + ")")
                        );
                    }
                });
}


void dpp_commands::brain_status(const dpp::slashcommand_t &event) {
    size_t brain_size = 0;
    size_t scd_brain_size = 0; {
        std::scoped_lock lock(mc_mutex, scd_mc_mutex);
        brain_size = mc.get_brain_size();
        scd_brain_size = scd_mc.get_brain_size();
    }

    event.reply("Brain 1N size: " + std::to_string(brain_size) + " states.\n" +
                "Brain 2N size: " + std::to_string(scd_brain_size) + " states.");
}


void dpp_commands::info(const dpp::slashcommand_t &event) {
    std::string info_message =
            "cipereusz bot - genai bot alternative\n"
            "last update: 19.11.2025\n"
            "https://github.com/suchencjusz/cipereusz";

    event.reply(info_message);
}

void dpp_commands::save_models(const dpp::slashcommand_t &event) {
    if (!is_admin(event)) {
        event.reply("You don't have admin permissions to use this command.");
        return;
    }

    size_t model_one_size = 0;
    size_t model_two_size = 0; {
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


void dpp_commands::get_models(const dpp::slashcommand_t &event) {
    event.thinking(true);

    if (!is_admin(event)) {
        event.reply("You don't have admin permissions to use this command.");
        return;
    }

    std::string model_one_path;
    std::string model_two_path; {
        std::scoped_lock lock(mc_mutex, scd_mc_mutex);
        model_one_path = cfg.model_path_n_one;
        model_two_path = cfg.model_path_n_two;

        dpp::message msg = dpp::message()
                .set_content("Here are the current models:")
                .add_file("model1n.json", dpp::utility::read_file(model_one_path))
                .add_file("model2n.json", dpp::utility::read_file(model_two_path));

        dpp::user user_to_dm = event.command.get_issuing_user();
        bot.direct_message_create(user_to_dm.id, msg);

        event.edit_original_response(
            dpp::message("Models have been sent to your DMs.")
        );
    }
}

void dpp_commands::change_model(const dpp::slashcommand_t &event, bool *first_model) {
    if (!is_admin(event)) {
        event.reply("You don't have admin permissions to use this command.");
        return;
    } {
        std::scoped_lock lock(mc_mutex, scd_mc_mutex);

        *first_model = !*first_model;

        std::string response = "Switched to " + std::string(*first_model ? "1N" : "2N") + " model.";
        event.reply(response);
    }
}

void dpp_commands::process_next_channel_from_queue(const dpp::slashcommand_t &event,
                                                   std::shared_ptr<std::vector<nlohmann::json> > all_json_msgs,
                                                   std::shared_ptr<std::deque<dpp::snowflake> > channels_queue) {
    if (channels_queue->empty()) {
        if (all_json_msgs->empty()) {
            event.edit_original_response(dpp::message("No messages found anywhere."));
            return;
        }

        std::stringstream ss;
        for (const auto &j: *all_json_msgs) {
            ss << j.dump() << "\n";
        }

        std::string result_data = ss.str();

        if (result_data.size() > 8 * 1024 * 1024) {
            event.edit_original_response(dpp::message(
                "File too big for Discord! Collected " + std::to_string(all_json_msgs->size()) +
                " messages. Check console/logs locally."));
            return;
        }

        dpp::message msg("Scraping finished! Total messages: " + std::to_string(all_json_msgs->size()));
        msg.add_file("dataset.jsonl", result_data);
        event.edit_original_response(msg);
        return;
    }

    dpp::snowflake next_channel_id = channels_queue->front();
    channels_queue->pop_front();

    fetch_channel_history(event, next_channel_id, 0, all_json_msgs, channels_queue);
}

void dpp_commands::fetch_channel_history(const dpp::slashcommand_t &event, dpp::snowflake channel_id,
                                         dpp::snowflake before_id,
                                         std::shared_ptr<std::vector<nlohmann::json> > all_json_msgs,
                                         std::shared_ptr<std::deque<dpp::snowflake> > channels_queue) {

    bot.messages_get(channel_id, 0, before_id, 0, 100,
                     [this, event, channel_id, all_json_msgs, channels_queue](
                 const dpp::confirmation_callback_t &callback) {
                         if (callback.is_error()) {
                             process_next_channel_from_queue(event, all_json_msgs, channels_queue);
                             return;
                         }

                         auto msg_map = std::get<dpp::message_map>(callback.value);

                         if (msg_map.empty()) {
                             process_next_channel_from_queue(event, all_json_msgs, channels_queue);
                             return;
                         }

                         dpp::snowflake oldest = 0;

                         for (const auto &p: msg_map) {
                             dpp::message m = p.second;

                             if (m.content.empty()) continue;

                             nlohmann::json j;
                             j["id"] = std::to_string(m.id);
                             j["author_id"] = std::to_string(m.author.id);
                             j["author_name"] = m.author.username;
                             j["content"] = m.content;
                             j["channel_id"] = std::to_string(m.channel_id);
                             j["timestamp"] = m.sent;

                             if (m.message_reference.message_id != 0) {
                                 j["reply_to_msg_id"] = std::to_string(m.message_reference.message_id);

                                 // if (m.message_reference.message_id != 0) {
                                 // }
                             } else {
                                 j["reply_to_msg_id"] = nullptr;
                             }

                             all_json_msgs->push_back(j);

                             if (oldest == 0 || m.id < oldest) oldest = m.id;
                         }


                         if (msg_map.size() >= 100) {
                             fetch_channel_history(event, channel_id, oldest, all_json_msgs, channels_queue);
                         } else {
                             process_next_channel_from_queue(event, all_json_msgs, channels_queue);
                         }
                     });
}

void dpp_commands::get_guild_logs(const dpp::slashcommand_t &event) {
    if (!is_admin(event)) {
        event.reply("Admin only.");
        return;
    }
    event.thinking(true);

    dpp::guild *g = dpp::find_guild(event.command.guild_id);
    if (!g) {
        event.edit_original_response(dpp::message("Guild not found in cache."));
        return;
    }

    auto channels_queue = std::make_shared<std::deque<dpp::snowflake> >();

    for (const auto &c_id: g->channels) {
        dpp::channel *c = dpp::find_channel(c_id);
        if (c && c->is_text_channel()) {
            channels_queue->push_back(c_id);
        }
    }

    auto all_json_msgs = std::make_shared<std::vector<nlohmann::json> >();

    event.edit_original_response(dpp::message(
        "Started scraping " + std::to_string(channels_queue->size()) + " channels. This may take a while..."));

    process_next_channel_from_queue(event, all_json_msgs, channels_queue);
}
