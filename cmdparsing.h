#ifndef MYSHELL_CMD_PARSING_HEADER
#define MYSHELL_CMD_PARSING_HEADER

#include "cmdhiearchy.h"

/* Parses passed command line into commands.
 * Returns Commands on success, NULL on syntax error.
 * In that case 'err_msg' is set to an error message which the caller must
 * deallocate. Otherwise its unchanged.
 * */
Cmds *
parse_line(const char *line, char **err_msg);
#endif /* ifndef MYSHELL_CMD_PARSING_HEADER */
