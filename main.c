/**
 * @file main.c
 * @author Rafael Ferreira
 * @date Mar 2018
 * @brief Mini Unix Shell
 *
 * Code developed for academic purposes
 * 
 * 1 - For the execution of commands with arguments I used the calls to the system fork and the exec family with a dynamic system of pipes 
 *   - To work with files I used the system calls open and dup2
 * 2 - To implement the cd command I used getcwd and chdir
 * 3 - To ignore CTRL + C and + Z I used signals
 * 4 - Tab completion for system executables
 *   - Creates word library using threads and deep search 
 *   - second link for details
 * 5 - myls using threads and recursive search
 * 6 - myfind using threads (Producer/Consumer) and recursive search 
 * 
 * @see www.linkedin.com/in/rafaf10
 * @see https://robots.thoughtbot.com/tab-completion-in-gnu-readline
 * @see http://man7.org/linux/man-pages/man2/stat.2.html
 */

#include "header.h"
/* SIGNALS */
sigset_t block_mask;
/* PTHREAD */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexP = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexC = PTHREAD_MUTEX_INITIALIZER;
pthread_t *dynamic_threads = NULL;
pthread_t tidC[CONSUMERS]; // consumers threads
pthread_t tidP; 			// producer thread
/* SEM */
sem_t can_prod, // can produce
	  can_cons; // can consume
/* VARS */
char *path = NULL;	// store current path
char *line; 		// store input
char **directories = NULL; // store directories from $PATH
char **dictionary = NULL;  // store executable programs from directories
static int incremento_dicionario = 0;
char *string; // $PATH
char **myfind = NULL;
static int cnt = 0;
int prodptr = 0, consptr = 0, nItem = 0;


int main(int argc, const char *argv[])
{
    string = malloc((strlen(getenv("PATH")) + 1) * sizeof(char));
    strcpy(string, getenv("PATH"));
    update_path();

    /// Bloqueia CTRL+C e CTRL+Z
    struct sigaction sa, sa_orig_int, sa_orig_sigtstp;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = SA_SIGINFO;    
    sigaction(SIGINT, &sa, &sa_orig_int);
    sigaction(SIGTSTP, &sa, &sa_orig_sigtstp);
    
    /// tab completion
    rl_attempted_completion_function = character_name_completion;

    int sizePath = parse_path();
    pthread_t tid[sizePath];
    int index[sizePath], i = 0;

    for (i = 0; i < sizePath; i++)
    {
        index[i] = i;
        pthread_create(&tid[i], NULL, &insert_directories, &index[i]);
    }
    for (i = 0; i < sizePath; i++)
    {
        pthread_join(tid[i], NULL);
    }

    int my;
    while (1)
    {
        line = readline(path);
        if (strcmp(line, "") && strcmp(line, " "))
        {
            if (!strcmp(line, "exit"))
                exit(0);
            add_history(line);
            // parse input to root (CMD)
            CMD *root = parse_line(line);
            //print_command_list(root);
            my = strncmp(root->argv[0], "my", 2);
            if (my != 0)
                exec_comandos(root);
            else
            {
                myexec(root);
            }

            free_command_list(root);
            free(line);
        }
    }

    return 0;
}

/*
 * @brief count the number of commands in CMD
 * @return number of commands
 */
int n_commands(CMD *root)
{
    int n = 0;
    CMD *aux = root;
    while (aux != NULL)
    {
        n++;
        aux = aux->next;
    }
    return n;
}

/*
 * @brief update var path 
 */
void update_path()
{
    char pwd[2048];
    getcwd(pwd, sizeof(pwd));
    path = malloc((strlen(pwd) + 9) * sizeof(char));
    strcpy(path, "msh$ ~");
    strcat(path, pwd);
    strcat(path, "$ ");
}

/*
 * @brief store directories
 * @param void* pos - position in directories to use
 */
