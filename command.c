#include "command.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define SAFEALLOC(call) \
    if (call == NULL) { \
        perror("alloc"); \
        return 1; \
    }

int
init_commands(Commands *com, size_t size)
{
    com->data = malloc(size*sizeof(*(com->data)));
    SAFEALLOC(com->data)
    com->size = size;
    com->top = -1;
    return 0;
}

void
final_commands(Commands *com)
{
    for (int i = 0; i <= com->top; ++i) {
        free(com->data[i].name);
        free(com->data[i].infile);
        free(com->data[i].outfile);
        int j = 0;
        while (com->data[i].arg[j] != NULL) {
            free(com->data[i].arg[j++]);
        }
        free(com->data[i].arg);
    }
    free(com->data);
    com->size = 0;
    com->top = -1;
    return;
}

int
add_elem(Commands *com, Command newelem)
{
    ++(com->top);
    if (com->top == com->size) {
        com->data = realloc(com->data, 2*(com->size)*sizeof(*(com->data)));
        SAFEALLOC(com->data);
        com->size *= 2;
    }
    com->data[com->top] = newelem;
    return 0;
}

int
add_com_name(Commands *com, char *name)
{
    Command newelem;
    newelem.name = name;
    newelem.infile = NULL;
    newelem.outfile = NULL;
    newelem.arg = malloc(2 * sizeof(*newelem.arg));
    SAFEALLOC(newelem.arg);
    newelem.arg[0] = malloc((strlen(name) + 1) * sizeof(*name));
    strcpy(newelem.arg[0], name);
    newelem.arg[1] = NULL;
    newelem.operation = ';';
    return add_elem(com, newelem);
}

int
add_file(Commands *com, char *filename, char c)
{

    if (c == '<') {
        free(com->data[com->top].infile);
        com->data[com->top].infile = filename;
    }
    else {
        free(com->data[com->top].outfile);
        com->data[com->top].outfile = filename;
        com->data[com->top].outmode = c;
    }
    return 0;
}

int
add_bracket(Commands *com, char c)
{
    Command newelem;
    newelem.name = malloc(2 * sizeof(*newelem.name));
    SAFEALLOC(newelem.name);
    *newelem.name = c;
    *(newelem.name + 1) = '\0';
    newelem.infile = NULL;
    newelem.outfile = NULL;
    newelem.arg = malloc(sizeof(*newelem.arg));
    SAFEALLOC(newelem.arg);
    *newelem.arg = NULL;
    newelem.operation = ' ';
    if (c == ')') {
        newelem.operation = ';';
    }
    return add_elem(com, newelem);
}

int
add_com(Commands *com, char c)
{
    com->data[com->top].operation = c;
    return 0;
}

int
add_arg(Commands *com, char *name)
{
    int len = 0;
    while (com->data[com->top].arg[len++] != NULL);
    com->data[com->top].arg = realloc(com->data[com->top].arg, (len + 1) * sizeof(*com->data[com->top].arg));
    SAFEALLOC(com->data[com->top].arg);
    com->data[com->top].arg[len - 1] = name;
    com->data[com->top].arg[len] = NULL;
    return 0;
}

#define SAFEOPEN(call) \
    if ((f = call) == -1) { \
        perror("open"); \
        exit(1); \
    } \
    
#define SAFECLOSE(call) \
    if (call == -1) { \
        perror("close"); \
        exit(1); \
    } \


#define BALANCE(name, bal) \
    if (!strcmp(name, "(")) { \
        ++bal; \
    } \
    if (!strcmp(name, ")")) { \
        --bal; \
    }

int
calculate_commands(const Commands *com, int begin, int end)
{
    pid_t pid;
    int status;
    int input = 0;
    int output = 1;
    int j = 0;
    int f = -1;
    int prev = 0;
    int prevfd[2], fd[2];
    for (int i = begin; i <= end; ++i) {
        int flag = 0;
        if (!strcmp(com->data[i].name, "(")) {
            flag = 1;
            int bal = 1;
            j = i;
            while (++i <= com->top) {
                BALANCE(com->data[i].name, bal)
                if (!strcmp(com->data[i].name, ")") && bal == 0) {
                    break;
                }
            }
        }
        if (com->data[i].operation == '|') {
            if (pipe(fd) == -1) {
                perror("pipe");
                return 1;
            }
            input = dup(1);
            dup2(fd[1], 1);
        }
        if (prev) {
            output = dup(0);
            if (com->data[i].operation == '|') {
                dup2(prevfd[0], 0);
            }
            else {
                dup2(fd[0], 0);
            }
        }
        if ((pid = fork()) == 0) {
            if (com->data[i].outfile != NULL) {
                if (com->data[i].outmode == '>') {
                    SAFEOPEN(open(com->data[i].outfile, O_WRONLY | O_CREAT | O_TRUNC, 0666));
                }
                else if (com->data[i].outmode == '^') {
                    SAFEOPEN(open(com->data[i].outfile, O_WRONLY | O_CREAT | O_APPEND, 0666));
                }
                dup2(f, 1);
                SAFECLOSE(close(f));
            }
            if (com->data[i].infile != NULL) {
                SAFEOPEN(open(com->data[i].infile, O_RDONLY));
                dup2(f, 0);
                SAFECLOSE(close(f));
            }
            if (flag) {
                exit(calculate_commands(com, j + 1, i - 1));
            }
            else {
                execvp(com->data[i].name, com->data[i].arg);
                perror(com->data[i].name);
                exit(1);
            }
        }
        if (prev) {
            dup2(output, 0);
            if (com->data[i].operation == '|') {
                close(prevfd[0]);
            }
            else {
                close(fd[0]);
            }
        }
        if (com->data[i].operation == '|') {
            dup2(input, 1);
            close(fd[1]);
            prevfd[0] = fd[0];
            prevfd[1] = fd[1];
            prev = 1;
        }
        if (com->data[i].operation == ';' || com->data[i].operation == '&') {
            prev = 0;
            waitpid(pid, &status, 0);
            while (wait(NULL) != -1);
            if (com->data[i].operation == '&' && !(WIFEXITED(status) && !WEXITSTATUS(status))) {
                int bal = 0;
                while (++i <= com->top) {
                    BALANCE(com->data[i].name, bal)
                    if (com->data[i].operation == ';' && bal == 0) {
                        break;
                    }
                }
            }
        }
    }
    return !(WIFEXITED(status) && !WEXITSTATUS(status));
}

