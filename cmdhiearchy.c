#include "cmdhiearchy.h"

#include <assert.h>
#include <err.h>
#include <stdlib.h>
#include <stdio.h>

CmdIO
cmd_gen_IO() {
	CmdIO io = {NULL, NULL, false};
	return io;
}

void
cmd_add_IOs(CmdIO *from, CmdIO *to) {
	assert(from);
	assert(to);

	if (from->in != NULL) {
		free(to->in);
		to->in = from->in;
	}
	if (from->out != NULL) {
		free(to->out);
		to->out = from->out;
		to->app = from->app;
	}
}

CmdSimple *
cmd_alloc_simple(char *name, CmdIO io) {
	assert(name);

	CmdSimple *cmd = (CmdSimple *)malloc(sizeof *cmd);
	if (!cmd)
		err(1, "malloc");
	cmd->name = name;
	STAILQ_INIT(&cmd->args);
	cmd->io = io;
	return cmd;
}

void
cmd_free_simple(CmdSimple *cmd) {
	if (!cmd)
		return;
	/* Frees the list of arguments. */
	CmdArg *arg = STAILQ_FIRST(&cmd->args);
	while (arg != NULL) {
		CmdArg *tmp = STAILQ_NEXT(arg, tailq);
		cmd_free_arg(arg);
		arg = tmp;
	}

	free((char *)cmd->name);
	free(cmd->io.in);
	free(cmd->io.out);
	free(cmd);
}

CmdArg *
cmd_alloc_arg(char *arg_val) {
	assert(arg_val);
	CmdArg *arg = malloc(sizeof *arg);
	if (!arg)
		err(1, "malloc");
	arg->val = arg_val;
	return arg;
}

void
cmd_free_arg(CmdArg *arg) {
	if (!arg)
		return;
	free(arg->val);
	free(arg);
}

PipeCmd *
cmd_alloc_pipe(CmdSimple *firstCmd) {
	assert(firstCmd);

	PipeCmd *cmd = malloc(sizeof *cmd);
	if (!cmd)
		err(1, "malloc");
	STAILQ_INIT(&cmd->cmds);
	STAILQ_INSERT_TAIL(&cmd->cmds, firstCmd, tailq);
	return cmd;
}

void
cmd_free_pipe(PipeCmd *cmd) {
	if (!cmd)
		return;
	CmdSimple *simple, *next;
	simple = STAILQ_FIRST(&cmd->cmds);
	while (simple != NULL) {
		next = STAILQ_NEXT(simple, tailq);
		cmd_free_simple(simple);
		simple = next;
	}
	free(cmd);
}

Cmds *
cmd_alloc_cmds() {
	Cmds *cmds = malloc(sizeof *cmds);
	if (!cmds)
		errx(1, "malloc");
	STAILQ_INIT(cmds);
	return cmds;
}

void
cmd_free_cmds(Cmds *cmds) {
	if (!cmds)
		return;
	PipeCmd *cmd, *next;
	cmd = STAILQ_FIRST(cmds);
	while (cmd != NULL) {
		next = STAILQ_NEXT(cmd, tailq);
		cmd_free_pipe(cmd);
		cmd = next;
	}
	free(cmds);
}
