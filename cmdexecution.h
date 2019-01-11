#ifndef MYSHELL_COMMAND_EXECUTION_HEADER
#define MYSHELL_COMMAND_EXECUTION_HEADER

#include "cmdhiearchy.h"

/* Execute the list of commands and returns exit value of the last command
 * executed.
 * Both pointers must be valid.
 * */
void
exec_cmds(Cmds *cmds, int *exval);
#endif
