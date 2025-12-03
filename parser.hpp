#pragma once

#include "task.hpp"

#include <map>
#include <string>
#include <vector>

struct TaskFile {
	std::string path;
	std::vector<std::string> lines;
	std::map<std::string, Task> tasks;

	bool load(const std::string& filepath);
	bool save();
	bool validate();
	std::vector<std::string> get_next();
	bool complete(const std::string& name);
	void print_list();
	void print_blocked();
	void print_graph();
};
