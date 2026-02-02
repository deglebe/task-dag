#pragma once

#include <string>

struct Config {
	std::string editor;
	std::string graph_direction;
	std::string graph_text_color;
};

Config load_config();
std::string get_config_dir();
std::string get_data_dir();
