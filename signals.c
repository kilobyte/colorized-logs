#include <signal.h>
#include <sys/wait.h>
#include <stdio.h>

static const char* sigobits[NSIG]=
{
    [SIGHUP]    = "Hangup",
    [SIGINT]    = "Interrupt",
    [SIGQUIT]   = "Quit",
    [SIGILL]    = "Illegal instruction",
    [SIGTRAP]   = "Breakpoint trap",
    [SIGABRT]   = "Aborted",
    [SIGBUS]    = "Bus error",
    [SIGFPE]    = "Floating point exception",
    [SIGKILL]   = "Killed",
    [SIGUSR1]   = "User signal 1",
    [SIGSEGV]   = "Segmentation fault",
    [SIGUSR2]   = "User signal 2",
    [SIGPIPE]   = "Broken pipe",
    [SIGALRM]   = "Alarm clock",
    [SIGTERM]   = "Terminated",
#ifdef SIGSTKFLT
    [SIGSTKFLT] = "Stack fault on coprocessor",
#endif
    [SIGCHLD]   = "Child died",
    [SIGCONT]   = "Continue",
    [SIGSTOP]   = "Stopped",
    [SIGTSTP]   = "Stop request on terminal",
    [SIGTTIN]   = "Stopped on tty input",
    [SIGTTOU]   = "Stopped on tty output",
    [SIGURG]    = "Urgent condition on socket",
    [SIGXCPU]   = "CPU limit exceeded",
    [SIGXFSZ]   = "File size limit exceeded",
    [SIGVTALRM] = "Virtual alarm clock",
    [SIGPROF]   = "Profiler timer expired",
    [SIGWINCH]  = "Window size changed",
    [SIGIO]     = "I/O ready",
#ifdef SIGPWR
    [SIGPWR]    = "Power loss",
#endif
    [SIGSYS]    = "Bad system call",
};

void sigobit(int ret)
{
    int core = WCOREDUMP(ret);
    int s = WTERMSIG(ret);
    if (s>0 && s<NSIG && sigobits[s])
        fprintf(stderr, "%s%s\n", sigobits[s], core?" (core dumped)":"");
    else
        fprintf(stderr, "Died to signal %d%s\n", s, core?" (core dumped)":"");
}
