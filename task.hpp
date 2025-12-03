#pragma once

#include <string>
#include <vector>

struct Task {
	std::string name;
	bool completed = false;
	std::vector<std::string> deps;
	int line_num = 0;
};
