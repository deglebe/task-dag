#include "commands.hpp"
#include "config.hpp"
#include "parser.hpp"

#include <string>
#include <vector>

int main(int argc, char** argv) {
	std::string file_hint;
	std::string command = "next";
	std::vector<std::string> command_args;

	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if (arg == "next" || arg == "list" || arg == "complete" || arg == "block" || arg == "graph" ||
		    arg == "edit" || arg == "help" || arg == "done" || arg == "add") {
			command = arg;
			/* collect remaining arguments for commands that need them */
			if (command == "add") {
				for (int j = i + 1; j < argc; j++) {
					command_args.push_back(argv[j]);
				}
				break;
			}
		} else if (arg == "-h" || arg == "--help") {
			command = "help";
		} else if (command == "add") {
			/* collecting args for add command */
			command_args.push_back(arg);
		} else {
			file_hint = arg;
		}
	}

	if (command == "help") {
		usage(argv[0]);
		return 0;
	}

	Config config = load_config();
	std::string filepath = find_file(file_hint);

	if (command == "edit") {
		TaskFile tf; // dummy, not used
		return run_command(tf, command, config, filepath, command_args);
	}

	TaskFile tf;
	if (!tf.load(filepath))
		return 1;
	if (!tf.validate())
		return 1;

	return run_command(tf, command, config, filepath, command_args);
}
