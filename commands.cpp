#include "commands.hpp"

#include "config.hpp"
#include "task.hpp"
#include "util.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

void usage(const char* prog) {
	std::cerr << "usage: " << prog << " [file] <command>\n\n"
		  << "commands:\n"
		  << "  next      show actionable tasks (default)\n"
		  << "  list      show all tasks\n"
		  << "  add       add a new task\n"
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

/* command implementations */
int cmd_next(TaskFile& tf, [[maybe_unused]] const Config& config, [[maybe_unused]] const std::string& filepath,
	     [[maybe_unused]] const std::vector<std::string>& args) {
	for (const auto& name : tf.get_next()) {
		std::cout << name << "\n";
	}
	return 1;
}

int cmd_done(TaskFile& tf, [[maybe_unused]] const Config& config, [[maybe_unused]] const std::string& filepath,
	     [[maybe_unused]] const std::vector<std::string>& args) {
	std::vector<std::string> actionable = tf.get_next();
	if (actionable.empty()) {
		std::cerr << "no actionable tasks\n";
		return 0;
	} else if (actionable.size() == 1) {
		std::string task_name = actionable[0];
		if (!tf.complete(task_name))
			return 1;
		std::cout << "completed: " << task_name << "\n";
		return 0;
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
		std::cerr << "use 'task-dag complete' to complete one of these tasks\n";
		return 1;
	}
}

int cmd_complete(TaskFile& tf, [[maybe_unused]] const Config& config, [[maybe_unused]] const std::string& filepath,
		 [[maybe_unused]] const std::vector<std::string>& args) {
	std::string task_name;
	if (!std::getline(std::cin, task_name)) {
		std::cerr << "error: no task name provided\n";
		return 1;
	}
	task_name = trim(task_name);
	if (!tf.complete(task_name))
		return 1;
	std::cout << "completed: " << task_name << "\n";
	return 0;
}

int cmd_edit([[maybe_unused]] TaskFile& tf, const Config& config, const std::string& filepath,
	     [[maybe_unused]] const std::vector<std::string>& args) {
	std::string cmd = config.editor + " \"" + filepath + "\"";
	int result = std::system(cmd.c_str());
	if (result != 0) {
		std::cerr << "error: failed to launch editor '" << config.editor << "'\n";
		return 1;
	}
	return 0;
}

int cmd_add([[maybe_unused]] TaskFile& tf, [[maybe_unused]] const Config& config, [[maybe_unused]] const std::string& filepath,
	    const std::vector<std::string>& args) {
	/* parse arguments: "task name" [--priority h|m|l] [--deps "dep1,dep2"] */
	std::string task_name;
	Priority priority = Priority::Med;
	std::vector<std::string> deps;

	if (args.empty()) {
		std::cerr << "error: task name required\n";
		std::cerr << "usage: task-dag add \"task name\" [--priority h|m|l] [--deps \"dep1,dep2\"]\n";
		return 1;
	}

	/* find task name (first argument that is not a flag or flag value) */
	size_t task_name_idx = SIZE_MAX;
	for (size_t i = 0; i < args.size(); i++) {
		if (args[i] == "--priority" || args[i] == "--deps") {
			/* this is a flag, skip its value */
			if (i + 1 >= args.size()) {
				std::cerr << "error: " << args[i] << " requires a value\n";
				return 1;
			}
			i++; /* skip the flag value */
		} else {
			/* this is not a flag, it must be the task name */
			task_name_idx = i;
			break;
		}
	}

	if (task_name_idx == SIZE_MAX) {
		std::cerr << "error: task name required\n";
		std::cerr << "usage: task-dag add \"task name\" [--priority h|m|l] [--deps \"dep1,dep2\"]\n";
		return 1;
	}

	/* parse task name (may be quoted) */
	task_name = args[task_name_idx];
	if (task_name.front() == '"' && task_name.back() == '"' && task_name.length() >= 2) {
		task_name = task_name.substr(1, task_name.length() - 2);
	}
	task_name = trim(task_name);
	if (task_name.empty()) {
		std::cerr << "error: task name cannot be empty\n";
		return 1;
	}

	/* parse optional flags */
	for (size_t i = 0; i < args.size(); i++) {
		if (i == task_name_idx) {
			continue; /* skip the task name */
		}
		/* check if this is a flag value (the argument after a flag) */
		if (i > 0 && (args[i - 1] == "--priority" || args[i - 1] == "--deps")) {
			continue; /* skip flag values, they're handled when we process the flag */
		}
		if (args[i] == "--priority") {
			if (i + 1 >= args.size()) {
				std::cerr << "error: --priority requires a value (h|m|l)\n";
				return 1;
			}
			std::string prio = args[i + 1];
			if (prio == "h" || prio == "high") {
				priority = Priority::High;
			} else if (prio == "m" || prio == "med" || prio == "medium") {
				priority = Priority::Med;
			} else if (prio == "l" || prio == "low") {
				priority = Priority::Low;
			} else {
				std::cerr << "error: invalid priority '" << prio << "', must be h|m|l\n";
				return 1;
			}
			i++; /* skip the flag value */
		} else if (args[i] == "--deps") {
			if (i + 1 >= args.size()) {
				std::cerr << "error: --deps requires a value\n";
				return 1;
			}
			std::string deps_str = args[i + 1];
			/* remove quotes if present */
			if (deps_str.front() == '"' && deps_str.back() == '"' && deps_str.length() >= 2) {
				deps_str = deps_str.substr(1, deps_str.length() - 2);
			}
			deps = split(deps_str, ',');
			/* trim each dependency */
			for (auto& dep : deps) {
				dep = trim(dep);
			}
			i++; /* skip the flag value */
		} else {
			std::cerr << "error: unexpected argument '" << args[i] << "'\n";
			std::cerr << "usage: task-dag add \"task name\" [--priority h|m|l] [--deps \"dep1,dep2\"]\n";
			return 1;
		}
	}

	/* print parsed values for now */
	std::cout << "parsed task:\n";
	std::cout << "  name: " << task_name << "\n";
	std::cout << "  priority: ";
	switch (priority) {
		case Priority::High:
			std::cout << "high";
			break;
		case Priority::Med:
			std::cout << "med";
			break;
		case Priority::Low:
			std::cout << "low";
			break;
	}
	std::cout << "\n";
	if (!deps.empty()) {
		std::cout << "  deps: ";
		for (size_t i = 0; i < deps.size(); i++) {
			if (i > 0)
				std::cout << ", ";
			std::cout << deps[i];
		}
		std::cout << "\n";
	}

	return 0;
}

int run_command(TaskFile& tf, const std::string& command, const Config& config, const std::string& filepath,
		const std::vector<std::string>& args) {
	if (command == "next") {
		return cmd_next(tf, config, filepath, args);
	} else if (command == "list") {
		tf.print_list();
	} else if (command == "done") {
		return cmd_done(tf, config, filepath, args);
	} else if (command == "complete") {
		return cmd_complete(tf, config, filepath, args);
	} else if (command == "block") {
		tf.print_blocked();
	} else if (command == "graph") {
		tf.print_graph(config);
	} else if (command == "edit") {
		return cmd_edit(tf, config, filepath, args);
	} else if (command == "add") {
		return cmd_add(tf, config, filepath, args);
	}

	return 0;
}