void *insert_directories(void *pos)
{
    int i = *((int *)pos);
    DIR *dir;
    struct dirent *entry;
    struct stat sb;

    if ((dir = opendir(directories[i])) == NULL)
        perror("opendir() error");
    else
    {
        char *result = NULL;
        while ((entry = readdir(dir)) != NULL)
        {
            int ponto = strcmp(entry->d_name, ".");
            int ponto2 = strcmp(entry->d_name, "..");

            if (ponto2 != 0 && ponto != 0)
            {
                result = malloc((strlen(directories[i]) + strlen(entry->d_name) + 2) * sizeof(char));
                strcpy(result, directories[i]);
                strcat(result, "/");
                strcat(result, entry->d_name);

                if (stat(result, &sb) == 0 && sb.st_mode & S_IXUSR)
                {
                    // CRITICAL AREA
                    pthread_mutex_lock(&mutex);
                    dictionary = realloc(dictionary, (incremento_dicionario + 1) * sizeof(char **));
                    dictionary[incremento_dicionario] = (char *)malloc((strlen(entry->d_name) + 1) * sizeof(char));
                    strcpy(dictionary[incremento_dicionario], entry->d_name);
                    incremento_dicionario++;
                    pthread_mutex_unlock(&mutex);
                    // END OF CRITICAL AREA
                }
            }
        }
    }
    closedir(dir);
}

/*
 * @brief calculate the number of directories in var PATH
 * @return number of directories
 */
int parse_path()
{
    char *a;
    int n = 0, j = 0;

    a = strtok(string, ":");
    for (n = 0; a; n++)
    {
        directories = realloc(directories, (n + 1) * sizeof(char **));
        directories[n] = (char *)malloc((strlen(a) + 1) * sizeof(char));
        strcpy(directories[n], a);
        //printf("\n\t %s", directories[n]);
        a = strtok(NULL, ":");
    }

    return n;
}

/*
 * @brief Point 1 and 2
 * 
 * 1 - count commands
 *   - create pipes
 *   - create processes
 * 		- work with previous pipe or stdin or files
 * 		- write (execute) to the pipe or stdout or files
 * 
 * 2 - check if first argument is cd
 * 	 - check if have destination
 * 	 - chande directory and update path var
 * 
 */
void exec_comandos(CMD *root)
{
    if (strcmp(root->argv[0], "cd") == 0)
    {
        if (root->argv[1] == NULL)
        {
            chdir("/");
        }
        else
        {
            chdir(root->argv[1]);
        }
        update_path();
        return;
    }
    int nComandos = n_commands(root);
    int i, fp, status = 0;
    int fds[2 * nComandos];
    CMD *aux = root;
    pid_t pid, wpid;

    // create n pipes
    for (i = 0; i < nComandos - 1; i++)
    {
        if (pipe(fds + i * 2) < 0)
        {
            perror("pipe(fds)");
            exit(1);
        }
    }
	// Execute
    i = 0;
    while (aux != NULL)
    {
        pid = fork();
        if (pid < 0)
        {
            perror("pid");
            exit(1);
        }
        if (pid == 0)
        {
            if (i != 0) // Read from the previous pipe (if it is not the first one)
            {
                dup2(fds[(i - 1) * 2], 0); // i - 1 (=) previous pipe (reading)
            }
            if (i != nComandos - 1) // Write to the pipe (if not the last one)
            {
                dup2(fds[i * 2 + 1], 1); // current pipe (written)
            }
            if (aux->infile != NULL)
            {
                fp = open(aux->infile, O_RDONLY, 0400); // owner read
                if (fp == -1)
                {
                    perror("File not existent");
                    exit(1);
                }
                dup2(fp, 0);
                close(fp);
            }
            if (aux->outfile != NULL)
            {
                fp = open(aux->outfile, O_WRONLY | O_TRUNC | O_CREAT, 0200); // owner write
                if (fp == -1)
                {
                    perror("Error creating file");
                    exit(1);
                }
                dup2(fp, 1);
                close(fp);
            }
            if (aux->errfile != NULL)
            {
                fp = open(aux->errfile, O_WRONLY | O_TRUNC | O_CREAT, 0200); // owner write
                if (fp == -1)
                {
                    perror("Error creating file");
                    exit(1);
                }
                dup2(fp, 2);
                close(fp);
            }
            execvp(aux->argv[0], aux->argv);
            perror("execvp");
            exit(1);
        }
        if (i != 0)
        {
            close(fds[((i - 1) * 2)]);
        }
        if (i != nComandos - 1)
        {
            close(fds[i * 2 + 1]);
        }
        aux = aux->next;
        i++;
    }
    while (wait(&status) > 0)
        ;
}

