#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

#define MAXSIZE 100
#define BUFLEN 100
#define BUFFER_SIZE 1024
#define TOKEN_DELIMITERS " \t\r\n"
#define MAX_ARGS 64
#define COM (strstr(dir->d_name, partone) != NULL) && (strstr(dir->d_name, parttwo) != NULL) && (dir->d_name[0] != '.')

void execute_command(char *command);
char *find_executable(const char *command);

void expand_wildcard(char *token, char **tokens, int *argc){
	//char *token = "mysh*.c";
	char *files[1024];
	//int argc = strlen(token);
	int args = 0;
	
	char *partone = malloc(sizeof(char) * 50);
	partone[0] = '\0';
	char *parttwo = malloc(sizeof(char) * 50);
	parttwo[0] = '\0';
	int curchar = 0;
	 for(int i = 0; i < strlen(token); i++){
	 	if(token[i] != '*'){
	 		partone[i] = token[i];
	 	}
	 	else{
	 		curchar = i;
	 		strncat(partone, "\0", 1);
	 		break;
	 	}
	 }
	 printf("%s\n", partone);
	 int j = 0;
	 
	 for(int i = curchar; i < strlen(token); i++){
	 	
	 	if(token[i] != '*'){
	 		parttwo[j] = token[i];
	 		j++;
	 	}
	 }
	 strncat(parttwo, "\0", 1);
	 
	 printf("%s\n", parttwo);
	 
	 //compare the program names with 
	 int check = 0;
	 char cwd[1024];
	 DIR *handle = opendir(getcwd(cwd, sizeof(cwd)));
	 if(handle == NULL){
		perror(cwd);
		exit(EXIT_FAILURE);
	 }
	 
	 struct dirent *dir;
		while((dir = readdir(handle)))
		{
			//printf("directory file: %s\n", dir->d_name);
			if(COM){
				//add file name/executable that matches here
				files[args++] = dir->d_name;
			}
		}
			
		check = closedir(handle);
				
	if(check < 0)
	{
		perror(cwd);
		exit(EXIT_FAILURE);
	}
	
	if(args > 0){
		for(int i = 0; i < args; i++){
			tokens[*argc++] = files[i];
		}
	}
	else{
		tokens[*argc++] = token;
	}
	
	free(partone);
	free(parttwo);
}

char *find_executable(const char *command) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        //printf("%s\n", cwd);
    } else {
        perror("getcwd");
    }
    char *directories[] = {"/usr/local/bin", "/usr/bin", "/bin", cwd};
    int num_directories = sizeof(directories) / sizeof(directories[0]);

    for (int i = 0; i < num_directories; i++) {
        char *path = malloc(strlen(directories[i]) + strlen(command) + 2);
        sprintf(path, "%s/%s", directories[i], command);

        if (access(path, X_OK) == 0) {
            return path;
        }

        free(path);
    }

    return NULL;
}

