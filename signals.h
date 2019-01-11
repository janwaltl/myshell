#ifndef MYSHELL_SIGNALS_HEADER
#define MYSHELL_SIGNALS_HEADER

#include <signal.h>

/* Blocks SIGINT(enables EINTR) and sets 'old_act' to old signal handler.
 * Exits on error.
 * */
void
block_SIGINT(struct sigaction *old_act);

/* Sets signal action as a signal handler for SIGINT.
 * Exits on error.
 * */
void
set_SIGINT(struct sigaction *act);
#endif /* ifndef MYSHELL_SIGNALS_HEADER */
