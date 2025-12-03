#include "util.hpp"

#include <sstream>

std::string trim(const std::string& s) {
	size_t start = s.find_first_not_of(" \t\r\n");
	if (start == std::string::npos)
		return "";
	size_t end = s.find_last_not_of(" \t\r\n");
	return s.substr(start, end - start + 1);
}

std::vector<std::string> split(const std::string& s, char delim) {
	std::vector<std::string> parts;
	std::stringstream ss(s);
	std::string part;
	while (std::getline(ss, part, delim)) {
		std::string trimmed = trim(part);
		if (!trimmed.empty()) {
			parts.push_back(trimmed);
		}
	}
	return parts;
}
