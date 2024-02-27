#pragma once
#include <stddef.h>

typedef struct {
    char *name; 
    char *infile;
    char *outfile;
    char **arg;
    char outmode;
    char operation;
} Command;

typedef struct {
    Command *data;
    size_t size;
    size_t top; 
} Commands;

int init_commands(Commands *com, size_t size);
int add_elem(Commands *com, Command newelem);
int calculate_commands(const Commands *com, int begin, int end);
int add_com_name(Commands *com, char *name);
int add_file(Commands *com, char *filename, char c);
int add_bracket(Commands *com, char c);
int add_com(Commands *com, char c);
int add_arg(Commands *com, char *name);
void final_commands(Commands *com);

