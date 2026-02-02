#include "config.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

std::string get_home_dir() {
	const char* home = std::getenv("HOME");
	return home ? home : "";
}

std::string get_config_dir() {
	const char* xdg_config = std::getenv("XDG_CONFIG_HOME");
	if (xdg_config)
		return std::string(xdg_config) + "/task-dag";

	std::string home = get_home_dir();
	return home + "/.config/task-dag";
}

std::string get_data_dir() {
	const char* xdg_data = std::getenv("XDG_DATA_HOME");
	if (xdg_data)
		return std::string(xdg_data) + "/task-dag";

	std::string home = get_home_dir();
	return home + "/.local/share/task-dag";
}

std::map<std::string, std::string> parse_config(const std::string& filepath) {
	std::map<std::string, std::string> config;
	std::ifstream file(filepath);

	if (!file)
		return config;

	std::string line;
	while (std::getline(file, line)) {
		size_t comment_pos = line.find('#');
		if (comment_pos != std::string::npos) {
			line = line.substr(0, comment_pos);
		}

		line.erase(line.begin(),
			   std::find_if(line.begin(), line.end(), [](unsigned char ch) { return !std::isspace(ch); }));
		line.erase(
		    std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(),
		    line.end());

		if (line.empty())
			continue;

		/* parse key=value */
		size_t equals_pos = line.find('=');
		if (equals_pos != std::string::npos) {
			std::string key = line.substr(0, equals_pos);
			std::string value = line.substr(equals_pos + 1);

			key.erase(key.begin(), std::find_if(key.begin(), key.end(),
							    [](unsigned char ch) { return !std::isspace(ch); }));
			key.erase(
			    std::find_if(key.rbegin(), key.rend(), [](unsigned char ch) { return !std::isspace(ch); })
				.base(),
			    key.end());

			value.erase(value.begin(), std::find_if(value.begin(), value.end(),
								[](unsigned char ch) { return !std::isspace(ch); }));
			value.erase(std::find_if(value.rbegin(), value.rend(),
						 [](unsigned char ch) { return !std::isspace(ch); })
					.base(),
				    value.end());

			config[key] = value;
		}
	}

	return config;
}

Config load_config() {
	Config config;

	config.editor = std::getenv("EDITOR") ? std::getenv("EDITOR") : "vim";
	config.graph_direction = "horizontal";
	config.graph_text_color = ""; /* empty means auto-detect */

	std::string config_dir = get_config_dir();
	std::string config_file = config_dir + "/config";

	auto parsed = parse_config(config_file);
	if (parsed.count("editor")) {
		config.editor = parsed["editor"];
	}
	if (parsed.count("graph_direction")) {
		std::string dir = parsed["graph_direction"];
		if (dir == "vertical" || dir == "horizontal") {
			config.graph_direction = dir;
		}
	}
	if (parsed.count("graph_text_color")) {
		config.graph_text_color = parsed["graph_text_color"];
	}

	return config;
}
