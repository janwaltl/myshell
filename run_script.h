#ifndef MYSHELL_RUN_SCRIPT_HEADER
#define MYSHELL_RUN_SCRIPT_HEADER

/*
 * Executes commands in the passed file.
 * Exits if the file cannot be opened or read.
 * Returns exit value of the last command executed.
 * */
int
run_script(const char *file);

#endif /* ifndef MYSHELL_RUN_SCRIPT_HEADER */
