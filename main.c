#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

/**
 * get_location - Finds the location of a command in the PATH
 * @command: The command to search for
 *
 * Return: the full path to the command, or NULL
 */
char *get_location(char *command)
{
    char *path, *path_copy, *token, *full_path;
    int command_num, token_num;
    struct stat buff;

    path = getenv("PATH");
    if (!path)
    {
        return NULL;
    }

    // Handle empty PATH variable
    if (strlen(path) == 0)
    {
        if (stat(command, &buff) == 0 && S_ISREG(buff.st_mode) && (buff.st_mode & S_IXUSR))
        {
            return strdup(command);
        }
        return NULL;
    }

    path_copy = strdup(path);
    token = strtok(path_copy, ":");
    while (token != NULL)
    {
        command_num = strlen(command);
        token_num = strlen(token);
        full_path = malloc(command_num + token_num + 2);
        strcpy(full_path, token);
        strcat(full_path, "/");
        strcat(full_path, command);
        strcat(full_path, "\0");

        // Check if file exists and is executable
        if (access(full_path, X_OK) == 0)
        {
            free(path_copy);
            return full_path;
        }
        else
        {
            free(full_path);
            token = strtok(NULL, ":");
        }
    }
    free(path_copy);

    // Check if file exists and is executable in current directory
    if (stat(command, &buff) == 0 && S_ISREG(buff.st_mode) && (buff.st_mode & S_IXUSR))
    {
        return strdup(command);
    }

    return NULL;
}

/**
 * execmd - Executes a command with arguments
 * @argv: An array of strings containing the command and its arguments
 */
void execmd(char **argv)
{
    char *command = NULL, *path = NULL;

    if (argv)
    {
        command = argv[0];
        path = get_location(command);
        if (execve(path, argv, NULL) == -1)
        {
            perror("Error:");
            exit(1);
        };
    }
}

/**
 * print_environment - Prints all environment variables
 */
void print_environment(void)
{
    extern char **environ;
    char **envp = environ;
    while (*envp != NULL)
    {
        printf("%s\n", *envp);
        envp++;
    }
}

/**
 * execute_command - Executes a command in anew child process
 * @argv: An array of strings containing the command and its arguments
 */
void execute_command(char **argv)
{
    pid_t pid;
    pid = fork();
    if (pid == -1)
    {
        perror("tsh: fork error");
        exit(1);
    }
    else if (pid == 0)
    {
        execmd(argv);
        exit(0);
    }
    else
    {
        wait(NULL);
    }
}

/**
 * copy_file - Copies a file to a new location
 * @src: The path to the source file
 * @dst: The path to the destination file
 */
void copy_file(char *src, char *dst)
{
    FILE *fp1, *fp2;
    char ch;

    fp1 = fopen(src, "r");
    if (fp1 == NULL)
    {
        perror("Error:");
        exit(1);
    }

    fp2 = fopen(dst, "w");
    if (fp2 == NULL)
    {
        perror("Error:");
        exit(1);
    }

    while ((ch = fgetc(fp1)) != EOF)
    {
        fputc(ch, fp2);
    }

    fclose(fp1);
    fclose(fp2);
}

/**
 * main - Entry point for the tsh shell program
 * @argc: The number of arguments passed to the program
 * @argv: An array of strings containing the arguments passed tothe program
 *
 * Return: 0 on success, -1 on failure
 */
int main(int argc, char **argv)
{
    char *buff = NULL, *buff_copy = NULL, *token, *prompt = "$ ", *location;
    char *trimmed_input;
    size_t buff_size = 0;
    ssize_t characters;
    const char *delims = " \n";
    int i, j, token_num = 0;
    (void)argc;

    while (1)
    {
        printf("%s", prompt);
        characters = getline(&buff, &buff_size, stdin);
        if (characters == -1)
        {
            printf("Exiting shell....\n");
            exit(0);
        }
        // Trim leading/trailing spaces from input
        trimmed_input = strtok(buff, delims);
        if (trimmed_input == NULL)
        {
            continue;
        }

        buff_copy = malloc(sizeof(char) * characters);
        if (buff_copy == NULL)
        {
            perror("tsh: memory allocation error");
        }
        strcpy(buff_copy, trimmed_input);
        token = strtok(buff, delims);
        while (token != NULL)
        {
            token_num++;
            token = strtok(NULL, delims);
        }
        token_num++;
        argv = malloc(sizeof(char *) * token_num);
        token = strtok(buff_copy, delims);
        for (i = 0; token != NULL; i++)
        {
            argv[i] = malloc(sizeof(char) * (strlen(token) + 1));
            strcpy(argv[i], token);
            token = strtok(NULL, delims);
        }
        argv[i] = NULL;
        location = get_location(argv[0]);
        if (strcmp(argv[0], "exit") == 0)
            break;
        else if (strcmp(argv[0], "env") == 0)
        {
            print_environment();
        }
        else if (strcmp(argv[0], "cp") == 0 && argv[1] != NULL && argv[2] != NULL)
        {
            copy_file(argv[1], argv[2]);
        }
        else if (location == NULL)
        {
            printf("Command not found: %s\n", argv[0]);
            continue;
        }
        else
        {
            // Handle command execution multiple times
            int times = 1;
            if (argv[1] != NULL)
            {
                times = atoi(argv[1]);
                if (times == 0)
                {
                    times = 1;
                }
            }
            for (j = 0; j < times; j++)
            {
                execute_command(argv);
            }
        }
    }
    free(buff);
    free(buff_copy);
    return (0);
}
