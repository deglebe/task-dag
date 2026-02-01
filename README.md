# task-dag

a dag (directed acyclic graph)-based task manager that shows only actionable tasks

## the problem

standard todo lists lack hierarchy. a list of 50 items may only have 3 actionable items since the others depend on those
three.

## the solution

model tasks as a directed acyclic graph. this program's default output shows tasks with zero uncompleted dependencies.

## building

```sh
make
make setup	# optional, move config to ~/.config/task-dag/config
make install	# optional, to /usr/local/bin
```

## file format

```
# comments start with octothorpe

[ ] task name
[ ] task with deps -> dep1, dep2
[ ] high priority task !high
[ ] low priority task !low -> dep1
[x] completed task
```

dependencies are comma-separated task names. task names are everything between the checkbox and arrow.

priorities can be specified with `!high`, `!med`, or `!low` at the end of the task name (before the arrow if dependencies exist). default priority is `!med`. tasks are sorted by priority (high > med > low) in all outputs.

## sub-commands

```sh
task-dag [file] next		# show actionable tasks (no pending deps)
task-dag [file] list		# show all tasks with their status
task-dag [file] complete	# mark a task complete (reads name from stdin)
task-dag done			# mark next task complete
task-dag [file] block		# show what's blocking each pending task
task-dag [file] graph		# output dot format for graphviz
```
If no file is specified, looks for: $TASKDAG_FILE, tasks.dag, tasks.txt,
todo.dag, todo.txt (in that order).

## workflow

1. brain dump all tasks into a file
2. run `task-dag next`
3. pick one and do it
4. run `echo "task name" | task-dag complete`
5. repeat from step 2

you can also use fzf:

`task-dag next | fzf | task-dag complete`

## visualization

if you have graphviz, you can visualize it like this:

`task-dag graph | dot -Tpng -o tasks.png`

## on development

i wrote this to learn c++ for a job interview, as well as to benefit myself by more formalizing my todo workflow. i
can't promise my work here is that great, since i'm not a massive fan of oop (read: skill issue). would eventually like
to do a rewrite in c. as such, this is public domain.

EDIT: looks like i use this every day now :P so i guess i'm going to do some more work on it over the coming months in
my spare time. maybe a rewrite will come, but as a result of the interview i have to keep working with c++ >:D
