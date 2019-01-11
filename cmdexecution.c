#include "cmdexecution.h"

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <readline/history.h>
#include <readline/readline.h>
#include <sys/queue.h>
#include <sys/wait.h>

#include "signals.h"

/* Executes internal cd command according to the passed info.
 * Cmd must be valid cmd and cmd->name must be "cd".
 * */
static void
exec_cd(CmdSimple *cmd) {
	assert(cmd);
	assert(strncmp("cd", cmd->name, 3) == 0);

	/* "cd" goes to home directory.
	 * "cd -" goes to the last directory.
	 * "cd dir" goes to the 'dir' directory.
	 * */
	const char *newPWD;
	CmdArg *arg = STAILQ_FIRST(&cmd->args);
	if (arg == NULL) {
		newPWD = getenv("HOME");
		if (newPWD == NULL)
			errx(1, "cd: HOME not set.");
	} else if (STAILQ_NEXT(arg, tailq) != NULL) {
		errx(1, "cd: too many arguments.");
	} else /* One arg = new PWD */
	{
		assert(arg->val);
		if (strncmp("-", arg->val, 2) == 0) {
			const char *old = getenv("OLDPWD");
			if (old == NULL)
				errx(1, "cd: OLDPWD not set.");
			newPWD = old;
		} else
			newPWD = arg->val;
	}

	const char *currPWD = getenv("PWD");
	if (currPWD && setenv("OLDPWD", currPWD, 1) == -1)
		err(1, "Failed to set OLDPWD env. variable.");
	if (setenv("PWD", newPWD, 1) == -1)
		err(1, "Failed to set PWD varible.");
	if (chdir(newPWD) == -1)
		err(1, "Failed to change curr. dir (chdir).");
}

/* Executes the exit command = exits programx with *exval value.
 * *exval must be valid and cmd->name must be "exit".
 * */
static void
exec_exit(CmdSimple *cmd, int *exval) {
	assert(cmd);
	assert(exval);
	assert(strncmp("exit", cmd->name, 5) == 0);

	if (!STAILQ_EMPTY(&cmd->args))
		errx(1, "exit: too many arguments.");
	exit(*exval);
}

/* Replaces current process with the passed command and its arguments or exits
 * with error.
 * */
static void
exec_simple(char *cmd, CmdArgs *cmd_args) {
	assert(cmd);
	assert(cmd_args);

	int num_args = 0;
	CmdArg *arg;
	STAILQ_FOREACH(arg, cmd_args, tailq) { ++num_args; }
	/* Allocate an array for the new args + program's name + ending NULL. */
	char **args = malloc((num_args + 2) * sizeof *args);
	if (!args)
		err(1, "malloc");
	args[0] = cmd;
	args[num_args + 1] = NULL;
	int i = 0;
	STAILQ_FOREACH(arg, cmd_args, tailq) { args[++i] = arg->val; }
	assert(i == num_args);

	execvp(cmd, args);
	err(127, "%s", cmd);
}

/* Replaces standard IO with IOs in io argument if there are any.
 * The argument itself must be valid(!=NULL).
 * */
static void
set_IO(const CmdIO *io) {
	assert(io);

	if (io->in) {
		int in_fd;
		if ((in_fd = open(io->in, O_RDONLY)) == -1)
			err(1, "Cannot open \"%s\". (open)", io->in);
		if (dup2(in_fd, STDIN_FILENO) == -1)
			err(1, "Cannot redirect command's input. (dup2)");
		close(in_fd);
	}
	if (io->out) {
		int out_fd;
		int flags = O_WRONLY | O_CREAT;
		flags |= io->app ? O_APPEND : O_TRUNC;
		if ((out_fd = open(io->out, flags, 0664)) == -1)
			err(1, "Cannot open \"%s\". (open)", io->out);
		if (dup2(out_fd, STDOUT_FILENO) == -1)
			err(1, "Cannot redirect command's output. (dup2)");
		close(out_fd);
	}
}

