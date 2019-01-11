%{
#include <err.h>
#include <stdio.h>

/* Parser must be included before lexel.*/
#include "cmdparser.h"
#include "cmdlexer.h"

/* Makes a copy of 'msg' and assigns it to 'err_msg', user must deallocate this
 * string. 
 * */
int yyerror(yyscan_t  scanner,Cmds** cmds,char **err_msg, const char *msg);
%}

%code requires {
#include "cmdhiearchy.h"
}

%pure-parser
%lex-param   { void* scanner }
%parse-param { void* scanner }
%parse-param { Cmds** cmds }
%parse-param { char** err_msg }
%output  "cmdparser.c"
%defines "cmdparser.h"
%define parse.error verbose

%union {
	char *sval;
	CmdSimple* simple;
	PipeCmd* cmd;
	Cmds* cmds;
	CmdIO io;
}

%token TOK_SCOLON ";"
%token TOK_IO_IN "<"
%token TOK_IO_OUT ">"
%token TOK_IO_APP ">>"
%token TOK_PIPE "|"
%token <sval> TOK_STR "string"

%type <io> maybeio
%type <simple> simplecmd
%type <cmd> cmd
%type <cmds> cmds
%start line

%destructor { free($$); } <sval>
%destructor { cmd_free_pipe($$); } <cmd>
%destructor { cmd_free_cmds($$); } <cmds>
%destructor { free($$.in); free($$.out); } <io>
%%

line:
	%empty 
	{
		*cmds=cmd_alloc_cmds();
	}
	|cmds 
	{ 
		*cmds=$1;
	}
	|cmds TOK_SCOLON 
	{ 
		*cmds=$1;
	}
	;
cmds:
	cmds TOK_SCOLON cmd 
	{
		STAILQ_INSERT_TAIL($1,$3,tailq);
		$$=$1;
	}
	|cmd 
	{
		Cmds* cmds=cmd_alloc_cmds();
		STAILQ_INSERT_TAIL(cmds,$1,tailq);
		$$=cmds;
	}
	;
cmd:
	cmd TOK_PIPE simplecmd 
	{
		PipeCmd* cmd=$1;
		STAILQ_INSERT_TAIL(&cmd->cmds,$3,tailq);
		$$=cmd;
	}
	|simplecmd  
	{
		$$=cmd_alloc_pipe($1);
	}
	;
simplecmd:
	simplecmd TOK_STR maybeio 	/* append cmd argument to $1, app io */
	{
		CmdSimple* cmd = $1;	
		
		cmd_add_IOs(& $3, &cmd->io);

		CmdArg* arg = cmd_alloc_arg($2);
		STAILQ_INSERT_TAIL(&cmd->args,arg,tailq);

		$$=cmd;
	}
	|maybeio TOK_STR maybeio 
	{
		cmd_add_IOs(& $1, &$3);
		$$=cmd_alloc_simple($2,$3);
	}
	;
maybeio:
	maybeio TOK_IO_IN TOK_STR 
	{
		free($1.in);
		$1.in=$3;
		$$=$1;
	}
	|maybeio TOK_IO_OUT TOK_STR 
	{
		free($1.out);
		$1.out=$3;
		$1.app=false;
		$$=$1;
	}
	|maybeio TOK_IO_APP TOK_STR 
	{
		free($1.out);
		$1.out=$3;
		$1.app=true;
		$$=$1;	
	}
	|%empty 			
	{
		$$ = cmd_gen_IO();
	}
	;
%%

int yyerror(void*  scanner,Cmds** cmds,char** err_msg, const char *msg)
{
	(void)cmds;
	(void)scanner;
	int err_len=strlen(msg);
	char* errstr=(char*)malloc(err_len+1);
	if(!errstr)
		err(1,"malloc");
	strcpy(errstr,msg);
	*err_msg=errstr;
	return 2;
}
