#ifndef SYSTEM_H
#define SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Implement of run command in another process,
 * close all parent's fds compare to libc system()
 */
int system_fd_closexec(const char *command);
int runapp_result(char *cmd);

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_H
