 /*
 * @file parse.c
 * @author Rafael Ferreira
 * @date Mar 2018
 * @brief Parse line to CMD
 */

#include "header.h"

/*
 * @brief Print the command list
 */
void print_command_list(CMD *root)
{
    CMD *temp;
    int i;

    for (temp = root; temp != NULL; temp = temp->next)
    {
        printf("-----NEW COMMAND-----\n");
        printf("Name = %s\n", temp->name);

        for (i = 0; temp->argv[i] != NULL; i++)
        {
            printf("argv[%d] = %s\n", i, temp->argv[i]);
        }

        if (temp->infile != NULL)
            printf("Infile = %s\n", temp->infile);
        if (temp->outfile != NULL)
            printf("Outfile = %s\n", temp->outfile);
        if (temp->errfile != NULL)
            printf("Errfile = %s\n", temp->errfile);
    }
}

/*
 * @brief Delete the command list
 */
void free_command_list(CMD *root)
{
    CMD *temp1, *temp2;
    int i;

    temp1 = root;

    while (temp1 != NULL)
    {

        free(temp1->name);

        for (i = 0; temp1->argv[i] != NULL; i++)
        {
            free(temp1->argv[i]);
            temp1->argv[i] = NULL;
        }

        if (temp1->infile != NULL)
            free(temp1->infile);
        if (temp1->outfile != NULL)
            free(temp1->outfile);
        if (temp1->errfile != NULL)
            free(temp1->errfile);

        // to avoid dangling pointers
        temp1->errfile = NULL;
        temp1->infile = NULL;
        temp1->name = NULL;
        temp1->outfile = NULL;

        temp2 = temp1;
        temp1 = temp2->next;
        free(temp2);
        temp2 = NULL;
    }
}

/*
 * @brief Create and initialize the node
 * @return new - empty node initialized to be filled in parse_line()
 */
struct command *insert_command()
{
    CMD *new;
    new = (CMD *)malloc(sizeof(CMD));
    if (new == NULL)
    {
        perror("malloc error!\n");
        exit(1);
    }

    //It is necessary to initialize the node, since we have no guarantees than the
    //function parse_line() will do it (the line of text may be empty ...)

    new->name = NULL;
    new->argv[0] = NULL;
    new->infile = NULL;
    new->outfile = NULL;
    new->errfile = NULL;
    new->next = NULL;

    return new;
}

/*
 * @brief parse line  to right spot in the structure CMD
 * @returno root - Structure CMD
 */
CMD *parse_line(char *line)
{
    char *a;
    int n, count = 0;
    char *name, *param, *infile, *outfile, *errfile;
    CMD *command, *root;

    root = insert_command(); // Let's install the first one on the list
    command = root;
    a = strtok(line, " \t\r\n");
    for (n = 0; a; n++)
    {
        if (command->argv[0] != NULL && (n != 0 && strcmp(command->argv[0], "echo") == 0) && a[0] != '|' && a[0] != '<' && a[0] != '>' && (a[0] != '2' && a[1] != '>'))
        {
            if (command->argv[1] == NULL)
            {
                command->argv[1] = strdup(a);
            }
            else
            {
               char *echo_string = malloc(strlen(command->argv[1]) + strlen(a) + 2);
               strcpy(echo_string, command->argv[1]);
               strcat(echo_string, " ");
               strcat(echo_string, a);
               command->argv[1] = strdup(echo_string);
            }
            count = 2;
        }
        if (a[0] == '|')
        {
            command->next = insert_command(); // We have a new command so let's insert a new node in the list
            command = command->next;
            count = 0;
        }
        else if (a[0] == '<')
        {
            infile = a[1] ? a + 1 : strtok(NULL, " \t\r\n");
            command->infile = strdup(infile);
        }
        else if (a[0] == '>')
        {
            outfile = a[1] ? a + 1 : strtok(NULL, " \t\r\n");
            command->outfile = strdup(outfile);
        }
        else if (a[0] == '2' && a[1] == '>')
        {
            errfile = a[2] ? a + 2 : strtok(NULL, " \t\r\n");
            command->errfile = strdup(errfile);
        }
        else
        {
            if (!count)
            { // count==0 implies that we have the command name and the value for argv[0]
                name = a;
                command->name = strdup(name);
                command->argv[count] = strdup(name);
                count++;
            }
            else
            { //count != 0 implies that we have all the arguments of the program
                param = a;
                if (count < MAXARGS - 1)
                { // To ensure that the arguments do not exceed the size of the vector...
                    command->argv[count] = strdup(param);
                    count++;
                }
            }
        }

        a = strtok(NULL, " \t\r\n");
    }
    command->argv[count] = NULL; // to mark the end of the vector
    return root;
}