/* Accepts exstatus received from wait() call and assign correct exit value to
 * the passed exval pointer. exval must be valid.
 * */
static void
child_exited(int exstatus, int *exval) {
	assert(exval);

	if (WIFEXITED(exstatus))
		*exval = WEXITSTATUS(exstatus);
	else if (WIFSIGNALED(exstatus)) {
		int sig = WTERMSIG(exstatus);
		*exval = 128 + sig;
		dprintf(STDERR_FILENO, "Killed by signal %d.\n", sig);
	}
}

/* Executes one simple command, puts its return value( if any ) into *exval.
 * Both pointers must be valid. The command is executed as child process
 * and the function waits for it.
 * */
static void
exec_one(CmdSimple *cmd, int *exval) {
	assert(exval);
	assert(cmd);

	if (strncmp("exit", cmd->name, 5) == 0)
		exec_exit(cmd, exval);
	else if (strncmp("cd", cmd->name, 3) == 0) {
		exec_cd(cmd);
		*exval = 0;
	} else {
		int childID;
		switch (childID = fork()) {
		case -1:
			err(1, "fork");
		case 0: /* Child */
			set_IO(&cmd->io);
			exec_simple(cmd->name, &cmd->args);
			break;
		default:
			break;
		}
		int exstatus;
		struct sigaction old_act;
		int wstatus;
		block_SIGINT(&old_act);
		/* Wait for the child to exit. If the user interrupted, pass it
		 * to the child. Repeat that until the child dies
		 * NOTE: from my testing the C-c sends signal both to the parent and
		 * the child even if the child has stdin redirected. So its probably
		 * sent to whole process group. Not sure how reliable it is, so I'll
		 * foward it to the child just to be sure.
		 * */
		while ((wstatus = wait(&exstatus)) == -1 && errno == EINTR)
			if (kill(childID, SIGINT) == -1 && errno != ESRCH)
				err(1, "kill");
		set_SIGINT(&old_act);

		if (wstatus == -1)
			err(1, "wait");
		else
			child_exited(exstatus, exval);
	}
}

/* Replaces standard IO with passed file descriptors from a pipe.
 * arg==0 means no replacement.
 * */
static void
set_pipe_IO(int read_end, int write_end) {
	if (read_end != -1) {
		if (dup2(read_end, STDIN_FILENO) == -1)
			err(1, "dup2");
		close(read_end);
	}
	if (write_end != -1) {
		if (dup2(write_end, STDOUT_FILENO) == -1)
			err(1, "dup2");
		close(write_end);
	}
}

/* Whether SIGINT has been triggered during execution of a pipe command. */
static bool pipe_interrupted = false;

static void
pipe_SIGINT_handler(int signum) {
	(void)signum;
	pipe_interrupted = true;
}

static void
pipe_set_SIGINT(struct sigaction *old_act) {
	struct sigaction act;
	act.sa_handler = &pipe_SIGINT_handler;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	if (sigaction(SIGINT, &act, old_act) == -1)
		err(1, "Cannot set pipe's  SIGINT(sigaction).");
}

static void
pipe_clear_SIGINT(struct sigaction *old_act) {
	if (sigaction(SIGINT, old_act, NULL) == -1)
		err(1, "Cannot restore SIGINT(sigaction).");
}

/* Sends kill command to all children in passed array. */
static void
kill_children(int *child_pids, int num_children) {
	assert(child_pids);

	for (int i = 0; i < num_children; ++i)
		kill(child_pids[i], SIGINT);
}

/* Closes pipe descriptors if they are not -1. */
static void
close_pipe(int pipe[2]) {
	if (pipe[0] != -1)
		while (close(pipe[0]) == -1 && errno == EINTR)
			;
	if (pipe[1] != -1)
		while (close(pipe[1]) == -1 && errno == EINTR)
			;
}

