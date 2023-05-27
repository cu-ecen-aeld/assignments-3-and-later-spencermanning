#include "systemcalls.h"
#include <stdlib.h> // for system()
#include "unistd.h" // for fork(), execv()
#include <sys/wait.h> // for wait()
#include <stdio.h> // for open()
#include <fcntl.h> // for O_WRONLY|O_CREAT flags
#include "syslog.h"
#include "errno.h"

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/
    int ret = system(cmd);
    printf("Command used: %s\n", cmd);
    if (ret) {
        printf("SYSTEM FAILED %i Errno: %d\n", ret, errno);
        return false;
    }
    else {
        printf("SYSTEM SUCCESS\n");
        return true;
    }
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/
bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait() instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/
    pid_t current_pid = 0;
    int pidstatus = 0;
    pid_t ended_child_pid = 0;

    for (int i = 0; i < count; i++) {
        printf("Command %d: %s\n", i, command[i]);
    }
    printf("\n");

    // Avoid duplicate printf()s
    fflush(stdout);

    // Make a copy of the current process
    current_pid = fork();

    // if in the child
    if (current_pid == 0) {
        printf("In child. PID: %i\n", current_pid);
        printf("In child. command[0]: %s, command[1]: %s\n", command[0], command[1]);

        // Replace the child process with command[0]
        int out = execv(command[0], command);
        // Should not get here, unless failed
        printf("Execv returned: %i\n", out);
        if (out == -1) {
            // printf("Execv failed. Could be good %i\n", errno);
            syslog(LOG_ERR, "FAILEDDDD\n");
            perror("Execv failedded. Could be good for test.\n");
            // exit(1);
            return false;
        }
        else if (out == 2) {
            perror("Execv failed with 2\n");
            return false;
        }
        else {
            printf("Execv success: %d\n", out);
            return true;
        }
    }
    else if (current_pid == -1) {
        printf("FORK FAILED\n");
        return false;
    }

    // if in parent
    if (current_pid > 0) {
        printf("In parent. PID: %i\n", current_pid);

        // Wait for any child process to end
        ended_child_pid = wait(&pidstatus);

        if (ended_child_pid == -1) {
            return false;
        }

        if (current_pid == ended_child_pid) {
            if (WIFEXITED(pidstatus)) {
                printf("Hit WIFEXTED?\n");
                printf("Exit status %i\n", WEXITSTATUS(pidstatus));
                return WEXITSTATUS(pidstatus) == 0;
            }
        // FIX THIS SECTION
            if (WIFSIGNALED(pidstatus)) {
                printf("Hit WIFSIGNALED?\n");
                return false;
            }
            if (WIFSTOPPED(pidstatus)) {
                printf("Hit WIFSTOPPED?\n");
                return false;
            }
            if (WIFCONTINUED(pidstatus)) {
                printf("Hit WIFCONTINUED?\n");
                return false;
            }
            else {
                printf("I guess WIFEXITED returned false\n");
                return true;
            }
        }
    }
    va_end(args);
    return false;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    // command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a reference,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/
    pid_t current_pid = 0;
    int pidstatus = 0;
    pid_t ended_child_pid = 0;

    int fd = open(outputfile, O_WRONLY|O_CREAT); // TODO Do I need a mode parameter?

    // Avoid duplicate printf()s
    fflush(stdout);

    current_pid = fork();

    // if in the child
    if (current_pid == 0) {
        printf("In child. PID: %i\n", current_pid);
        printf("In child. command[0]: %s, command[1]: %s\n", command[0], command[1]);

        if (dup2(fd, 1) < 0) {
            perror("dup2 failed\n");
            return false;
        }
        close(fd);

        // Replace the child process with command[0]
        int out = execv(command[0], command);
        // Should not get here, unless failed
        printf("Execv returned: %i\n", out);
        if (out == -1) {
            // printf("Execv failed. Could be good %i\n", errno);
            syslog(LOG_ERR, "FAILEDDDD\n");
            perror("Execv failedded. Could be good for test.\n");
            // exit(1);
            return false;
        }
        else if (out == 2) {
            perror("Execv failed with 2\n");
            return false;
        }
        else {
            printf("Execv success: %d\n", out);
            return true;
        }
    }
    else if (current_pid == -1) {
        printf("FORK FAILED\n");
        return false;
    }

    // if in parent
    if (current_pid > 0) {
        printf("In parent. PID: %i\n", current_pid);

        // Wait for any child process to end
        ended_child_pid = wait(&pidstatus);

        if (ended_child_pid == -1) {
            return false;
        }

        if (current_pid == ended_child_pid) {
            if (WIFEXITED(pidstatus) == true) {
                printf("Exit status %i\n", WEXITSTATUS(pidstatus));
                return !WEXITSTATUS(pidstatus);
            }
        // FIX THIS SECTION
            if (WIFSIGNALED(pidstatus)) {
                return false;
            }
            if (WIFSTOPPED(pidstatus)) {
                return false;
            }
            if (WIFCONTINUED(pidstatus)) {
                return false;
            }
            else {
                printf("I guess WIFEXITED returned false");
                return true;
            }
        }
    }
    close(fd);
    va_end(args);
    return true;
}