/*
 * @brief Point 5 and 6
 * 
 * 5 - check parameters (-a, -l, -al, -R)
 *   - check path
 *     - use path or get current path
 *   - open the directory and print
 * 6 - get current path
 * 	 - create consumers and producers 
 * 
 */
void myexec(CMD *root)
{
    if (strcmp(root->argv[0], "myls") == 0)
    {
        DIR *dir;
        struct dirent *entry;
        struct stat sb;
        char pwd[2024] = "\0";
        int param[2] = {0};
        CMD *aux = root;

        int i = 1, p = 0, a, l, al, r;
        // check parameters 
        while (aux->argv[i] != NULL)
        {
            a = strcmp(aux->argv[i], "-a");
            l = strcmp(aux->argv[i], "-l");
            al = strcmp(aux->argv[i], "-al");
            r = strcmp(aux->argv[i], "-R");
            if (l == 0) // -l
            {
                param[0] = 1;
            }
            else if (a == 0) // -a
            {
                param[1] = 1;
            }
            else if (al == 0) // -al
            {
                param[0] = 1;
                param[1] = 1;
            }
            else if (r == 0) // -R
            {
                param[2] = 1;
            }
            else
            {
                p++;
                strcpy(pwd, aux->argv[i]);
            }
            i++;
        }

        if (p == 0) // no path given
        {
            getcwd(pwd, sizeof(pwd));
        }

        char *aux_pwd = malloc((strlen(pwd) + 1) * sizeof(char));
        strcpy(aux_pwd, pwd);
        
        if ((dir = opendir(pwd)) == NULL)
            perror("opendir() error");
        else
        {
            char *result = NULL;
            while ((entry = readdir(dir)) != NULL)
            {

                result = malloc((strlen(pwd) + strlen(entry->d_name) + 2) * sizeof(char));
                strcpy(result, pwd);
                strcat(result, "/");
                strcat(result, entry->d_name);

                if (stat(result, &sb) == 0)
                {
                    if (param[0] == 1 && param[1] == 1)
                    { // -al
                        struct passwd *pw = getpwuid(sb.st_uid);
                        struct group *gr = getgrgid(sb.st_gid);

                        printf((sb.st_mode & S_IFDIR) ? "d" : "-");
                        printf((sb.st_mode & S_IRUSR) ? "r" : "-");
                        printf((sb.st_mode & S_IWUSR) ? "w" : "-");
                        printf((sb.st_mode & S_IXUSR) ? "x" : "-");
                        printf((sb.st_mode & S_IRGRP) ? "r" : "-");
                        printf((sb.st_mode & S_IWGRP) ? "w" : "-");
                        printf((sb.st_mode & S_IXGRP) ? "x" : "-");
                        printf((sb.st_mode & S_IROTH) ? "r" : "-");
                        printf((sb.st_mode & S_IWOTH) ? "w" : "-");
                        printf((sb.st_mode & S_IXOTH) ? "x" : "-");
                        printf(" %s", pw->pw_name);
                        printf(" %s", gr->gr_name);
                        if (strncmp(entry->d_name, ".", 1) == 0 || strncmp(entry->d_name, "..", 2) == 0)
                            printf(" %s %s %s ", VERMELHO, entry->d_name, BRANCO);
                        else if (sb.st_mode & S_IFDIR)
                            printf(" %s %s %s", BRANCO, entry->d_name, BRANCO);
                        else if (sb.st_mode & S_IXUSR)
                            printf(" %s %s %s   ", VERDE, entry->d_name, BRANCO);
                        printf("\n");
                    }
                    else if (param[0] == 1)
                    { // -l
                        struct passwd *pw = getpwuid(sb.st_uid);
                        struct group *gr = getgrgid(sb.st_gid);
                        if (sb.st_mode & S_IFDIR)
                            ;
                        else if (sb.st_mode & S_IXUSR)
                        {
                            printf((sb.st_mode & S_IFDIR) ? "d" : "-");
                            printf((sb.st_mode & S_IRUSR) ? "r" : "-");
                            printf((sb.st_mode & S_IWUSR) ? "w" : "-");
                            printf((sb.st_mode & S_IXUSR) ? "x" : "-");
                            printf((sb.st_mode & S_IRGRP) ? "r" : "-");
                            printf((sb.st_mode & S_IWGRP) ? "w" : "-");
                            printf((sb.st_mode & S_IXGRP) ? "x" : "-");
                            printf((sb.st_mode & S_IROTH) ? "r" : "-");
                            printf((sb.st_mode & S_IWOTH) ? "w" : "-");
                            printf((sb.st_mode & S_IXOTH) ? "x" : "-");
                            printf(" %s", pw->pw_name);
                            printf(" %s", gr->gr_name);
                            printf(" %s %s %s   ", VERDE, entry->d_name, BRANCO);
                            printf("\n");
                        }
                    }
                    else if (param[1] == 1)
                    { // -a
                        if (strncmp(entry->d_name, ".", 1) == 0 || strncmp(entry->d_name, "..", 2) == 0)
                            printf(" %s %s %s ", VERMELHO, entry->d_name, BRANCO);
                        else if (sb.st_mode & S_IFDIR)
                            printf("%s %s %s", BRANCO, entry->d_name, BRANCO);
                        else if (sb.st_mode & S_IXUSR)
                            printf("%s %s %s   ", VERDE, entry->d_name, BRANCO);
                        else if (sb.st_mode & S_IFLNK)
                            printf("%s %s %s   ", CYAN, entry->d_name, BRANCO);
                    }
                    else if (param[2] == 1)
                    {
                        //-R
                        if (sb.st_mode)
                        {
                            cnt = 0;
                            free(dynamic_threads);
                            dynamic_threads = NULL;
                            dynamic_threads = realloc(dynamic_threads, (cnt + 1) * sizeof(pthread_t));
                            pthread_create(&dynamic_threads[cnt], NULL, list_dir, aux_pwd);
                            int c = 0;
                            for (c = 0; c <= cnt; c++)
                            {
                                pthread_join(dynamic_threads[c], NULL);
                            }
                            printf("\n");
                            closedir(dir);
                            return;
                        }
                    }
                    else
                    {
                        if (strncmp(entry->d_name, ".", 1) != 0 && strncmp(entry->d_name, "..", 2) != 0)
                        {
                            if (sb.st_mode & S_IFDIR)
                                printf("%s %s %s", AZUL, entry->d_name, BRANCO);
                            else if (sb.st_mode & S_IXUSR)
                                printf("%s %s %s   ", VERDE, entry->d_name, BRANCO);
                            else if (sb.st_mode & S_IFLNK)
                                printf("%s %s %s   ", CYAN, entry->d_name, BRANCO);
                        }
                    }
                }
            }
            printf("\n");
            closedir(dir);
        }
    }
    else if (strcmp(root->argv[0], "myfind") == 0)
    {
        char pwd[2024] = "\0";
        getcwd(pwd, sizeof(pwd));
        char *aux_pwd = malloc((strlen(pwd) + 1) * sizeof(char));
        strcpy(aux_pwd, pwd);

        sem_init(&can_cons, 0, 1);
        sem_init(&can_prod, 0, N - 1);

        int i;
        cnt = prodptr = consptr = nItem = 0;

        myfind = realloc(myfind, N * sizeof(char **));

        myfind[prodptr] = malloc((strlen(aux_pwd) + 2) * sizeof(char));
        strcpy(myfind[prodptr], aux_pwd);
        nItem++;
        prodptr = (prodptr + 1) % N;

        pthread_create(&tidP, NULL, &produtor, aux_pwd);
        for (i = 0; i < CONSUMERS; i++)
        {
            pthread_create(&tidC[i], NULL, &consumidor, (void *)root->argv[1]);
        }
        pthread_join(tidP, NULL);

        for (i = 0; i < CONSUMERS; i++)
        {
            pthread_join(tidC[i], NULL);
        }

        printf("\n");
        return;
    }
}

