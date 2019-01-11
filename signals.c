#include "signals.h"

#include <assert.h>
#include <err.h>
#include <stdio.h>

static void
empty_handler(int signum) {
	(void)signum;
}

void
block_SIGINT(struct sigaction *old_act) {
	struct sigaction act;
	act.sa_handler = &empty_handler;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	if (sigaction(SIGINT, &act, old_act) == -1)
		err(1, "Cannot block SIGINT(sigaction).");
}

void
set_SIGINT(struct sigaction *act) {
	assert(act);

	if (sigaction(SIGINT, act, NULL) == -1)
		err(1, "Cannot restore SIGINT(sigaction).");
}
