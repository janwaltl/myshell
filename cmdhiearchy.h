#ifndef MYSHELL_CMDHIEARCHY_HEADER
#define MYSHELL_CMDHIEARCHY_HEADER

#include <stdbool.h>

#include <sys/queue.h>

/* Filenames to which redirect the input and output of a command.
 * App specifies whether the output should be appended or not.
 * */
typedef struct {
	char *in;
	char *out;
	bool app;
} CmdIO;

/* A command arguments. */
typedef struct CmdArg_tag {
	char *val;
	STAILQ_ENTRY(CmdArg_tag) tailq;
} CmdArg;

/* List of command arguments. */
STAILQ_HEAD(CmdArgs_tag, CmdArg_tag);
typedef struct CmdArgs_tag CmdArgs;

/* One command with its arguments. */
typedef struct CmdSimple_tag {
	char *name;
	CmdArgs args;
	CmdIO io;
	STAILQ_ENTRY(CmdSimple_tag) tailq;
} CmdSimple;

/* List of SimpleCmds piped together. */
STAILQ_HEAD(CmdPipedCmds_tag, CmdSimple_tag);
typedef struct CmdPipedCmds_tag CmdPipedCmds;

/* One piped command. The list might contain only one command. */
typedef struct PipeCmd_tag {
	CmdPipedCmds cmds;
	STAILQ_ENTRY(PipeCmd_tag) tailq;
} PipeCmd;

/* List of commands that were separated by semicolons. */
STAILQ_HEAD(Cmds_tag, PipeCmd_tag);
typedef struct Cmds_tag Cmds;

/* Returns initialized CmdIO structure that does not redirect any
 * inputs/outputs.
 * */
CmdIO
cmd_gen_IO();

/* Adds redirections from the first argument to the second argument.
 * Only adds non-NULL redirections.
 * Strings of the replaced redirections are deallocated.
 * */
void
cmd_add_IOs(CmdIO *from, CmdIO *to);

/* Generates CmdSimple with given name, IO and empty argument list. */
CmdSimple *
cmd_alloc_simple(char *name, CmdIO io);

/* Frees a CmdSimple command allocated from cmd_alloc_simple. */
void
cmd_free_simple(CmdSimple *cmd);

/*
 * Allocates and returns new CmdArg,
 * Claims argVal pointer and sets it as arg->val. If the user needs to work with
 * *arg_val it should make its own copy.
 * argVal must be heap-allocated because cmd_free_arg will free it.
 * */
CmdArg *
cmd_alloc_arg(char *arg_val);

/* Frees allocated CmdArg and its ->val. */
void
cmd_free_arg(CmdArg *arg);

/* Allocates a list of piped commands. */
PipeCmd *
cmd_alloc_pipe(CmdSimple *firstCmd);

/* Deallocates a pipe allocated from cmd_alloc_pipe. */
void
cmd_free_pipe(PipeCmd *cmd);

/* Allocates semicolon list of commands. */
Cmds *
cmd_alloc_cmds();

/* Frees commands allocated by cmd_alloc_cmds. */
void
cmd_free_cmds(Cmds *cmds);
#endif /* ifndef MYSHELL_CMDHIEARCHY_HEADER */
