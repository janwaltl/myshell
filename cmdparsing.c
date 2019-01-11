#include "cmdparsing.h"

#include <assert.h>
#include <stdio.h>

/* Parser must be include before lexer. */
#include "cmdparser.h"
#include "cmdlexer.h"

Cmds *
parse_line(const char *line, char **err_msg) {
	assert(line);
	assert(err_msg);

	yyscan_t scanner;
	yylex_init(&scanner);
	YY_BUFFER_STATE state = yy_scan_string(line, scanner);
	Cmds *cmds = NULL;
	int parsing_status = yyparse(scanner, &cmds, err_msg);
	yy_delete_buffer(state, scanner);
	yylex_destroy(scanner);
	if (parsing_status == 1) {
		cmd_free_cmds(cmds);
		return NULL;
	} else
		return cmds;
}
