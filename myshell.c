#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/wait.h>

#include "cmdexecution.h"
#include "cmdparsing.h"
#include "myshell.h"
#include "run_script.h"
#include "run_prompt.h"
#include "signals.h"

static int
run_cmd(const char *cmd_str) {
	char *err_msg = NULL;
	Cmds *cmds = parse_line(cmd_str, &err_msg);
	if (!cmds) {
		dprintf(STDERR_FILENO, "error: %s\n", err_msg);
		free((char *)err_msg);
		return 2;
	} else {
		int exval = 0;
		exec_cmds(cmds, &exval);
		cmd_free_cmds(cmds);
		return exval;
	}
}

/*
 * Prints a message how to use the program exits.
 * */
static void
usage(char *prog_name) {
	printf("Usage:"
		   "\t%s\n"
		   "\t\t- Starts interactive mode.\n"
		   "\t%s -c CMD\n"
		   "\t\t- Executes CMD.\n"
		   "\t%s FILE\n"
		   "\t\t- Executes all commands in the FILE.\n"
		   "\nOther cases will show this help message.\n",
		   prog_name, prog_name, prog_name);
	exit(0);
}

/* Parses program's arguments.
 * Returns string passed to the -c argument or NULL.
 * Exits on syntax error.
 * */
static char *
parse_args(int argc, char *argv[]) {
	int opt;
	while ((opt = getopt(argc, argv, "c:h")) != -1) {
		switch (opt) {
		case 'c':
			return optarg;
		default:
			usage(argv[0]);
		}
	}
	return NULL;
}

int
run_myshell(int argc, char **argv) {
	char *c_arg = parse_args(argc, argv);
	if (c_arg != NULL) /* -c arg present */ {
		return run_cmd(c_arg);
	} else if (argc == 2) {
		return run_script(argv[1]);
	}
	return run_prompt();
}
