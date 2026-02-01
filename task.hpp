#pragma once

#include <string>
#include <vector>

enum class Priority { Low, Med, High };

struct Task {
	std::string name;
	bool completed = false;
	std::vector<std::string> deps;
	Priority priority = Priority::Med;
	int line_num = 0;
};
