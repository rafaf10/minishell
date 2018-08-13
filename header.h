// LIBS
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <signal.h>
#include <memory.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <pwd.h>
#include <grp.h>
// MACROS
#define MAXARGS 10
#define N 5
#define CONSUMERS 2
#define VERMELHO  "\x1B[31m\e[1m"
#define VERDE  "\x1B[32m\e[1m"
#define AZUL  "\x1B[34m\e[1m"
#define CYAN  "\x1B[36m\e[1m"
#define BRANCO  "\e[97m\e[0m"

// STRUCTS
typedef struct command {
    char *name;
    char *argv[MAXARGS];
    char *infile;
    char *outfile;
    char *errfile;
    struct command *next;
} CMD;

// FUNCTIONS
CMD *insert_command();
void free_command_list();
void print_command_list();
void exec_comandos(CMD* root);
int n_commands(CMD *root);
void update_path();
CMD * parse_line(char *);
int parse_path();
void myexec(CMD* root);
void *insert_directories(void *pos);
char **character_name_completion(const char *, int, int);
char *character_name_generator(const char *, int);
void *list_dir(void *name);
void *produtor(void *name);
void *consumidor(void *name);

