#include "commands.hpp"
#include "config.hpp"
#include "parser.hpp"

#include <string>

int main(int argc, char** argv) {
	std::string file_hint;
	std::string command = "next";

	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if (arg == "next" || arg == "list" || arg == "complete" || arg == "block" || arg == "graph" ||
		    arg == "edit" || arg == "help") {
			command = arg;
		} else if (arg == "-h" || arg == "--help") {
			command = "help";
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
		return run_command(tf, command, config, filepath);
	}

	TaskFile tf;
	if (!tf.load(filepath))
		return 1;
	if (!tf.validate())
		return 1;

	return run_command(tf, command, config, filepath);
}