/*
 * @brief print directories (file names) recursively - myls -R
 * @param void* name - directory to use
 * @return next directory
 * 
 * browse the directory and print the files
 * whenever it finds another directory create new task with directory as parameter
 */
void *list_dir(void *name)
{
    char *diretorio = strdup(name);

    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(diretorio)))
        return;

    printf("\n%s: \n", diretorio);
    while ((entry = readdir(dir)) != NULL)
    {
        if (strncmp(entry->d_name, ".", 1) != 0 && strncmp(entry->d_name, "..", 2) != 0)
        {
            if (entry->d_type == DT_DIR)
            {
                char *path = NULL;
                path = malloc((strlen(diretorio) + strlen(entry->d_name) + 2) * sizeof(char));
                strcpy(path, diretorio);
                strcat(path, "/");
                strcat(path, entry->d_name);
                printf(" %s %s %s", AZUL, entry->d_name, BRANCO);
                // CRITICAL AREA
                pthread_mutex_lock(&mutex);
                cnt++;
                dynamic_threads = realloc(dynamic_threads, (cnt + 1) * sizeof(pthread_t));
                pthread_create(&dynamic_threads[cnt], NULL, &list_dir, path);
                pthread_mutex_unlock(&mutex);
                // END OF CRITICAL AREA
            }
            else
            {
                printf(" %s %s %s   ", VERDE, entry->d_name, BRANCO);
            }
        }
    }
    printf("\n");
    closedir(dir);
}

