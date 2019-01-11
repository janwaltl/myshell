#include "run_prompt.h"

#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <readline/history.h>
#include <readline/readline.h>

#include "cmdexecution.h"
#include "cmdhiearchy.h"
#include "cmdparsing.h"
#include "signals.h"

/* Writes new prompt based on PWD into the passed buffer.
 * Returns number of chars printed(excl. \0).
 * If return>=length, the output was truncated. But the output
 * is always null-terminated.
 * */
static int
gen_prompt(char *buffer, int length) {
	const char *curr_dir = getenv("PWD");
	if (!curr_dir)
		curr_dir = "";

	const char *home = getenv("HOME");
	bool in_home = false;
	if (home) /* Skip the path to home if we are in home subtree. */
	{
		int home_len = strlen(home);
		/* Prefix matches. The home folder is not prefix of another or
		 * home==curr_dir. */
		if (strncmp(home, curr_dir, home_len) == 0 &&
			(curr_dir[home_len] == '/' || curr_dir[home_len] == '\0')) {
			curr_dir += home_len;
			in_home = true;
		}
	}
	int num_written =
		snprintf(buffer, length, "mysh:%s%s$", in_home ? "~" : "", curr_dir);
	if (num_written < 0)
		err(1, "Could not generate prompt(snprintf).");
	return num_written;
}

/* Signals whether the call to read_line() has been SIGINTed and
 * returned only the unfinished line which should be discarded.
 * */
static bool read_line_interrupted = false;

/*
 * Reads one line from the stdin and returns it.
 * Returns NULL on EOF.
 * If C-c has been pressed then still returns unfinished but valid line and sets
 * 'read_line_interrupted' to true. The variable is cleared in the beginning of
 * each call.
 * */
static char *
read_line() {
#define PROMPT_LEN 256
	char prompt[PROMPT_LEN];
	gen_prompt(prompt, PROMPT_LEN);
#undef PROMPT_LEN
	struct sigaction old_act;
	read_line_interrupted = false;
	block_SIGINT(&old_act);
	char *line = readline(prompt);
	set_SIGINT(&old_act);
	return line;
}

/*
 * Read one character from passed stream.
 * is used as rl_getc_function in the readline library.
 * Note:
 * 	This function together with read_line()'s SIGINT settings allows
 * 	SIGINT interruption of the input and immidietely ends the input line.
 * 	Default implementation is rl_getc which restarts interrupted read()
 *  operation. So C-c doesn't break out of readline() call => cannot offer new
 *  prompt.
 * */
int
get_char(FILE *input) {
	unsigned char c;
	int res = read(fileno(input), &c, 1);
	if (res == 1)
		return c;
	else if (res == 0) /*EOF*/
		return EOF;
	else if (errno == EINTR) /* SIGINT*/
	{
		read_line_interrupted = true;
		return '\n';
	} else
		err(1, "Failed to read an input character.");
}

int
run_prompt() {
	rl_getc_function = &get_char;
	int exval = 0;
	char *line = NULL;
	while (true) {
		line = read_line();
		if (line == NULL)
			break;
		if (read_line_interrupted) {
			free(line);
			continue;
		}
		if (strcmp(line, "") != 0)
			add_history(line);
		char *err_msg = NULL;
		Cmds *cmds = parse_line(line, &err_msg);
		free(line);
		if (!cmds) {
			dprintf(STDERR_FILENO, "error: %s\n", err_msg);
			free(err_msg);
			exval = 2;
		} else {
			exec_cmds(cmds, &exval);
			cmd_free_cmds(cmds);
		}
	}
	if (line == NULL) /*CTRL+D was pressed->newline + exit.*/
		printf("\n");
	return exval;
}
