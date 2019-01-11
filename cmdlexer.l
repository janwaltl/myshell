%{
#include <err.h>
#include <stdio.h>
#include <string.h>

#include "cmdparser.h"
%}

%option outfile="cmdlexer.c" header-file="cmdlexer.h"
%option reentrant noyywrap never-interactive nounistd
%option bison-bridge
%option noinput
%option nounput


%%


#.*	;
[ \t]	;
\;	{ return TOK_SCOLON; }
\<	{ return TOK_IO_IN; }
>>	{ return TOK_IO_APP; }
>	{ return TOK_IO_OUT;}
\|	{ return TOK_PIPE; }
[a-zA-Z.\-_0-9/]+ { 
	char* str = malloc(strlen(yytext)+1);
	if(!str) 
		err(1,"malloc"); 
	strcpy(str,yytext);
	yylval->sval=str;
	 return TOK_STR; 
	}

%%