/*
 * @brief save directories in myfind[] recursively
 * @param void* name - directory to use
 * @return next directory
 */
void *produtor(void *name)
{
    char *diretorio = strdup(name);

    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(diretorio)))
        return;

    while ((entry = readdir(dir)) != NULL)
    {
        if (strncmp(entry->d_name, ".", 1) != 0 && strncmp(entry->d_name, "..", 2) != 0)
        {
            if (entry->d_type == DT_DIR)
            {
                sem_wait(&can_prod);
                pthread_mutex_lock(&mutexP);
                myfind[prodptr] = malloc((strlen(diretorio) + strlen(entry->d_name) + 2) * sizeof(char));
                strcpy(myfind[prodptr], diretorio);
                strcat(myfind[prodptr], "/");
                strcat(myfind[prodptr], entry->d_name);
                int aux = prodptr;
                prodptr = (prodptr + 1) % N; // to cycle from 0 to N
                nItem++;
                pthread_mutex_unlock(&mutexP);
                sem_post(&can_cons);
                produtor(myfind[aux]);
            }
        }
    }
    closedir(dir);
}

/*
 * @brief search the file in the directories array (myfind[])
 * @param void* name - file to search
 * - while there are directories to process
 *   - open the directory and search the file
 */
void *consumidor(void *name)
{
    char *fn; // file name
    fn = (char *)name;
    DIR *dir;
    struct dirent *entry;

    int aux = 0;
    while (nItem != 0)
    {
        sem_wait(&can_cons);
        pthread_mutex_lock(&mutexC);
        char *diretorio = malloc((strlen(myfind[consptr]) + 1) * sizeof(char));
        strcpy(diretorio, myfind[consptr]);
        if (!(dir = opendir(diretorio)))
            return;
        while ((entry = readdir(dir)) != NULL)
        {
            if (strncmp(entry->d_name, ".", 1) != 0 && strncmp(entry->d_name, "..", 2) != 0)
            {
                if (fn == NULL)
                {
                    printf("\n ./%s", entry->d_name);
                }
                else if (strcmp(entry->d_name, fn) == 0)
                {
                    printf("\n %s", diretorio);
                    printf("\n ./%s", entry->d_name);
                }
            }
        }
        closedir(dir);
        consptr = (consptr + 1) % N;
        nItem--;
        pthread_mutex_unlock(&mutexC);
        sem_post(&can_prod);
    }
}


/*
 * @brief Point 3
 * See second link
 */
char **
character_name_completion(const char *text, int start, int end)
{
    rl_attempted_completion_over = 1;
    return rl_completion_matches(text, character_name_generator);
}

/*
 * @brief Point 3
 * See second link
 */
char *
character_name_generator(const char *text, int state)
{
    static int list_index, len;
    char *name;

    if (!state)
    {
        list_index = 0;
        len = strlen(text);
    }

    while ((name = dictionary[list_index++]))
    {
        if (strncmp(name, text, len) == 0)
        {
            return strdup(name);
        }
    }

    return NULL;
}
