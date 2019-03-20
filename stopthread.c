/*
 * Author: Chris Down <chris@chrisdown.name>
 *
 * A small program to stop individual threads from executing, instead of the
 * whole thread group.
 *
 * This is needed because SIGSTOP won't work on a single thread (even with
 * t(g)kill), because SIGSTOP and other job control-related signals only
 * operate on a whole thread group. PTRACE_SEIZE + PTRACE_INTERRUPT, while not
 * really designed for use in this way, are a satisfyingly hacky workaround.
 *
 * Use like so:
 *
 *     $ ./stopthread [list of tids]
 *
 * Press Ctrl-C or send another signal to make the threads in question start
 * executing again and return to their original state.
 *
 * WARNING:
 *
 * This can be used in emergencies to stem bleeding, however, be careful:
 *
 * 1. You must make sure that the threads you want to stop do not hold any
 *    synchronisation primitives when stopped. Verify this using tracing first.
 * 2. If running this on a service which understands admission control, you may
 *    need to bump admission control limits to account for the stopped threads,
 *    and allow new ones to execute.
 * 3. If running this on a service which understands I/O gating, you may need
 *    to bump I/O gating limits for the use cases the threads you're stopping
 *    are classified as, to avoid unintentionally causing throttling for those
 *    use cases.
 */

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>

int pid_t_from_charbuf(char *input, pid_t *output)
{
	char *buf;
	uintmax_t raw_pid;

	errno = 0;
	raw_pid = strtoumax(input, &buf, 10);

	if (errno > 0 || input == buf || *buf != '\0' ||
	    raw_pid != (uintmax_t)(pid_t)raw_pid)
		return -EINVAL;

	*output = (pid_t)raw_pid;

	return 0;
}

int stop_tid(pid_t tid)
{
	int status;

	if (ptrace(PTRACE_SEIZE, tid, NULL, NULL) != 0)
		return -errno;
	if (ptrace(PTRACE_INTERRUPT, tid, NULL, NULL) != 0)
		return -errno;
	if (wait4(tid, &status, __WALL, NULL) != tid)
		return -errno;

	assert(WIFSTOPPED(status));

	return 0;
}

int main(int argc, char *argv[])
{
	int i;

	for (i = 1; i < argc; i++) {
		int r;
		pid_t tid;

		r = pid_t_from_charbuf(argv[i], &tid);
		if (r < 0) {
			fprintf(stderr, "invalid tid '%s'\n", argv[i]);
			return EXIT_FAILURE;
		}

		r = stop_tid(tid);
		if (r < 0) {
			fprintf(stderr, "cannot trace stop %ju: %s\n",
				(uintmax_t)tid, strerror(abs(r)));
			return EXIT_FAILURE;
		}

		printf("Stopped TID %ju\n", (uintmax_t)tid);
	}

	printf("Waiting for signal...\n");

	/* processes are released back to their parents when we kick the bucket */
	pause();

	return EXIT_SUCCESS;
}
