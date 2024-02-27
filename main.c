#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include "command.h"

#define SPACE(ptr) \
    while (*ptr == ' ') { \
        ++(ptr); \
    }

char*
read_name(char **ptr)
{
    char* name = malloc(sizeof(*name));
    int len = 0;
    SPACE(*ptr);
    while ((**ptr != '\0') && (**ptr != ' ') && (**ptr != '&') && (**ptr != '|') && (**ptr != ';')
                           && (**ptr != '>') && (**ptr != '<') && (**ptr != '(') && (**ptr != ')')) {
        name = realloc(name, (len + 1)*sizeof(*name));
        name[len++] = **ptr;
        ++(*ptr);
    }
    SPACE(*ptr);
    if (len > 0) {
        name = realloc(name, (len + 1)*sizeof(*name));
        name[len++] = '\0';
        return name;
    }
    else {
        free(name);
        return NULL;
    }
}

#define ERROR(c) \
    printf("syntax error near unexpected token %c\n", c); \
    return 1;

int
parsing(Commands *com, int *mode, char *ptr) 
{
    while (1) {
        SPACE(ptr);
        if (*ptr == '\0') {
            add_com(com, ';');
            return 0;
        }
        while (*ptr == '(') {
            add_bracket(com, *(ptr++));
            SPACE(ptr);
        }
        char *name = read_name(&ptr);
        if (name != NULL) {
            add_com_name(com, name);
        }
        else {
            ERROR(*ptr);
        }
        while ((*ptr != '\0') && (*ptr != '|') && (*ptr != '&') && (*ptr != ';') && (*ptr != '>')
                              && (*ptr != '<') && (*ptr != '(') && (*ptr != ')')) {
            char *arg_name = read_name(&ptr);
            if (name != NULL) {
                add_arg(com, arg_name);
            }
        }
        SPACE(ptr);
        while (*ptr == ')' || *ptr == '<' || *ptr == '>') {
            while (*ptr == ')') {
                add_bracket(com, *(ptr++));
                SPACE(ptr);
            }
            while (*ptr == '<' || *ptr == '>') {
                char c = *ptr;
                if ((*ptr == '>') && (*(ptr + 1) == '>')) {
                    ++ptr;
                    c = '^';
                }
                ++ptr;
                char *file = read_name(&ptr);
                if (file != NULL) {
                    add_file(com, file, c);
                }
                else {
                    ERROR(*ptr);
                }
            }   
        }
        switch (*ptr) {
            case '\0':
                add_com(com, ';');
                return 0;
            case '|':
            case ';':
                add_com(com, *ptr);
                break;
            case '&':
                if (*(ptr + 1) == '&') {
                    add_com(com, *(++ptr));
                }
                else {
                    *mode = 1;
                    add_com(com, ';');
                    return 0;
                }
                break;
            default:
                ERROR(*ptr);
        }
        ++ptr;
    }
    return 0;
}

char *
read_str(void)
{
    int c;
    char *str = malloc(sizeof(*str));
    if (str == NULL) {
        perror("str alloc");
        exit(1);
    }
    unsigned long len = 0;
    while ((c = getchar())) {
        str = realloc(str, (len + 1)*sizeof(*str));
        if (str == NULL) {
            free(str);
            perror("str alloc");
            exit(1);
        }
        if (c == EOF || c == '\n') {
            if (len == 0) {
                free(str);
                return NULL;
            }
            str[len++] = '\0';
            return str;
        }
        str[len++] = c;
    }
    return str;
}

int
main(void)
{
    Commands com;
    if (init_commands(&com, 5)) {
        exit(2);
    }
    int mode = 0;
    //printf(">> ");
    char* str = read_str();
    if (parsing(&com, &mode, str)) {
        free(str);
        final_commands(&com);
        exit(1);
    }
    if (!mode) {
        calculate_commands(&com, 0, com.top);
    }
    else {
        if (fork() == 0) {
            exit(calculate_commands(&com, 0, com.top));
        }
    }
    free(str);
    final_commands(&com);
    return 0;
}

