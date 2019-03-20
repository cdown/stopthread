A small program to stop individual threads from executing, instead of the whole
thread group.

This is needed because SIGSTOP won't work on a single thread (even with
t(g)kill), because SIGSTOP and other job control-related signals only operate
on a whole thread group. PTRACE_SEIZE + PTRACE_INTERRUPT, while not really
designed for use in this way, are a satisfyingly hacky workaround.

Use like so:

    $ ./stopthread [list of tids]

Press Ctrl-C or send another signal to make the threads in question start
executing again and return to their original state.

**WARNING:**

This can be used in emergencies to stem bleeding, however, be careful:

1. You must make sure that the threads you want to stop do not hold any
   synchronisation primitives when stopped. Verify this using tracing first.
2. If running this on a service which understands admission control, you may
   need to bump admission control limits to account for the stopped threads,
   and allow new ones to execute.
3. If running this on a service which understands I/O gating, you may need to
   bump I/O gating limits for the use cases the threads you're stopping are
   classified as, to avoid unintentionally causing throttling for those use
   cases.
