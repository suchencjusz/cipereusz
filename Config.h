//
// Created by ognisty on 11/11/25.
//

#ifndef CIPEREUSZ_CONFIG_H
#define CIPEREUSZ_CONFIG_H

#include <string>
#include <fstream>
#include <iostream>
#include <dpp/json.h>

using json = nlohmann::json;

struct Config {
    std::string discord_token;
    std::string model_path;

    void load_config(const std::string& config_file) {
        std::ifstream f(config_file);
        if (!f.is_open()) {
            std::cerr << "Error opening config file: " << config_file << std::endl;
            std::cerr << "Creating a default config file." << std::endl;

            json default_config = {
                {"discord_token", "YOUR_DISCORD_BOT_TOKEN_HERE"},
                {"model_path", "model.json"}
            };

            try {
                std::ofstream o(config_file);
                o << std::setw(4) << default_config << std::endl;

                o.close();
            } catch (const std::exception& e) {
                std::cerr << "Failed to create default config file: " << e.what() << std::endl;
                exit(-1);
            }
            std::cerr << "Please edit the config file with your Discord bot token and restart the application." << std::endl;
            exit(-1);
        }

        json data = json::parse(f);

        discord_token = data.at("discord_token");
        model_path = data.at("model_path");

        std::cerr << "Config loaded successfully." << std::endl;
    }
};

#endif //CIPEREUSZ_CONFIG_H