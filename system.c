#include "system.h"

#include <errno.h>
#include <paths.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int system_fd_closexec(const char* command)
{
    int wait_val = 0;
    pid_t pid;

    if (command == NULL)
        return 1;

    if ((pid = vfork()) < 0)
        return -1;

    if (pid == 0) {
        int i = 0;
        int stdin_fd = fileno(stdin);
        int stdout_fd = fileno(stdout);
        int stderr_fd = fileno(stderr);
        long sc_open_max = sysconf(_SC_OPEN_MAX);
        if (sc_open_max < 0) {
            fprintf(stderr, "Warning, sc_open_max is unlimited!\n");
            sc_open_max = 20000; /* enough? */
        }
        /* close all descriptors in child sysconf(_SC_OPEN_MAX) */
        for (; i < sc_open_max; i++) {
            if (i == stdin_fd || i == stdout_fd || i == stderr_fd)
                continue;
            close(i);
        }

        execl(_PATH_BSHELL, "sh", "-c", command, (char*)0);
        fprintf(stderr,"%s, %d\n", __func__, __LINE__);
        _exit(127);
    }

    while (waitpid(pid, &wait_val, 0) < 0) {
        fprintf(stderr, "%s, errno: %d\n", __func__, errno);
        if (errno != EINTR) {
            wait_val = -1;
            break;
        }
    }

    return wait_val;
}

int runapp_result(char *cmd)
{
    char buffer[BUFSIZ] = {0};
    FILE *read_fp;
    int chars_read;
    int ret;

    read_fp = popen(cmd, "r");
    if (read_fp != NULL) {
        chars_read = fread(buffer, sizeof(char), BUFSIZ - 1, read_fp);
        if (chars_read > 0)
            ret = 1;
        else
            ret = -1;
        pclose(read_fp);
    } else {
        ret = -1;
    }

    return ret;
}
