#pragma once

#include <string>

struct Config {
	std::string editor;
	std::string graph_direction;
	std::string graph_text_color;
	std::string priority_high_color;
	std::string priority_med_color;
	std::string priority_low_color;
	std::string priority_high_bg;
	std::string priority_med_bg;
	std::string priority_low_bg;
};

Config load_config();
std::string get_config_dir();
std::string get_data_dir();
