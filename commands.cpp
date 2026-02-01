#include "commands.hpp"

#include "config.hpp"
#include "task.hpp"
#include "util.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

void usage(const char* prog) {
	std::cerr << "usage: " << prog << " [file] <command>\n\n"
		  << "commands:\n"
		  << "  next      show actionable tasks (default)\n"
		  << "  list      show all tasks\n"
		  << "  complete  mark task complete (reads name from stdin)\n"
		  << "  done      complete the task if only one actionable task exists\n"
		  << "  block     show blocking dependencies\n"
		  << "  graph     output DOT format\n"
		  << "  edit      open task file in editor\n"
		  << "  help      show this help\n\n"
		  << "file defaults to ~/.local/share/task-dag/tasks.dag or $TASKDAG_FILE\n";
}

std::string find_file(const std::string& hint) {
	if (!hint.empty())
		return hint;

	const char* env = std::getenv("TASKDAG_FILE");
	if (env)
		return env;

	/* check data directory first */
	std::string data_dir = get_data_dir();
	std::filesystem::create_directories(data_dir);

	const char* names[] = {"tasks.dag", "tasks.txt", "todo.dag", "todo.txt"};
	for (const char* name : names) {
		std::string path = data_dir + "/" + name;
		std::ifstream f(path);
		if (f)
			return path;
	}

	for (const char* name : names) {
		std::ifstream f(name);
		if (f)
			return name;
	}

	return data_dir + "/tasks.dag";
}

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

int run_command(TaskFile& tf, const std::string& command, const Config& config, const std::string& filepath) {
	if (command == "next") {
		for (const auto& name : tf.get_next()) {
			const Task& task = tf.get_task(name);
			std::cout << name;
			if (task.priority != Priority::Med) {
				std::cout << " " << priority_to_string(task.priority);
			}
			std::cout << "\n";
		}
	} else if (command == "list") {
		tf.print_list();
	} else if (command == "done") {
		std::vector<std::string> actionable = tf.get_next();
		if (actionable.empty()) {
			std::cerr << "no actionable tasks\n";
			return 0;
		} else if (actionable.size() == 1) {
			std::string task_name = actionable[0];
			if (!tf.complete(task_name))
				return 1;
			std::cout << "completed: " << task_name << "\n";
		} else {
			std::cerr << "multiple actionable tasks:\n";
			for (const auto& name : actionable) {
				const Task& task = tf.get_task(name);
				std::cerr << "  " << name;
				if (task.priority != Priority::Med) {
					std::cerr << " " << priority_to_string(task.priority);
				}
				std::cerr << "\n";
			}
			std::cerr << "use 'task-dag complete' or pipe through fzf to select one\n";
			return 1;
		}
	} else if (command == "complete") {
		std::string task_name;
		if (!std::getline(std::cin, task_name)) {
			std::cerr << "error: no task name provided\n";
			return 1;
		}
		task_name = trim(task_name);
		if (!tf.complete(task_name))
			return 1;
		std::cout << "completed: " << task_name << "\n";
	} else if (command == "block") {
		tf.print_blocked();
	} else if (command == "graph") {
		tf.print_graph();
	} else if (command == "edit") {
		std::string cmd = config.editor + " \"" + filepath + "\"";
		int result = std::system(cmd.c_str());
		if (result != 0) {
			std::cerr << "error: failed to launch editor '" << config.editor << "'\n";
			return 1;
		}
	}

	return 0;
}
