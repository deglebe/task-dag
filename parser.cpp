#include "parser.hpp"

#include "util.hpp"

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <set>
#include <sstream>

static std::string priority_to_string(Priority p) {
	switch (p) {
		case Priority::High:
			return "!high";
		case Priority::Med:
			return "!med";
		case Priority::Low:
			return "!low";
		default:
			return "!med";
	}
}

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
		Priority priority = Priority::Med;

		size_t arrow = rest.find("->");
		std::string name_part;
		if (arrow != std::string::npos) {
			name_part = trim(rest.substr(0, arrow));
			std::string dep_str = rest.substr(arrow + 2);
			deps = split(dep_str, ',');
		} else {
			name_part = rest;
		}

		/* parse priority: !high, !med, !low */
		std::string priority_str;
		size_t priority_pos = name_part.find_last_of('!');
		if (priority_pos != std::string::npos && priority_pos < name_part.length() - 1) {
			std::string after_bang = trim(name_part.substr(priority_pos + 1));
			/* check if it's a valid priority before a space or end */
			size_t space_pos = after_bang.find(' ');
			std::string potential_priority =
			    space_pos != std::string::npos ? after_bang.substr(0, space_pos) : after_bang;

			if (potential_priority == "high" || potential_priority == "High" ||
			    potential_priority == "HIGH") {
				priority = Priority::High;
				name = trim(name_part.substr(0, priority_pos));
			} else if (potential_priority == "med" || potential_priority == "Med" ||
				   potential_priority == "MED") {
				priority = Priority::Med;
				name = trim(name_part.substr(0, priority_pos));
			} else if (potential_priority == "low" || potential_priority == "Low" ||
				   potential_priority == "LOW") {
				priority = Priority::Low;
				name = trim(name_part.substr(0, priority_pos));
			} else {
				name = name_part;
			}
		} else {
			name = name_part;
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
		t.priority = priority;
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
		const Task& task_a = tasks.at(a);
		const Task& task_b = tasks.at(b);
		/* sort by priority first (High > Med > Low) */
		if (task_a.priority != task_b.priority) {
			return static_cast<int>(task_a.priority) > static_cast<int>(task_b.priority);
		}
		/* second by line num */
		return task_a.line_num < task_b.line_num;
	});

	return actionable;
}

const Task& TaskFile::get_task(const std::string& name) const {
	return tasks.at(name);
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
	std::sort(sorted.begin(), sorted.end(), [](const Task* a, const Task* b) {
		/* sort by priority first (High > Med > Low) */
		if (a->priority != b->priority) {
			return static_cast<int>(a->priority) > static_cast<int>(b->priority);
		}
		/* second by line num */
		return a->line_num < b->line_num;
	});

	for (const Task* t : sorted) {
		std::cout << (t->completed ? "[x] " : "[ ] ") << t->name;
		if (t->priority != Priority::Med) {
			std::cout << " " << priority_to_string(t->priority);
		}
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
			std::cout << name;
			if (task.priority != Priority::Med) {
				std::cout << " " << priority_to_string(task.priority);
			}
			std::cout << " blocked by: ";
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
