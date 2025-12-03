#include "parser.hpp"

#include "util.hpp"

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <set>
#include <sstream>

bool TaskFile::load(const std::string& filepath) {
	path = filepath;
	std::ifstream f(filepath);
	if (!f) {
		std::cerr << "error: cannot open " << filepath << "\n";
		return false;
	}

	std::string line;
	int line_num = 0;
	while (std::getline(f, line)) {
		lines.push_back(line);
		line_num++;

		std::string trimmed = trim(line);
		if (trimmed.empty() || trimmed[0] == '#')
			continue;

		/* parse: [x] or [ ] prefix */
		bool completed = false;
		std::string rest;

		if (trimmed.size() >= 3 && trimmed[0] == '[' && trimmed[2] == ']') {
			completed = (trimmed[1] == 'x' || trimmed[1] == 'X');
			rest = trim(trimmed.substr(3));
		} else {
			std::cerr << "warning: line " << line_num << ": expected [ ] or [x] prefix\n";
			continue;
		}

		/* parse: name -> dep1, dep2 */
		std::string name;
		std::vector<std::string> deps;

		size_t arrow = rest.find("->");
		if (arrow != std::string::npos) {
			name = trim(rest.substr(0, arrow));
			std::string dep_str = rest.substr(arrow + 2);
			deps = split(dep_str, ',');
		} else {
			name = rest;
		}

		if (name.empty()) {
			std::cerr << "warning: line " << line_num << ": empty task name\n";
			continue;
		}

		if (tasks.count(name)) {
			std::cerr << "warning: line " << line_num << ": duplicate task '" << name << "'\n";
			continue;
		}

		Task t;
		t.name = name;
		t.completed = completed;
		t.deps = deps;
		t.line_num = line_num;
		tasks[name] = t;
	}

	return true;
}

bool TaskFile::save() {
	std::ofstream f(path);
	if (!f) {
		std::cerr << "error: cannot write " << path << "\n";
		return false;
	}
	for (const auto& line : lines) {
		f << line << "\n";
	}
	return true;
}

bool TaskFile::validate() {
	bool valid = true;
	for (const auto& pair : tasks) {
		const std::string& name = pair.first;
		const Task& task = pair.second;
		for (const auto& dep : task.deps) {
			if (!tasks.count(dep)) {
				std::cerr << "error: task '" << name << "' depends on unknown task '" << dep << "'\n";
				valid = false;
			}
		}
	}

	std::set<std::string> visited, in_stack;
	std::vector<std::string> cycle_path;

	std::function<bool(const std::string&)> has_cycle = [&](const std::string& node) -> bool {
		visited.insert(node);
		in_stack.insert(node);
		cycle_path.push_back(node);

		if (tasks.count(node)) {
			for (const auto& dep : tasks.at(node).deps) {
				if (!visited.count(dep)) {
					if (has_cycle(dep))
						return true;
				} else if (in_stack.count(dep)) {
					std::cerr << "error: cycle detected: ";
					bool found = false;
					for (const auto& n : cycle_path) {
						if (n == dep)
							found = true;
						if (found)
							std::cerr << n << " -> ";
					}
					std::cerr << dep << "\n";
					return true;
				}
			}
		}

		cycle_path.pop_back();
		in_stack.erase(node);
		return false;
	};

	for (const auto& pair : tasks) {
		const std::string& name = pair.first;
		if (!visited.count(name)) {
			if (has_cycle(name)) {
				valid = false;
				break;
			}
		}
	}

	return valid;
}

std::vector<std::string> TaskFile::get_next() {
	std::vector<std::string> actionable;
	for (const auto& pair : tasks) {
		const std::string& name = pair.first;
		const Task& task = pair.second;
		if (task.completed)
			continue;

		bool blocked = false;
		for (const auto& dep : task.deps) {
			if (tasks.count(dep) && !tasks.at(dep).completed) {
				blocked = true;
				break;
			}
		}

		if (!blocked) {
			actionable.push_back(name);
		}
	}

	std::sort(actionable.begin(), actionable.end(), [this](const std::string& a, const std::string& b) {
		return tasks.at(a).line_num < tasks.at(b).line_num;
	});

	return actionable;
}

bool TaskFile::complete(const std::string& name) {
	if (!tasks.count(name)) {
		std::cerr << "error: unknown task '" << name << "'\n";
		return false;
	}

	Task& task = tasks.at(name);
	if (task.completed) {
		std::cerr << "warning: task '" << name << "' already completed\n";
		return true;
	}

	// Update the line in the file
	int idx = task.line_num - 1;
	std::string& line = lines[idx];
	size_t bracket = line.find("[ ]");
	if (bracket != std::string::npos) {
		line[bracket + 1] = 'x';
		task.completed = true;
		return save();
	}

	std::cerr << "error: could not find [ ] in line " << task.line_num << "\n";
	return false;
}

void TaskFile::print_list() {
	std::vector<const Task*> sorted;
	for (const auto& pair : tasks) {
		sorted.push_back(&pair.second);
	}
	std::sort(sorted.begin(), sorted.end(), [](const Task* a, const Task* b) { return a->line_num < b->line_num; });

	for (const Task* t : sorted) {
		std::cout << (t->completed ? "[x] " : "[ ] ") << t->name;
		if (!t->deps.empty()) {
			std::cout << " -> ";
			for (size_t i = 0; i < t->deps.size(); i++) {
				if (i > 0)
					std::cout << ", ";
				std::cout << t->deps[i];
			}
		}
		std::cout << "\n";
	}
}

void TaskFile::print_blocked() {
	for (const auto& pair : tasks) {
		const std::string& name = pair.first;
		const Task& task = pair.second;
		if (task.completed)
			continue;

		std::vector<std::string> blocking;
		for (const auto& dep : task.deps) {
			if (tasks.count(dep) && !tasks.at(dep).completed) {
				blocking.push_back(dep);
			}
		}

		if (!blocking.empty()) {
			std::cout << name << " blocked by: ";
			for (size_t i = 0; i < blocking.size(); i++) {
				if (i > 0)
					std::cout << ", ";
				std::cout << blocking[i];
			}
			std::cout << "\n";
		}
	}
}

void TaskFile::print_graph() {
	std::cout << "digraph tasks {\n";
	std::cout << "    rankdir=LR;\n";
	std::cout << "    node [shape=box];\n";

	for (const auto& pair : tasks) {
		const std::string& name = pair.first;
		const Task& task = pair.second;
		std::string style = task.completed ? "filled" : "solid";
		std::string fill = task.completed ? ",fillcolor=gray" : "";
		std::cout << "    \"" << name << "\" [style=" << style << fill << "];\n";
	}

	for (const auto& pair : tasks) {
		const std::string& name = pair.first;
		const Task& task = pair.second;
		for (const auto& dep : task.deps) {
			std::cout << "    \"" << dep << "\" -> \"" << name << "\";\n";
		}
	}

	std::cout << "}\n";
}
