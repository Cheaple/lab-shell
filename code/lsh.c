/*
 * Main source code file for lsh shell program
 *
 * You are free to add functions to this file.
 * If you want to add functions in a separate file(s)
 * you will need to modify the CMakeLists.txt to compile
 * your additional file(s).
 *
 * Add appropriate comments in your code to make it
 * easier for us while grading your assignment.
 *
 * Using assert statements in your code is a great way to catch errors early and make debugging
 * easier. Think of them as mini self-checks that ensure your program behaves as expected. By
 * setting up these guardrails, you're creating a more robust and maintainable solution. So go
 * ahead, sprinkle some asserts in your code; they're your friends in disguise!
 *
 * All the best!
 */
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

// The <unistd.h> header is your gateway to the OS's process management facilities.
#include <unistd.h>

#include "parse.h"
#include "lsh.h"

static void run_cmds(Command *);
static void print_cmd(Command *cmd);
static void print_pgm(Pgm *p);
void stripwhite(char *);

pipe_node *pipe_list = NULL;

int main(void) {
    for (;;) {
        char *line;
        line = readline("> ");

        // If EOF encountered, exit shell
        if (!line) {
            // break;
        }

        // Remove leading and trailing whitespace from the line
        stripwhite(line);

        // If stripped line not blank
        if (*line) {
            add_history(line);

            Command cmd;
            if (parse(line, &cmd) == 1) {
                // print_cmd(&cmd);
                run_cmds(&cmd);
            } else {
                printf("Parse ERROR\n");
            }
        }

        // Clear memory
        free(line);
        // usleep(100000);
    }

    return 0;
}

void cmd_exit() {
    // Todo: collect zombie processes
    exit(0);
}

void cmd_cd(char *dir) {
    int err;
    if (dir == NULL)
        err = chdir(getenv("HOME"));
    else
        err = chdir(dir);
    if (err == -1)
        fprintf(stderr, "cd: no such file or directory: %s\n", dir);
}

/* Execute the given command(s).

 * Note: The function currently only prints the command(s).
 *
 * TODO:
 * 1. Implement this function so that it executes the given command(s).
 * 2. Remove the debug printing before the final submission.
 */
void run_cmds(Command *cmd_list) {
    // print_cmd(cmd_list);
    Pgm *cmd = cmd_list->pgm;
    pid_list *pdl_current = NULL;
    pid_t foreground_id = -1;

    // File descriptor for input and output redirectory destination
    int input_file_fd = STDIN_FILENO;
    int output_file_fd = STDOUT_FILENO;

    int left_fd[2] = {0, 0}, right_fd[2] = {0, 0};

    // I/O Redirection
    if (cmd_list->rstdin) {
        input_file_fd = open(cmd_list->rstdin, O_RDONLY);
        if (input_file_fd == -1) {
            fprintf(stderr, "Could not read file %s\n", cmd_list->rstdin);
            exit(EXIT_FAILURE);
        }
    }
    if (cmd_list->rstdout) {
        output_file_fd = open(cmd_list->rstdout, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (output_file_fd == -1) {
            fprintf(stderr, "Could not write file %s\n", cmd_list->rstdout);
            exit(EXIT_FAILURE);
        }
    }

    // Iterate execution, fork for all single command and connect the intput/output via pipes.
    while (cmd != NULL) {
        // Built-in commands: exit, cd
        if (strcmp(cmd->pgmlist[0], "exit") == 0)
            cmd_exit();
        if (strcmp(cmd->pgmlist[0], "cd") == 0) {
            cmd_cd(cmd->pgmlist[1]);
            cmd = cmd->next;
            continue;
        }

        right_fd[0] = left_fd[0];
        right_fd[1] = left_fd[1];
        left_fd[0] = 0;
        left_fd[1] = 0;

        if (cmd->next != NULL) {
            if (pipe(left_fd) == -1) {
                fprintf(stderr, "Pipe construction failed!\n");
                exit(EXIT_FAILURE);
            }
        }

        pid_t pid = fork();
        if (pid == -1) {
            fprintf(stderr, "Child process fork failed!\n");
            exit(EXIT_FAILURE);
            break;
        } else if (pid == 0) { // Child Process
            if (cmd_list->background)
                setpgid(getpid(), getpid());

            if (left_fd[0] != 0 || left_fd[1] != 0) {
                close(left_fd[1]);
                if (dup2(left_fd[0], STDIN_FILENO) == -1) {
                    fprintf(stderr, "Pipe read end failed for process %d\n", getpid());
                    exit(EXIT_FAILURE);
                }
            }
            if (right_fd[1] != 0 || right_fd[0] != 0) {
                close(right_fd[0]);
                if (dup2(right_fd[1], STDOUT_FILENO) == -1) {
                    fprintf(stderr, "Pipe write end failed for process %d\n", getpid());
                    exit(EXIT_FAILURE);
                }
            }

            // Input Redirection for the first command
            if (input_file_fd != 0 && cmd->next == NULL)
                dup2(input_file_fd, STDIN_FILENO);

            // Outout Redirection for the last command
            if (output_file_fd != 0 && cmd == cmd_list->pgm)
                dup2(output_file_fd, STDOUT_FILENO);

            // Execute a single command
            execvp(cmd->pgmlist[0], cmd->pgmlist);
            fprintf(stderr, "Command not found: %s\n", cmd->pgmlist[0]);
            // exit(EXIT_FAILURE);
        } else { // Parent Process
            if (foreground_id == -1) {
                foreground_id = pid; // Record the last foreground command' pid
            }
            if (right_fd[0] != 0 || right_fd[1] != 0) {
                close(right_fd[0]);
                close(right_fd[1]);
            }
            if (cmd->next == NULL && !cmd_list->background)
                waitpid(foreground_id, NULL, 0); // Wait for the foreground process to finish

            cmd = cmd->next;
        }
    }
}

/*
 * Print a Command structure as returned by parse on stdout.
 *
 * Helper function, no need to change. Might be useful to study as inpsiration.
 */
static void print_cmd(Command *cmd_list) {
    printf("------------------------------\n");
    printf("Parse OK\n");
    printf("stdin:      %s\n", cmd_list->rstdin ? cmd_list->rstdin : "<none>");
    printf("stdout:     %s\n", cmd_list->rstdout ? cmd_list->rstdout : "<none>");
    printf("background: %s\n", cmd_list->background ? "true" : "false");
    printf("Pgms:\n");
    print_pgm(cmd_list->pgm);
    printf("------------------------------\n");
}

/* Print a (linked) list of Pgm:s.
 *
 * Helper function, no need to change. Might be useful to study as inpsiration.
 */
static void print_pgm(Pgm *p) {
    if (p == NULL) {
        return;
    } else {
        char **pl = p->pgmlist;

        /* The list is in reversed order so print
         * it reversed to get right
         */
        print_pgm(p->next);
        printf("            * [ ");
        while (*pl) {
            printf("%s ", *pl++);
        }
        printf("]\n");
    }
}

/* Strip whitespace from the start and end of a string.
 *
 * Helper function, no need to change.
 */
void stripwhite(char *string) {
    size_t i = 0;

    while (isspace(string[i])) {
        i++;
    }

    if (i) {
        memmove(string, string + i, strlen(string + i) + 1);
    }

    i = strlen(string) - 1;
    while (i > 0 && isspace(string[i])) {
        i--;
    }

    string[++i] = '\0';
}