void execute_command(char *command) {
    char *tokens[MAX_ARGS];
    int argc = 0, pipefound = 0, pipelocation = 0;
    char *input_file = NULL;
    char *output_file = NULL;
    int input_fd, output_fd;
    int prev_status = 0;
    
    // Tokenize the command
    char *token = strtok(command, TOKEN_DELIMITERS);
    while (token != NULL && argc < MAX_ARGS - 1) {
    	//printf("token: %s\n", token);
        if (strcmp(token, "<") == 0) {
            // Input redirection
            token = strtok(NULL, TOKEN_DELIMITERS);
            input_file = token;
        } else if (strcmp(token, ">") == 0) {
            // Output redirection
            token = strtok(NULL, TOKEN_DELIMITERS);
            output_file = token;
        } else if(strchr(token, '*')){
    	    expand_wildcard(token, tokens, &argc);
    	}else {
            tokens[argc++] = token;
        }
        //printf("token: %s\n", token);
        token = strtok(NULL, TOKEN_DELIMITERS);
    }
    tokens[argc] = NULL; // Null-terminate the token array

    if (argc == 0)
        return;

    for (int i = 0; i < argc; i++) {
        if (strchr(tokens[i], '|')) {
            pipefound = 1;
            pipelocation = i;
        }
    }

    char *pipecommand = malloc(sizeof(char) * 100);
    pipecommand[0] = '\0';
    if (pipefound == 1) {
        tokens[pipelocation] = NULL;
        for (int i = (pipelocation + 1); i < argc; i++) {
            strcat(pipecommand, tokens[i]);
            strcat(pipecommand, " ");
        }
    }

    if (strcmp(tokens[0], "cd") == 0) {
        // Change directory
        if (argc != 2) {
            fprintf(stderr, "cd: wrong number of arguments\n");
            return;
        }
        if (chdir(tokens[1]) != 0) {
            perror("cd");
        }
    } else if (strcmp(tokens[0], "pwd") == 0) {
        // Print current working directory
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("getcwd");
        }
    } else if (strcmp(tokens[0], "which") == 0) {
        // Print the path of a command
        if (argc != 2) {
            fprintf(stderr, "which: wrong number of arguments\n");
            return;
        }
        char *name = malloc(sizeof(char) * 50);;
        char *path = realpath(tokens[1], name);
        printf("%s\n", path);
        free(name);

    } else if (strcmp(tokens[0], "exit") == 0) {
        // Exit shell
        if (argc > 1) {
            for (int i = 1; i < argc; i++) {
                printf("%s ", tokens[i]);
            }
            printf("\n");
        }
        exit(EXIT_SUCCESS);
    } else if (strcmp(tokens[0], "then") == 0) {
        // Execute the command only if the previous command succeeded
        if (prev_status == 0) {
            char *new_command = malloc(sizeof(char) * 100);
            int i;
            
            for(i = 1; i < argc - 1; i++){
            	strcat(new_command, tokens[i]);
            	strcat(new_command, " ");
            }
            strcat(new_command, tokens[i]);
            
            //printf("then found: %s\n", new_command);
            execute_command(new_command);
        }
    } else if (strcmp(tokens[0], "else") == 0) {
        // Execute the command only if the previous command failed
        if (prev_status != 0) {
            char *new_command = malloc(sizeof(char) * 100);
            int i;
            
            for(i = 1; i < argc - 1; i++){
            	strcat(new_command, tokens[i]);
            	strcat(new_command, " ");
            }
            strcat(new_command, tokens[i]);
            
            //printf("then found: %s\n", new_command);
            execute_command(new_command);
        }
    } else {
        // Find executable path
        char *executable = find_executable(tokens[0]);
        if (executable == NULL) {
            printf("Command not found: %s\n", tokens[0]);
            return;
        }

        pid_t pid = fork();
        if (pid == 0) {
            // Child process

            // Input redirection
            if (input_file != NULL) {
                input_fd = open(input_file, O_RDONLY);
                if (input_fd < 0) {
                    perror("Input redirection");
                    exit(EXIT_FAILURE);
                }
                dup2(input_fd, STDIN_FILENO);
                close(input_fd);
            }

            // Output redirection
            if (output_file != NULL) {
                output_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0640);
                if (output_fd < 0) {
                    perror("Output redirection");
                    exit(EXIT_FAILURE);
                }
                dup2(output_fd, STDOUT_FILENO);
                close(output_fd);
            }

            if (execv(executable, tokens) == -1) {
                perror("execv");
                exit(EXIT_FAILURE);
            }
        } else if (pid > 0) {
            // Parent process
            int status;
            waitpid(pid, &status, 0);
            prev_status = WEXITSTATUS(status);
        } else {
            // Fork failed
            perror("fork");
        }

        if (pipefound == 1) {
            printf("command: %s\n", pipecommand);
            pipefound = 0;
            execute_command(pipecommand);
        }

        // Cleanup
        free(pipecommand);
        free(executable);
    }
}

int main(int argc, char **argv) {

    int fd = 0;

    if (argc > 1) {
        char *fname = argc > 1 ? argv[1] : "test.txt";
        fd = open(fname, O_RDONLY);
        if (fd < 0) {
            perror(fname);
            exit(EXIT_FAILURE);
        }
    }

    int bytes;
    char *buf = malloc(sizeof(char *) * BUFLEN);

    if (isatty(fd)) {
        // Interactive mode
        printf("Welcome to my shell!\n");
    } else {
        // Batch mode
        while ((bytes = read(fd, buf, BUFLEN)) > 0) {
            buf[strcspn(buf, "\n")] = 0;
            execute_command(buf);
        }

        free(buf);
        close(fd);
        exit(EXIT_SUCCESS);
    }

    char *message = malloc(sizeof(char) * 25);
    strcpy(message, "mysh> ");
    int prev_status = 0;
    while (strcmp(buf, "exit") != 0) {
        write(1, message, 7); // Writes "mysh> " to terminal (stdout)

        if ((bytes = read(0, buf, BUFLEN)) <= 0) {
            printf("No command given\n");
            break;
        }

        buf[strcspn(buf, "\n")] = 0;

        // Execute command only if the previous command succeeded
        if (prev_status == 0) {
            execute_command(buf);
        }

        prev_status = 0;
        if (strcmp(buf, "exit") != 0) {
            if (strstr(buf, "then") || strstr(buf, "else")) {
                prev_status = 1;
            }
        }
    }

    printf("mysh: exiting\n");

    free(message);
    free(buf);
    exit(EXIT_SUCCESS);
}