/* Executes piped command = creates child processes, pipes them together
 * and waits for them. *exval is return value of the last process in the pipe.
 * */
static void
exec_pipe(PipeCmd *cmd, int *exval) {
	assert(cmd);
	assert(exval);

	CmdSimple *c;
	int num_cmds = 0;
	STAILQ_FOREACH(c, &cmd->cmds, tailq) { ++num_cmds; }
	int *child_pids = (int *)malloc(num_cmds * sizeof *child_pids);
	if (!child_pids)
		err(1, "malloc");

	int lpipe[2] = {-1, -1};
	int rpipe[2] = {-1, -1};

	pipe_interrupted = false;
	struct sigaction old_act;
	pipe_set_SIGINT(&old_act);

	int cmds_started = 0;
	STAILQ_FOREACH(c, &cmd->cmds, tailq) {
		/* Create another pipe if it's not the last cmd.  */
		if (STAILQ_NEXT(c, tailq) != NULL && pipe(rpipe) == -1)
			err(1, "pipe");

		switch (child_pids[cmds_started] = fork()) {
		case -1:
			if (errno != EINTR)
				err(1, "Failed to create a child process.(fork)");
			break;
		case 0: /* Child */
			pipe_clear_SIGINT(&old_act);
			set_pipe_IO(lpipe[0], rpipe[1]);
			/* Close still open ends from the parent. */
			int pipe[2] = {lpipe[1], rpipe[0]};
			close_pipe(pipe);

			set_IO(&c->io);
			exec_simple(c->name, &c->args);
			break;
		default:
			/* Only increment on successful fork.
			 * Otherwise there was EINTR which is handled below and needs
			 * correct number of actually created commands. */
			++cmds_started;
			break;
		}
		close_pipe(lpipe);
		lpipe[0] = rpipe[0];
		lpipe[1] = rpipe[1];
		rpipe[0] = -1;
		rpipe[1] = -1;

		if (pipe_interrupted) {
			close_pipe(lpipe);
			/* Send first wave of signals. */
			kill_children(child_pids, cmds_started);
			break;
		}
	}
	int exstatus, wstatus;
	/* Will hold return value of the pipe if it finishes peacefully. */
	int last_exstatus = 0;
	/* Assert-only variable to ensure the last child exited. */
	bool exstatus_set = false;
	/* Wait for all the children. */
	while (true) {
		wstatus = wait(&exstatus);
		if (wstatus == -1) {
			if (errno == EINTR)
				kill_children(child_pids, cmds_started);
			else if (errno == ECHILD)
				break;
			else
				err(1, "wait");
		}
		/* Last cmd exited.*/
		else if (cmds_started == num_cmds &&
				 wstatus == child_pids[num_cmds - 1]) {
			exstatus_set = true;
			last_exstatus = exstatus;
		}
	}
	pipe_clear_SIGINT(&old_act);
	/* Last child must have exited if all the children did. */
	assert(exstatus_set);
	child_exited(last_exstatus, exval);
	free(child_pids);
}

/* Executes a command either as exec_one or exec_pipe,
 * Both pointers must be valid and cmd must contain atleast one cmd.
 * */
static void
exec_cmd(PipeCmd *cmd, int *exval) {
	assert(cmd);
	assert(exval);
	assert(!STAILQ_EMPTY(&cmd->cmds));

	CmdSimple *first = STAILQ_FIRST(&cmd->cmds);
	assert(first);
	if (STAILQ_NEXT(first, tailq) == NULL) /* Only one cmd */
		exec_one(first, exval);
	else {
		exec_pipe(cmd, exval);
	}
}

void
exec_cmds(Cmds *cmds, int *exval) {
	assert(cmds);
	assert(exval);

	PipeCmd *cmd;
	STAILQ_FOREACH(cmd, cmds, tailq) { exec_cmd(cmd, exval); }
}
