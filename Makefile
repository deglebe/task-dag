CC	= c++
CFLAGS	= -std=c++17 -Wall -Wextra -pedantic -O2
PREFIX	?= /usr/local

task-dag: main.cpp commands.cpp parser.cpp util.cpp config.cpp
	$(CC) $(CFLAGS) -o $@ $^

install: task-dag
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 task-dag $(DESTDIR)$(PREFIX)/bin/

setup:
	# Set up user config and data directories
	mkdir -p $${XDG_CONFIG_HOME:-$$HOME/.config}/task-dag
	mkdir -p $${XDG_DATA_HOME:-$$HOME/.local/share}/task-dag
	if [ ! -f $${XDG_CONFIG_HOME:-$$HOME/.config}/task-dag/config ]; then \
		cp config.default $${XDG_CONFIG_HOME:-$$HOME/.config}/task-dag/config; \
	fi
	@echo "task-dag setup complete!"
	@echo "Config: $${XDG_CONFIG_HOME:-$$HOME/.config}/task-dag/"
	@echo "Data: $${XDG_DATA_HOME:-$$HOME/.local/share}/task-dag/"

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/task-dag

clean:
	rm -f task-dag

.PHONY: install uninstall clean setup
